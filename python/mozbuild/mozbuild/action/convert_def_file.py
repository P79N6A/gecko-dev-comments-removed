






from __future__ import absolute_import

import itertools
import re
import sys
from StringIO import StringIO

from mozbuild.preprocessor import Preprocessor
from mozbuild.util import FileAvoidWrite

def preprocess_file(pp, deffile):
    output = StringIO()
    with open(deffile, 'r') as input:
        pp.processFile(input=input, output=output)
    return output.getvalue().splitlines()

























def nss_preprocess_file(deffile):
    with open(deffile, 'r') as input:
        for line in input:
            yield line.replace(';;', '')

COMMENT = re.compile(';.*')

def extract_symbols(lines):
    
    nocomments = iter(COMMENT.sub('', s).strip() for s in lines)
    lines = iter(s for s in nocomments if len(s))

    exports = itertools.dropwhile(lambda s: 'EXPORTS' not in s, lines)
    symbols = set()
    for line in exports:
        if 'EXPORTS' in line:
            
            fields = line.split()[1:]
            if len(fields) == 0:
                continue
        else:
            fields = line.split()

        
        
        
        if '=' in fields[0]:
            raise BaseException, 'aliases are not supported (%s)' % line
        if len(fields) == 1:
            pass
        elif len(fields) != 2 or fields[1] != 'DATA':
            raise BaseException, 'aliases and keywords other than DATA are not supported (%s)' % line

        symbols.add(fields[0])

    return symbols

def main(args):
    pp = Preprocessor()
    optparser = pp.getCommandLineParser()
    optparser.add_option('--nss-file', action='append',
                         type='string', dest='nss_files', default=[],
                         help='Specify a .def file that should have NSS\'s processing rules applied to it')
    options, deffiles = optparser.parse_args(args)

    symbols = set()
    for f in options.nss_files:
        symbols |= extract_symbols(nss_preprocess_file(f))
    for f in deffiles:
        
        defpp = pp.clone()
        symbols |= extract_symbols(preprocess_file(defpp, f))

    script = """{
global:
  %s
local:
  *;
};
"""
    with FileAvoidWrite(options.output) as f:
        f.write(script % '\n  '.join("%s;" % s for s in sorted(symbols)))

if __name__ == '__main__':
    main(sys.argv[1:])
