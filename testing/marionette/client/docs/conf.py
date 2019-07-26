# -*- coding: utf-8 -*-












import sys, os






here = os.path.dirname(os.path.abspath(__file__))
parent = os.path.dirname(here)
sys.path.insert(0, parent)








extensions = ['sphinx.ext.autodoc']


templates_path = ['_templates']


source_suffix = '.rst'





master_doc = 'index'


project = u'Marionette Python Client'
copyright = u'2013, Mozilla Automation and Tools and individual contributors'






















exclude_patterns = ['_build']
















pygments_style = 'sphinx'









html_theme = 'default'




























html_static_path = ['_static']











































htmlhelp_basename = 'MarionettePythonClientdoc'




latex_elements = {








}



latex_documents = [
  ('index', 'MarionettePythonClient.tex', u'Marionette Python Client Documentation',
   u'Mozilla Automation and Tools team', 'manual'),
]


























man_pages = [
    ('index', 'marionettepythonclient', u'Marionette Python Client Documentation',
     [u'Mozilla Automation and Tools team'], 1)
]










texinfo_documents = [
  ('index', 'MarionettePythonClient', u'Marionette Python Client Documentation',
   u'Mozilla Automation and Tools team', 'MarionettePythonClient', 'One line description of project.',
   'Miscellaneous'),
]









