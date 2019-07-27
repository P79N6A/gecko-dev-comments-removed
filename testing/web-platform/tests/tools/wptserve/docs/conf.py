# -*- coding: utf-8 -*-












import sys, os
sys.path.insert(0, os.path.abspath(".."))













extensions = ['sphinx.ext.autodoc', 'sphinx.ext.viewcode']


templates_path = ['_templates']


source_suffix = '.rst'





master_doc = 'index'


project = u'wptserve'
copyright = u'2013, Mozilla Foundation and other wptserve contributers'






version = '0.1'

release = '0.1'













exclude_patterns = ['_build']
















pygments_style = 'sphinx'









html_theme = 'default'




























html_static_path = ['_static']











































htmlhelp_basename = 'wptservedoc'




latex_elements = {








}



latex_documents = [
  ('index', 'wptserve.tex', u'wptserve Documentation',
   u'James Graham', 'manual'),
]


























man_pages = [
    ('index', 'wptserve', u'wptserve Documentation',
     [u'James Graham'], 1)
]










texinfo_documents = [
  ('index', 'wptserve', u'wptserve Documentation',
   u'James Graham', 'wptserve', 'One line description of project.',
   'Miscellaneous'),
]









