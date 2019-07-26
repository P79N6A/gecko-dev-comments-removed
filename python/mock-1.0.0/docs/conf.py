# -*- coding: utf-8 -*-












import sys, os
sys.path.insert(0, os.path.abspath('..'))
from mock import __version__











extensions = ['sphinx.ext.doctest']

doctest_global_setup = """
import os
import sys
import mock
from mock import * # yeah, I know :-/
import unittest2
import __main__

if os.getcwd() not in sys.path:
    sys.path.append(os.getcwd())

# keep a reference to __main__
sys.modules['__main'] = __main__

class ProxyModule(object):
    def __init__(self):
        self.__dict__ = globals()

sys.modules['__main__'] = ProxyModule()
"""

doctest_global_cleanup = """
sys.modules['__main__'] = sys.modules['__main']
"""

html_theme = 'nature'
html_theme_options = {}





source_suffix = '.txt'


master_doc = 'index'


project = u'Mock'
copyright = u'2007-2012, Michael Foord & the mock team'





version = __version__[:3]

release = __version__





today_fmt = '%B %d, %Y'






exclude_trees = []









add_module_names = False






pygments_style = 'friendly'

































html_last_updated_fmt = '%b %d, %Y'













html_use_modindex = False



















htmlhelp_basename = 'Mockdoc'









latex_font_size = '12pt'



latex_documents = [
  ('index', 'Mock.tex', u'Mock Documentation',
   u'Michael Foord', 'manual'),
]
















latex_use_modindex = False