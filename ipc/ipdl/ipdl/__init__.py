 































__all__ = [ 'gencxx', 'genipdl', 'parse', 'typecheck', 'writetofile' ]

import os, sys, errno
from cStringIO import StringIO

from ipdl.cgen import IPDLCodeGen
from ipdl.mgen import DependGen
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


def gencxx(ipdlfilename, ast, outheadersdir, outcppdir):
    headers, cpps = LowerToCxx().lower(ast)

    def resolveHeader(hdr):
        return [
            hdr, 
            os.path.join(
                outheadersdir,
                *([ns.name for ns in ast.protocol.namespaces] + [hdr.name]))
        ]
    def resolveCpp(cpp):
        return [ cpp, os.path.join(outcppdir, cpp.name) ]

    for ast, filename in ([ resolveHeader(hdr) for hdr in headers ]
                          + [ resolveCpp(cpp) for cpp in cpps ]):
        tempfile = StringIO()
        CxxCodeGen(tempfile).cgen(ast)
        writetofile(tempfile.getvalue(), filename)


def genipdl(ast, outdir):
    return IPDLCodeGen().cgen(ast)


def genm(ast, dir, filename):
    tempfile = StringIO()
    DependGen(tempfile).mgen(ast)
    filename = dir + "/" + os.path.basename(filename) + ".depends"
    writetofile(tempfile.getvalue(), filename)


def writetofile(contents, file):
    dir = os.path.dirname(file)

    
    
    try:
        os.makedirs(dir)
    except OSError, ex:
        if ex.errno != errno.EEXIST:
            raise ex
        

    fd = open(file, 'wb')
    fd.write(contents)
    fd.close()
