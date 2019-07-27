# coding: utf-8

"""
This module provides a central location for defining default behavior.

Throughout the package, these defaults take effect only when the user
does not otherwise specify a value.

"""

try:
    
    from html import escape
except ImportError:
    from cgi import escape

import os
import sys

from pystache.common import MissingTags









DECODE_ERRORS = 'strict'



STRING_ENCODING = sys.getdefaultencoding()




FILE_ENCODING = sys.getdefaultencoding()


DELIMITERS = (u'{{', u'}}')


MISSING_TAGS = MissingTags.ignore



SEARCH_DIRS = [os.curdir]  












TAG_ESCAPE = lambda u: escape(u, quote=True)


TEMPLATE_EXTENSION = 'mustache'
