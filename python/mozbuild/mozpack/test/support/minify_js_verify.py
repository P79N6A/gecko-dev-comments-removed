



import sys


if len(sys.argv) != 4:
    raise Exception('Usage: minify_js_verify <exitcode> <orig> <minified>')

sys.exit(int(sys.argv[1]))
