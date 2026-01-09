use std::ffi::{CStr, CString};
use std::ptr::NonNull;

#[derive(Debug, thiserror::Error)]
pub enum FfiError {
    #[error("failed to create storage")]
    CreateStorageFailed,

    #[error("query returned null result set")]
    ExecuteQueryFailed,

    #[error("invalid utf8")]
    Utf8(#[from] std::str::Utf8Error),
}

#[derive(Clone, Copy)]
struct StorageRaw(usize);

// Raw handles are safe to move across threads in this repo because:
// - The C++ storage implementation is internally synchronized (mutex)
// - We do not transfer ownership; the owning Storage must outlive any use
unsafe impl Send for StorageRaw {}

#[allow(non_camel_case_types)]
mod sys {
    #[repr(C)]
    pub struct KadeDB_Storage {
        _private: [u8; 0],
    }

    #[repr(C)]
    pub struct KadeDB_ResultSet {
        _private: [u8; 0],
    }

    extern "C" {
        pub fn KadeDB_CreateStorage() -> *mut KadeDB_Storage;
        pub fn KadeDB_DestroyStorage(storage: *mut KadeDB_Storage);

        pub fn KadeDB_ExecuteQuery(
            storage: *mut KadeDB_Storage,
            query: *const i8,
        ) -> *mut KadeDB_ResultSet;

        pub fn KadeDB_ResultSet_NextRow(rs: *mut KadeDB_ResultSet) -> i32;
        pub fn KadeDB_ResultSet_ColumnCount(rs: *mut KadeDB_ResultSet) -> i32;
        pub fn KadeDB_ResultSet_GetString(rs: *mut KadeDB_ResultSet, column: i32) -> *const i8;

        pub fn KadeDB_DestroyResultSet(rs: *mut KadeDB_ResultSet);
    }
}

pub struct Storage {
    raw: NonNull<sys::KadeDB_Storage>,
}

unsafe impl Send for Storage {}
unsafe impl Sync for Storage {}

impl Storage {
    pub fn new() -> Result<Self, FfiError> {
        let raw = unsafe { sys::KadeDB_CreateStorage() };
        let raw = NonNull::new(raw).ok_or(FfiError::CreateStorageFailed)?;
        Ok(Self { raw })
    }

    pub fn execute_query(&self, query: &str) -> Result<ResultSet, FfiError> {
        let c_query = CString::new(query).expect("query contains NUL");
        let rs = unsafe { sys::KadeDB_ExecuteQuery(self.raw.as_ptr(), c_query.as_ptr()) };
        let rs = NonNull::new(rs).ok_or(FfiError::ExecuteQueryFailed)?;
        Ok(ResultSet { raw: rs })
    }

    pub async fn execute_query_rows_as_strings(
        &self,
        query: String,
    ) -> Result<Vec<Vec<String>>, FfiError> {
        // This is a blocking FFI call; run in a blocking task.
        // IMPORTANT: do not move `NonNull` across threads; move a raw pointer instead.
        // Also, do not drop/destroy the storage from the blocking thread.
        let storage = StorageRaw(self.raw.as_ptr() as usize);

        tokio::task::spawn_blocking(move || unsafe {
            let c_query = CString::new(query).expect("query contains NUL");

            let storage_ptr = storage.0 as *mut sys::KadeDB_Storage;
            let rs = sys::KadeDB_ExecuteQuery(storage_ptr, c_query.as_ptr());
            let rs = NonNull::new(rs).ok_or(FfiError::ExecuteQueryFailed)?;
            let mut rs = ResultSet { raw: rs };
            rs.all_rows_as_strings()
        })
        .await
        .expect("spawn_blocking")
    }
}

impl Drop for Storage {
    fn drop(&mut self) {
        unsafe { sys::KadeDB_DestroyStorage(self.raw.as_ptr()) };
    }
}

pub struct ResultSet {
    raw: NonNull<sys::KadeDB_ResultSet>,
}

unsafe impl Send for ResultSet {}

impl ResultSet {
    pub fn column_count(&self) -> i32 {
        unsafe { sys::KadeDB_ResultSet_ColumnCount(self.raw.as_ptr()) }
    }

    pub fn next_row(&mut self) -> bool {
        unsafe { sys::KadeDB_ResultSet_NextRow(self.raw.as_ptr()) != 0 }
    }

    pub fn get_string(&self, column: i32) -> Option<String> {
        let ptr = unsafe { sys::KadeDB_ResultSet_GetString(self.raw.as_ptr(), column) };
        let ptr = NonNull::new(ptr as *mut i8)?;
        let s = unsafe { CStr::from_ptr(ptr.as_ptr()) };
        Some(s.to_str().ok()?.to_string())
    }

    pub fn all_rows_as_strings(&mut self) -> Result<Vec<Vec<String>>, FfiError> {
        let cols = self.column_count();
        if cols < 0 {
            return Ok(vec![]);
        }

        let mut out = Vec::new();
        while self.next_row() {
            let mut row = Vec::with_capacity(cols as usize);
            for i in 0..cols {
                row.push(self.get_string(i).unwrap_or_default());
            }
            out.push(row);
        }
        Ok(out)
    }
}

impl Drop for ResultSet {
    fn drop(&mut self) {
        unsafe { sys::KadeDB_DestroyResultSet(self.raw.as_ptr()) };
    }
}
