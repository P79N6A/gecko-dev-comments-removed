# coding: utf-8

"""
Exposes functionality needed throughout the project.

"""

from sys import version_info

def _get_string_types():
    
    
    
    
    if version_info < (3, ):
         return basestring
    
    return (unicode, type(u"a".encode('utf-8')))


_STRING_TYPES = _get_string_types()


def is_string(obj):
    """
    Return whether the given object is a byte string or unicode string.

    This function is provided for compatibility with both Python 2 and 3
    when using 2to3.

    """
    return isinstance(obj, _STRING_TYPES)




def read(path):
    """
    Return the contents of a text file as a byte string.

    """
    
    
    
    
    
    
    f = open(path, 'rb')
    
    try:
        return f.read()
    finally:
        f.close()


class MissingTags(object):

    """Contains the valid values for Renderer.missing_tags."""

    ignore = 'ignore'
    strict = 'strict'


class PystacheError(Exception):
    """Base class for Pystache exceptions."""
    pass


class TemplateNotFoundError(PystacheError):
    """An exception raised when a template is not found."""
    pass
