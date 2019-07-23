 































__all__ = [ 'gencxx', 'genipdl', 'parse', 'typecheck' ]

import os, sys

from ipdl.cgen import IPDLCodeGen
from ipdl.lower import LowerToCxx
from ipdl.parser import Parser
from ipdl.type import TypeCheck

from ipdl.cxx.cgen import CxxCodeGen

def parse(specstring, filename='<stdin>'):
    return Parser().parse(specstring, filename)

def typecheck(ast, errout=sys.stderr):
    '''Returns True iff |ast| is well typed.  Print errors to |errout| if
    it is not.'''
    return TypeCheck().check(ast, errout)

def gencxx(ast, outdir):
    for hdr in LowerToCxx().lower(ast):
        path = os.path.join(outdir, hdr.filename)
        CxxCodeGen(outf=open(path, 'w')).cgen(hdr)

def genipdl(ast, outdir):
    return IPDLCodeGen().cgen(ast)
