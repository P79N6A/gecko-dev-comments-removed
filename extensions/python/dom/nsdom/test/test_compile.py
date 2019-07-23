




































import sys
import unittest
import new
from nsdom.domcompile import compile, compile_function

class TestCompile(unittest.TestCase):
    def testSyntaxError(self):
        globs = {}
        try:
            co = compile(u"\ndef class()", "foo.py", lineno=100)
            raise AssertionError, "not reached!"
        except SyntaxError, why:
            self.failUnlessEqual(why.lineno, 102)
            self.failUnlessEqual(why.filename, "foo.py")

    def testFilenameAndOffset(self):
        src = u"a=1\nraise RuntimeError"
        globs = {}
        co = compile_function(src, "foo.py", "func_name", ["x", "y", "z"], 
                              lineno=100)
        exec co in globs
        f = globs['func_name']
        
        
        globs = {'g':'call_globs', '__debug__': __debug__,
                 "__builtins__": globs["__builtins__"]}
        f = new.function(f.func_code, globs, f.func_name)
        try:
            f(1,2,3)
            raise AssertionError, "not reached!"
        except RuntimeError:
            
            
            tb = sys.exc_info()[2].tb_next
            self.failUnlessEqual(tb.tb_frame.f_lineno, 102)
            self.failUnlessEqual(tb.tb_frame.f_code.co_filename, "foo.py")

    def testFunction(self):
        
        src = u"assert g=='call_globs'\nreturn 'wow'"
   
        globs = {'g':'compile_globs'}
        co = compile_function(src, "foo.py", "func_name", ["x", "y", "z"], 
                              lineno=100)
        exec co in globs
        f = globs['func_name']
        
        globs = {'g':'call_globs', '__debug__': __debug__}
        f = new.function(f.func_code, globs, f.func_name)
        self.failUnlessEqual(f(1,2,3), 'wow')

    def testFunctionArgs(self):
        
        src = u"assert a==1\nassert b=='b'\nassert c is None\nreturn 'yes'"
   
        defs = (1, "b", None)
        co = compile_function(src, "foo.py", "func_name",
                              ["a", "b", "c"],
                              defs,
                              lineno=100)
        globs = {}
        exec co in globs
        f = globs['func_name']
        
        globs = {'__debug__': __debug__}
        f = new.function(f.func_code, globs, f.func_name, defs)
        self.failUnlessEqual(f(), 'yes')

    def testSimple(self):
        globs = {}
        
        co = compile(u"a=1", "foo.py")
        exec co in globs
        self.failUnlessEqual(globs['a'], 1)

    def testTrailingWhitespace(self):
        globs = {}
        
        co = compile(u"a=1\n  ", "foo.py")
        exec co in globs
        self.failUnlessEqual(globs['a'], 1)

    def _testNewlines(self, sep):
        src = u"a=1" + sep + "b=2"
        globs = {}
        co = compile(src, "foo.py")
        exec co in globs
        self.failUnlessEqual(globs['a'], 1)
        self.failUnlessEqual(globs['b'], 2)
        
    def testMacNewlines(self):
        self._testNewlines("\r")

    def testWindowsNewlines(self):
        self._testNewlines("\r\n")

    def testUnixNewlines(self):
        self._testNewlines("\n")

if __name__=='__main__':
    unittest.main()
