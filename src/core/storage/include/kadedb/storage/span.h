#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>

namespace kadedb {
namespace storage {

template <typename T>
class span {
public:
    using element_type = T;
    using value_type = typename std::remove_cv<T>::type;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr span() noexcept : data_(nullptr), size_(0) {}
    constexpr span(pointer data, size_type count) : data_(data), size_(count) {}
    
    template <class It>
    span(It first, size_type count) : data_(&*first), size_(count) {}
    
    template <class It, class End>
    span(It first, End last) : data_(&*first), size_(last - first) {}
    
    template <std::size_t N>
    constexpr span(element_type (&arr)[N]) noexcept : data_(arr), size_(N) {}
    
    template <class Container>
    span(Container& c) : data_(c.data()), size_(c.size()) {}
    
    iterator begin() const noexcept { return data_; }
    iterator end() const noexcept { return data_ + size_; }
    const_iterator cbegin() const noexcept { return data_; }
    const_iterator cend() const noexcept { return data_ + size_; }
    
    reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }
    
    reference front() const { return *data_; }
    reference back() const { return *(data_ + size_ - 1); }
    reference operator[](size_type idx) const { return data_[idx]; }
    
    pointer data() const noexcept { return data_; }
    size_type size() const noexcept { return size_; }
    size_type size_bytes() const noexcept { return size_ * sizeof(element_type); }
    bool empty() const noexcept { return size_ == 0; }
    
    span<element_type> first(size_type count) const {
        return {data_, count};
    }
    
    span<element_type> last(size_type count) const {
        return {data_ + size_ - count, count};
    }
    
    span<element_type> subspan(size_type offset, size_type count = static_cast<size_type>(-1)) const {
        if (count == static_cast<size_type>(-1)) {
            count = size_ - offset;
        }
        return {data_ + offset, count};
    }
    
private:
    pointer data_;
    size_type size_;
};

} // namespace storage
} // namespace kadedb
