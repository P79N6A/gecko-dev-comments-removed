# -*- coding: utf-8 -*-



import os
import sys









needs_sphinx = "1.0"



extensions = ["sphinx.ext.intersphinx"]


templates_path = ["_templates"]


source_suffix = ".rst"





master_doc = "index"


project = u"six"
copyright = u"2010-2014, Benjamin Peterson"

sys.path.append(os.path.abspath(os.path.join(".", "..")))
from six import __version__ as six_version
sys.path.pop()






version = six_version[:-2]

release = six_version













exclude_patterns = ["_build"]
















pygments_style = "sphinx"









html_theme = "default"




























html_static_path = ["_static"]











































htmlhelp_basename = 'sixdoc'












latex_documents = [
  ("index", "six.tex", u"six Documentation",
   u"Benjamin Peterson", "manual"),
]





























man_pages = [
    ("index", "six", u"six Documentation",
     [u"Benjamin Peterson"], 1)
]



intersphinx_mapping = {"py2" : ("https://docs.python.org/2/", None),
                       "py3" : ("https://docs.python.org/3/", None)}
