# -*- coding: utf-8 -*-












import sys, os




here = os.path.dirname(os.path.abspath(__file__))
parent = os.path.dirname(here)
for item in os.listdir(parent):
    path = os.path.join(parent, item)
    if (not os.path.isdir(path)) or (not os.path.exists(os.path.join(path, 'setup.py'))):
        continue
    sys.path.insert(0, path)








extensions = ['sphinx.ext.autodoc', 'sphinx.ext.doctest', 'sphinx.ext.todo', 'sphinx.ext.viewcode']


templates_path = ['_templates']


source_suffix = '.rst'





master_doc = 'index'


project = u'MozBase'
copyright = u'2012, Mozilla Automation and Tools team'






version = '1'

release = '1'













exclude_patterns = ['_build']
















pygments_style = 'sphinx'









html_theme = 'default'
on_rtd = os.environ.get('READTHEDOCS', None) == 'True'

if not on_rtd:
    try:
        import sphinx_rtd_theme
        html_theme = 'sphinx_rtd_theme'
        html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]
    except ImportError:
        pass











html_title = "mozbase documentation"
















html_static_path = ['_static']











































htmlhelp_basename = 'MozBasedoc'




latex_elements = {








}



latex_documents = [
  ('index', 'MozBase.tex', u'MozBase Documentation',
   u'Mozilla Automation and Tools team', 'manual'),
]


























man_pages = [
    ('index', 'mozbase', u'MozBase Documentation',
     [u'Mozilla Automation and Tools team'], 1)
]










texinfo_documents = [
  ('index', 'MozBase', u'MozBase Documentation',
   u'Mozilla Automation and Tools team', 'MozBase', 'One line description of project.',
   'Miscellaneous'),
]









