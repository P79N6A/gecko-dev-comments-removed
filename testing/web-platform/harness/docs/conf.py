# -*- coding: utf-8 -*-













import sys
import os














extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.viewcode',
]


templates_path = ['_templates']


source_suffix = '.rst'





master_doc = 'index'


project = u'wptrunner'
copyright = u''






version = '0.3'

release = '0.3'













exclude_patterns = ['_build']

















pygments_style = 'sphinx'












html_theme = 'default'




























html_static_path = ['_static']
















































htmlhelp_basename = 'wptrunnerdoc'




latex_elements = {








}




latex_documents = [
  ('index', 'wptrunner.tex', u'wptrunner Documentation',
   u'James Graham', 'manual'),
]


























man_pages = [
    ('index', 'wptrunner', u'wptrunner Documentation',
     [u'James Graham'], 1)
]










texinfo_documents = [
  ('index', 'wptrunner', u'wptrunner Documentation',
   u'James Graham', 'wptrunner', 'One line description of project.',
   'Miscellaneous'),
]















intersphinx_mapping = {'python': ('http://docs.python.org/', None),
                       'mozlog': ('http://mozbase.readthedocs.org/en/latest/', None)}
