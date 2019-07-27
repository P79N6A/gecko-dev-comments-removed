
# -*- coding: utf-8 -*-












import sys, os













extensions = ['sphinx.ext.autodoc', 'sphinx.ext.doctest', 'sphinx.ext.viewcode']


templates_path = ['_templates']


source_suffix = '.rst'





master_doc = 'index'


project = 'html5lib'
copyright = '2006 - 2013, James Graham, Geoffrey Sneddon, and contributors'






version = '1.0'

sys.path.append(os.path.abspath('..'))
from html5lib import __version__
release = __version__













exclude_patterns = ['_build', 'theme']
















pygments_style = 'sphinx'












html_theme = 'default'




























html_static_path = ['_static']











































htmlhelp_basename = 'html5libdoc'




latex_elements = {








}



latex_documents = [
  ('index', 'html5lib.tex', 'html5lib Documentation',
   'James Graham, Geoffrey Sneddon, and contributors', 'manual'),
]


























man_pages = [
    ('index', 'html5lib', 'html5lib Documentation',
     ['James Graham, Geoffrey Sneddon, and contributors'], 1)
]










texinfo_documents = [
  ('index', 'html5lib', 'html5lib Documentation',
   'James Graham, Geoffrey Sneddon, and contributors', 'html5lib', 'One line description of project.',
   'Miscellaneous'),
]













class CExtMock(object):
    """Required for autodoc on readthedocs.org where you cannot build C extensions."""
    def __init__(self, *args, **kwargs):
        pass

    def __call__(self, *args, **kwargs):
        return CExtMock()

    @classmethod
    def __getattr__(cls, name):
        if name in ('__file__', '__path__'):
            return '/dev/null'
        else:
            return CExtMock()

try:
    import lxml   
except ImportError:
    sys.modules['lxml'] = CExtMock()
    sys.modules['lxml.etree'] = CExtMock()
    print("warning: lxml modules mocked.")

try:
    import genshi   
except ImportError:
    sys.modules['genshi'] = CExtMock()
    sys.modules['genshi.core'] = CExtMock()
    print("warning: genshi modules mocked.")
