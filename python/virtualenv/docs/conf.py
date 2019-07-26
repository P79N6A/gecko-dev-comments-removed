# -*- coding: utf-8 -*-












import os
import sys

on_rtd = os.environ.get('READTHEDOCS', None) == 'True'


sys.path.insert(0, os.path.abspath(os.pardir))






extensions = ['sphinx.ext.autodoc']






source_suffix = '.rst'


master_doc = 'index'


project = 'virtualenv'
copyright = '2007-2014, Ian Bicking, The Open Planning Project, PyPA'



try:
    from virtualenv import __version__
    
    version = '.'.join(__version__.split('.')[:2])
    
    release = __version__
except ImportError:
    version = release = 'dev'





today_fmt = '%B %d, %Y'


unused_docs = []













pygments_style = 'sphinx'










if os.environ.get('DOCS_LOCAL'):
    import sphinx_rtd_theme
    html_theme = "sphinx_rtd_theme"
    html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]
else:
    
    html_theme = 'default'









html_last_updated_fmt = '%b %d, %Y'






















htmlhelp_basename = 'Pastedoc'























