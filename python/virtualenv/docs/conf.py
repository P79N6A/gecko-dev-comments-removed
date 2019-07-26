# -*- coding: utf-8 -*-












import sys









extensions = ['sphinx.ext.autodoc']






source_suffix = '.txt'


master_doc = 'index'


project = 'virtualenv'
copyright = '2007-2012, Ian Bicking, The Open Planning Project, The virtualenv developers'



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























