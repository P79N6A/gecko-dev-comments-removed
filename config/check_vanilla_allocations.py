





































from __future__ import print_function

import argparse
import re
import subprocess
import sys













has_failed = False


def fail(msg):
    print('TEST-UNEXPECTED-FAIL | check_vanilla_allocations.py |', msg)
    global has_failed
    has_failed = True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--aggressive', action='store_true',
                        help='also check for malloc, calloc, realloc and free')
    parser.add_argument('file', type=str,
                        help='name of the file to check')
    args = parser.parse_args()

    
    
    
    
    cmd = ['nm', '-u', '-C', '-l', args.file]
    lines = subprocess.check_output(cmd, universal_newlines=True,
                                    stderr=subprocess.PIPE).split('\n')

    
    

    alloc_fns = [
        
        r'operator new\(unsigned',

        
        r'operator new\[\]\(unsigned',

        r'memalign',
        
        
        
        r'valloc',
    ]

    if args.aggressive:
        alloc_fns += [
            r'malloc',
            r'calloc',
            r'realloc',
            r'free',
            r'strdup'
        ]

    
    alloc_fns_unescaped = [fn.translate(None, r'\\') for fn in alloc_fns]

    
    
    
    
    
    alloc_fns_re = r'U (' + r'|'.join(alloc_fns) + r').*\/([\w\.]+):(\d+)$'

    
    jsutil_cpp = set([])

    for line in lines:
        m = re.search(alloc_fns_re, line)
        if m is None:
            continue

        fn = m.group(1)
        filename = m.group(2)
        linenum = m.group(3)
        if filename == 'jsutil.cpp':
            jsutil_cpp.add(fn)
        else:
            
            fail("'" + fn + "' present at " + filename + ':' + linenum)


    
    
    for fn in alloc_fns_unescaped:
        if fn not in jsutil_cpp:
            fail("'" + fn + "' isn't used as expected in jsutil.cpp")
        else:
            jsutil_cpp.remove(fn)

    
    if jsutil_cpp:
        fail('unexpected allocation fns used in jsutil.cpp: ' +
             ', '.join(jsutil_cpp))

    if has_failed:
        sys.exit(1)

    print('TEST-PASS | check_vanilla_allocations.py | ok')
    sys.exit(0)


if __name__ == '__main__':
    main()

