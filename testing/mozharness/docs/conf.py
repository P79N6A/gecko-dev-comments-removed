# -*- coding: utf-8 -*-













import sys
import os





sys.path.insert(0, os.path.abspath('../scripts'))
sys.path.insert(0, os.path.abspath('../mozharness'))









extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.viewcode',
]


templates_path = ['_templates']


source_suffix = '.rst'





master_doc = 'index'


project = u'Moz Harness'
copyright = u'2014, aki and a cast of tens!'






version = '0.1'

release = '0.1'













exclude_patterns = ['_build']

















pygments_style = 'sphinx'












html_theme = 'default'




























html_static_path = ['_static']
















































htmlhelp_basename = 'MozHarnessdoc'




latex_elements = {








}




latex_documents = [
  ('index', 'MozHarness.tex', u'Moz Harness Documentation',
   u'aki and a cast of tens!', 'manual'),
]


























man_pages = [
    ('index', 'mozharness', u'Moz Harness Documentation',
     [u'aki and a cast of tens!'], 1)
]










texinfo_documents = [
  ('index', 'MozHarness', u'Moz Harness Documentation',
   u'aki and a cast of tens!', 'MozHarness', 'One line description of project.',
   'Miscellaneous'),
]















intersphinx_mapping = {'http://docs.python.org/': None}
