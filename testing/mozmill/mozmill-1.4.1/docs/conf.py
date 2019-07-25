# -*- coding: utf-8 -*-















import sys, os











extensions = ["sphinx.ext.intersphinx"]

intersphinx_mapping = {'http://docs.python.org/dev': None,
                       'http://mozrunner.googlecode.com/svn/trunk/docs/_build/html/': None,
                       'http://jsbridge.googlecode.com/svn/trunk/docs/_build/html/': None}


templates_path = ['_templates']


source_suffix = '.rst'





master_doc = 'index'


project = u'mozmill'
copyright = u'2009, Mikeal Rogers <mikeal.rogers@gmail.com>'






version = '1.2.1a1'

release = '1.2.1a1'
















exclude_trees = ['_build']
















pygments_style = 'sphinx'








html_style = 'default.css'




















html_static_path = ['_static']





































htmlhelp_basename = 'mozmilldoc'













latex_documents = [
  ('index', 'mozmill.tex', ur'mozmill Documentation',
   ur'Mikeal Rogers <mikeal.rogers@gmail.com>', 'manual'),
]

















