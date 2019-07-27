




'''
Script to generate the browsersearch.json file for Fennec.

This script follows these steps:

1. Read the region.properties file in all the given source directories (see
srcdir option). Merge all properties into a single dict accounting for the
priority of source directories.

2. Read the default search plugin from the 'browser.search.defaultenginename'.

3. Read the list of search plugins from the 'browser.search.order.INDEX'
properties with values identifying particular search plugins by name.

4. Generate a JSON representation of 2. and 3., and write the result to
browsersearch.json in the locale-specific raw resource directory
e.g. raw/browsersearch.json, raw-pt-rBR/browsersearch.json.
'''

from __future__ import (
    print_function,
    unicode_literals,
)

import argparse
import codecs
import json
import re
import sys
import os

from mozbuild.dotproperties import (
    DotProperties,
)
from mozbuild.util import (
    FileAvoidWrite,
)
import mozpack.path as mozpath


def merge_properties(filename, srcdirs):
    """Merges properties from the given file in the given source directories."""
    properties = DotProperties()
    for srcdir in srcdirs:
        path = mozpath.join(srcdir, filename)
        try:
            properties.update(path)
        except IOError:
            
            continue
    return properties


def main(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', default=False, action='store_true',
                        help='be verbose')
    parser.add_argument('--silent', '-s', default=False, action='store_true',
                        help='be silent')
    parser.add_argument('--srcdir', metavar='SRCDIR',
                        action='append', required=True,
                        help='directories to read inputs from, in order of priority')
    parser.add_argument('output', metavar='OUTPUT',
                        help='output')
    opts = parser.parse_args(args)

    
    properties = merge_properties('region.properties', reversed(opts.srcdir))

    default = properties.get('browser.search.defaultenginename')
    engines = properties.get_list('browser.search.order')

    if opts.verbose:
        writer = codecs.getwriter('utf-8')(sys.stdout)
        print('Read {len} engines: {engines}'.format(len=len(engines), engines=engines), file=writer)
        print("Default engine is '{default}'.".format(default=default), file=writer)

    browsersearch = {}
    browsersearch['default'] = default
    browsersearch['engines'] = engines

    
    output = os.path.abspath(opts.output)
    fh = FileAvoidWrite(output)
    json.dump(browsersearch, fh)
    existed, updated = fh.close()

    if not opts.silent:
        if updated:
            print('{output} updated'.format(output=output))
        else:
            print('{output} already up-to-date'.format(output=output))

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
