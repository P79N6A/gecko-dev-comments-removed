# coding: utf-8

"""
Defines a class responsible for rendering logic.

"""

import re

from pystache.common import is_string
from pystache.parser import parse


def context_get(stack, name):
    """
    Find and return a name from a ContextStack instance.

    """
    return stack.get(name)


class RenderEngine(object):

    """
    Provides a render() method.

    This class is meant only for internal use.

    As a rule, the code in this class operates on unicode strings where
    possible rather than, say, strings of type str or markupsafe.Markup.
    This means that strings obtained from "external" sources like partials
    and variable tag values are immediately converted to unicode (or
    escaped and converted to unicode) before being operated on further.
    This makes maintaining, reasoning about, and testing the correctness
    of the code much simpler.  In particular, it keeps the implementation
    of this class independent of the API details of one (or possibly more)
    unicode subclasses (e.g. markupsafe.Markup).

    """

    
    
    
    
    def __init__(self, literal=None, escape=None, resolve_context=None,
                 resolve_partial=None, to_str=None):
        """
        Arguments:

          literal: the function used to convert unescaped variable tag
            values to unicode, e.g. the value corresponding to a tag
            "{{{name}}}".  The function should accept a string of type
            str or unicode (or a subclass) and return a string of type
            unicode (but not a proper subclass of unicode).
                This class will only pass basestring instances to this
            function.  For example, it will call str() on integer variable
            values prior to passing them to this function.

          escape: the function used to escape and convert variable tag
            values to unicode, e.g. the value corresponding to a tag
            "{{name}}".  The function should obey the same properties
            described above for the "literal" function argument.
                This function should take care to convert any str
            arguments to unicode just as the literal function should, as
            this class will not pass tag values to literal prior to passing
            them to this function.  This allows for more flexibility,
            for example using a custom escape function that handles
            incoming strings of type markupsafe.Markup differently
            from plain unicode strings.

          resolve_context: the function to call to resolve a name against
            a context stack.  The function should accept two positional
            arguments: a ContextStack instance and a name to resolve.

          resolve_partial: the function to call when loading a partial.
            The function should accept a template name string and return a
            template string of type unicode (not a subclass).

          to_str: a function that accepts an object and returns a string (e.g.
            the built-in function str).  This function is used for string
            coercion whenever a string is required (e.g. for converting None
            or 0 to a string).

        """
        self.escape = escape
        self.literal = literal
        self.resolve_context = resolve_context
        self.resolve_partial = resolve_partial
        self.to_str = to_str

    

    
    
    
    
    
    
    
    def fetch_string(self, context, name):
        """
        Get a value from the given context as a basestring instance.

        """
        val = self.resolve_context(context, name)

        if callable(val):
            
            return self._render_value(val(), context)

        if not is_string(val):
            return self.to_str(val)

        return val

    def fetch_section_data(self, context, name):
        """
        Fetch the value of a section as a list.

        """
        data = self.resolve_context(context, name)

        
        
        
        
        
        
        
        if not data:
            data = []
        else:
            
            
            
            
            
            
            
            
            
            try:
                iter(data)
            except TypeError:
                
                data = [data]
            else:
                if is_string(data) or isinstance(data, dict):
                    
                    data = [data]
                

        return data

    def _render_value(self, val, context, delimiters=None):
        """
        Render an arbitrary value.

        """
        if not is_string(val):
            
            val = self.to_str(val)
        if type(val) is not unicode:
            val = self.literal(val)
        return self.render(val, context, delimiters)

    def render(self, template, context_stack, delimiters=None):
        """
        Render a unicode template string, and return as unicode.

        Arguments:

          template: a template string of type unicode (but not a proper
            subclass of unicode).

          context_stack: a ContextStack instance.

        """
        parsed_template = parse(template, delimiters)

        return parsed_template.render(self, context_stack)
