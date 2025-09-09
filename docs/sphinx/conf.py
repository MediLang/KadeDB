# Sphinx configuration for KadeDB
# Requires: sphinx, breathe, (optional) myst-parser

import os
import sys
from datetime import datetime

# -- Project information -----------------------------------------------------
project = 'KadeDB'
author = 'MediLang Team'
copyright = f'{datetime.now():%Y}, MediLang'

# -- General configuration ---------------------------------------------------
extensions = [
    'breathe',
    'myst_parser',  # optional, for Markdown support in Sphinx
]

# Source file parsers
source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

# Templates & exclusions
templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# -- Breathe (Doxygen XML) ---------------------------------------------------
# Assumes Doxygen outputs XML to docs/build/xml
breathe_projects = {
    'KadeDB': os.path.abspath(os.path.join('..', 'build', 'xml')),
}
breathe_default_project = 'KadeDB'

# -- HTML output -------------------------------------------------------------
html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']
