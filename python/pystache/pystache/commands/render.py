# coding: utf-8

"""
This module provides command-line access to pystache.

Run this script using the -h option for command-line help.

"""


try:
    import json
except:
    
    
    try:
        import simplejson as json
    except ImportError:
        
        
        
        from sys import exc_info
        ex_type, ex_value, tb = exc_info()
        new_ex = Exception("%s: %s" % (ex_type.__name__, ex_value))
        raise new_ex.__class__, new_ex, tb



from optparse import OptionParser
import sys







from pystache.common import TemplateNotFoundError
from pystache.renderer import Renderer


USAGE = """\
%prog [-h] template context

Render a mustache template with the given context.

positional arguments:
  template    A filename or template string.
  context     A filename or JSON string."""


def parse_args(sys_argv, usage):
    """
    Return an OptionParser for the script.

    """
    args = sys_argv[1:]

    parser = OptionParser(usage=usage)
    options, args = parser.parse_args(args)

    template, context = args

    return template, context







def main(sys_argv=sys.argv):
    template, context = parse_args(sys_argv, USAGE)

    if template.endswith('.mustache'):
        template = template[:-9]

    renderer = Renderer()

    try:
        template = renderer.load_template(template)
    except TemplateNotFoundError:
        pass

    try:
        context = json.load(open(context))
    except IOError:
        context = json.loads(context)

    rendered = renderer.render(template, context)
    print rendered


if __name__=='__main__':
    main()
