































import sys

from ipdl.cgen import CodePrinter
from ipdl.cxx.ast import TypeArray, Visitor

class DependGen(CodePrinter, Visitor):
    def __init__(self, outf=sys.stdout, indentCols=4):
        CodePrinter.__init__(self, outf, indentCols)

    def mgen(self, cxxfile):
        cxxfile.accept(self)

    def visitTranslationUnit(self, tu):
        self.write(tu.filename)
        self.write(": ")

        for pinc in tu.protocolIncludes:
            self.write(pinc.file)
            self.write(" ")

        self.println();

