 































__all__ = [ 'gencxx', 'genipdl', 'parse', 'typecheck', 'writeifmodified' ]

import os, sys
from cStringIO import StringIO

from ipdl.cgen import IPDLCodeGen
from ipdl.lower import LowerToCxx
from ipdl.parser import Parser
from ipdl.type import TypeCheck

from ipdl.cxx.cgen import CxxCodeGen

def parse(specstring, filename='/stdin', includedirs=[ ], errout=sys.stderr):
    '''Return an IPDL AST if parsing was successful.  Print errors to |errout|
    if it is not.'''
    return Parser().parse(specstring, os.path.abspath(filename), includedirs, errout)

def typecheck(ast, errout=sys.stderr):
    '''Return True iff |ast| is well typed.  Print errors to |errout| if
    it is not.'''
    return TypeCheck().check(ast, errout)

def gencxx(ast, outdir):
    for hdr in LowerToCxx().lower(ast):
        file = os.path.join(outdir,
                            *([ns.namespace for ns in ast.protocol.namespaces] + [hdr.filename]))

        tempfile = StringIO()
        CxxCodeGen(tempfile).cgen(hdr)
        writeifmodified(tempfile.getvalue(), file)

def genipdl(ast, outdir):
    return IPDLCodeGen().cgen(ast)

def writeifmodified(contents, file):
    dir = os.path.dirname(file)
    os.path.exists(dir) or os.makedirs(dir)

    oldcontents = None
    if os.path.exists(file):
        fd = open(file, 'rb')
        oldcontents = fd.read()
        fd.close()
    if oldcontents != contents:
        fd = open(file, 'wb')
        fd.write(contents)
        fd.close()
