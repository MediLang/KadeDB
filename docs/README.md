# KadeDB Documentation

Welcome to the KadeDB documentation! This directory contains all the documentation for the KadeDB project, including design documents, API references, and usage examples.

## Directory Structure

- `design/`: Design documents, architecture decisions, and technical specifications
- `api/`: API documentation and references
- `examples/`: Code examples and tutorials

## Documentation Guidelines

### Writing Documentation

1. Use Markdown (`.md`) for all documentation files
2. Follow the [Google Developer Documentation Style Guide](https://developers.google.com/style)
3. Include code examples where applicable
4. Keep diagrams in the `images/` directory
5. Update documentation when making code changes

### Building Documentation

KadeDB uses [Doxygen](https://www.doxygen.nl/) for API documentation and [Sphinx](https://www.sphinx-doc.org/) for user documentation.

#### Prerequisites

- Doxygen 1.8.5 or higher
- Sphinx 3.0 or higher
- Python 3.6 or higher
- pip

#### Setup

1. Install Python dependencies:
   ```bash
   pip install -r requirements-docs.txt
   ```

2. Generate API documentation:
   ```bash
   doxygen Doxyfile
   ```

3. Build HTML documentation:
   ```bash
   cd docs
   make html
   ```

The generated documentation will be available in `docs/_build/html/`.

## Contributing to Documentation

1. Fork the repository
2. Create a new branch for your changes
3. Make your changes
4. Submit a pull request

## License

Documentation is licensed under the [Creative Commons Attribution 4.0 International License](https://creativecommons.org/licenses/by/4.0/).

## Contact

For questions or suggestions about the documentation, please open an issue on GitHub.
