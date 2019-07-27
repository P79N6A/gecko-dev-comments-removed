# -*- coding: utf-8 -*-












import sys, os




sys.path.insert(0, os.path.abspath('../'))








extensions = ['sphinx.ext.coverage', 'sphinx.ext.viewcode', 'sphinx.ext.autodoc']
autoclass_content = 'both'


templates_path = ['_templates']


source_suffix = '.rst'





master_doc = 'index'


project = u'BrowserMob Proxy'
copyright = u'2014, David Burns'






version = '0.6.0'

release = '0.6.0'













exclude_patterns = ['_build']
















pygments_style = 'sphinx'









html_theme = 'default'




























html_static_path = ['_static']
































html_show_copyright = True










htmlhelp_basename = 'BrowserMobProxydoc'




latex_elements = {








}



latex_documents = [
  ('index', 'BrowserMobProxy.tex', u'BrowserMob Proxy Documentation',
   u'David Burns', 'manual'),
]


























man_pages = [
    ('index', 'browsermobproxy', u'BrowserMob Proxy Documentation',
     [u'David Burns'], 1)
]










texinfo_documents = [
  ('index', 'BrowserMobProxy', u'BrowserMob Proxy Documentation',
   u'David Burns', 'BrowserMobProxy', 'One line description of project.',
   'Miscellaneous'),
]









