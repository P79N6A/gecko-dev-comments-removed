# -*- coding: utf-8 -*-












import os
import sys


sys.path.insert(0, os.path.abspath(os.pardir))






extensions = ['sphinx.ext.autodoc']






source_suffix = '.rst'


master_doc = 'index'


project = 'virtualenv'
copyright = '2007-2013, Ian Bicking, The Open Planning Project, The virtualenv developers'



try:
    from virtualenv import __version__
    
    version = '.'.join(__version__.split('.')[:2])
    
    release = __version__
except ImportError:
    version = release = 'dev'





today_fmt = '%B %d, %Y'


unused_docs = []













pygments_style = 'sphinx'










html_theme = 'nature'
html_theme_path = ['_theme']








html_last_updated_fmt = '%b %d, %Y'






















htmlhelp_basename = 'Pastedoc'























