



from __future__ import print_function
import sys


if len(sys.argv) != 4:
    raise Exception('Usage: minify_js_verify <exitcode> <orig> <minified>')

retcode = int(sys.argv[1])

if retcode:
    print('Error message', file=sys.stderr)

sys.exit(retcode)
