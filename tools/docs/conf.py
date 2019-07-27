



from __future__ import unicode_literals

import os
import re

from datetime import datetime


mozilla_dir = os.environ['MOZILLA_DIR']

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.graphviz',
    'sphinx.ext.todo',
    'mozbuild.sphinx',
]

templates_path = ['_templates']
source_suffix = '.rst'
master_doc = 'index'
project = u'Mozilla Source Tree Docs'
year = datetime.now().year



with open(os.path.join(mozilla_dir, 'config', 'milestone.txt'), 'rt') as fh:
    for line in fh:
        line = line.strip()

        if not line or line.startswith('#'):
            continue

        release = line
        break

version = re.sub(r'[ab]\d+$', '', release)

exclude_patterns = ['_build']
pygments_style = 'sphinx'


on_rtd = os.environ.get('READTHEDOCS', None) == 'True'

if not on_rtd:
    import sphinx_rtd_theme
    html_theme = 'sphinx_rtd_theme'
    html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]


html_static_path = ['_static']
htmlhelp_basename = 'MozillaTreeDocs'
