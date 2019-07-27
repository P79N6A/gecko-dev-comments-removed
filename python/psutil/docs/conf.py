# -*- coding: utf-8 -*-













import datetime
import os
import sys


if sys.version_info >= (3, ):
    def u(s):
        return s
else:
    def u(s):
        if not isinstance(s, unicode):  
            s = unicode(s, "unicode_escape")  
        return s


PROJECT_NAME = u("psutil")
AUTHOR = u("Giampaolo Rodola'")
THIS_YEAR = str(datetime.datetime.now().year)
HERE = os.path.abspath(os.path.dirname(__file__))


def get_version():
    INIT = os.path.abspath(os.path.join(HERE, '../psutil/__init__.py'))
    f = open(INIT, 'r')
    try:
        for line in f:
            if line.startswith('__version__'):
                ret = eval(line.strip().split(' = ')[1])
                assert ret.count('.') == 2, ret
                for num in ret.split('.'):
                    assert num.isdigit(), ret
                return ret
        else:
            raise ValueError("couldn't find version string")
    finally:
        f.close()

VERSION = get_version()


needs_sphinx = '1.0'




extensions = ['sphinx.ext.autodoc',
              'sphinx.ext.coverage',
              'sphinx.ext.pngmath',
              'sphinx.ext.viewcode',
              'sphinx.ext.intersphinx']


templates_path = ['_template']


source_suffix = '.rst'





master_doc = 'index'


project = PROJECT_NAME
copyright = u('2009-%s, %s' % (THIS_YEAR, AUTHOR))






version = VERSION













exclude_patterns = ['_build']






add_function_parentheses = True




autodoc_docstring_signature = True






pygments_style = 'sphinx'













html_theme = 'pydoctheme'
html_theme_options = {'collapsiblesidebar': True}


html_theme_path = ["_themes"]



html_title = "{project} {version} documentation".format(**locals())











html_favicon = '_static/favicon.ico'




html_static_path = ['_static']



html_last_updated_fmt = '%b %d, %Y'



html_use_smartypants = True


html_sidebars = {
    'index': 'indexsidebar.html',
    '**': ['globaltoc.html',
           'relations.html',
           'sourcelink.html',
           'searchbox.html']
}








html_domain_indices = False


html_use_index = True






















htmlhelp_basename = '%s-doc' % PROJECT_NAME












latex_documents = [
    ('index', '%s.tex' % PROJECT_NAME,
     u('%s documentation') % PROJECT_NAME, AUTHOR),
]





























man_pages = [
    ('index', PROJECT_NAME, u('%s documentation') % PROJECT_NAME, [AUTHOR], 1)
]



