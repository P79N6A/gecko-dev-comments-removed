








import argparse
import os
import sys

from io import BytesIO

from header import print_header
from typelib import write_typelib
from xpidl import IDLParser
from xpt import xpt_link

from mozbuild.util import FileAvoidWrite


def process(input_dir, cache_dir, header_dir, xpt_dir, deps_dir, module, stems):
    p = IDLParser(outputdir=cache_dir)

    xpts = {}
    deps = set()

    
    
    for imported in ('header', 'typelib', 'xpidl', 'xpt'):
        path = sys.modules[imported].__file__

        if path.endswith('.pyc'):
            path = path[0:-1]

        deps.add(path)

    for stem in stems:
        path = os.path.join(input_dir, '%s.idl' % stem)
        idl_data = open(path).read()

        idl = p.parse(idl_data, filename=path)
        idl.resolve([input_dir], p)

        header_path = os.path.join(header_dir, '%s.h' % stem)
        deps_path = os.path.join(deps_dir, '%s.pp' % stem)

        xpt = BytesIO()
        write_typelib(idl, xpt, path)
        xpt.seek(0)
        xpts[stem] = xpt

        deps |= set(dep.replace('\\', '/') for dep in idl.deps)

        with FileAvoidWrite(header_path) as fh:
            print_header(idl, fh, path)

    
    xpt_path = os.path.join(xpt_dir, '%s.xpt' % module)
    xpt_link(xpts.values()).write(xpt_path)

    deps_path = os.path.join(deps_dir, '%s.pp' % module)
    with FileAvoidWrite(deps_path) as fh:
        
        s_deps = sorted(deps)
        fh.write('%s: %s\n' % (xpt_path, ' '.join(s_deps)))
        for dep in s_deps:
            fh.write('%s:\n' % dep)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--cache-dir',
        help='Directory in which to find or write cached lexer data.')
    parser.add_argument('inputdir',
        help='Directory in which to find source .idl files.')
    parser.add_argument('headerdir',
        help='Directory in which to write header files.')
    parser.add_argument('xptdir',
        help='Directory in which to write xpt file.')
    parser.add_argument('depsdir',
        help='Directory in which to write dependency files.')
    parser.add_argument('module',
        help='Final module name to use for linked output xpt file.')
    parser.add_argument('idls', nargs='+',
        help='Source .idl file(s). Specified as stems only.')

    args = parser.parse_args()
    process(args.inputdir, args.cache_dir, args.headerdir, args.xptdir,
        args.depsdir, args.module, args.idls)
