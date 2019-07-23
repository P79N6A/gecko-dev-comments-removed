







































import os
import sys

import unittest



def suite():
    
    try:
        me = __file__
    except NameError:
        me = sys.argv[0]
    me = os.path.abspath(me)
    files = os.listdir(os.path.dirname(me))
    suite = unittest.TestSuite()
    
    
    for file in files:
        base, ext = os.path.splitext(file)
        if ext=='.py' and os.path.basename(base).startswith("test_"):
            mod = __import__(base)
            if hasattr(mod, "suite"):
                test = mod.suite()
            else:
                test = unittest.defaultTestLoader.loadTestsFromModule(mod)
            suite.addTest(test)
    return suite

class CustomLoader(unittest.TestLoader):
    def loadTestsFromModule(self, module):
        return suite()

try:
    unittest.TestProgram(testLoader=CustomLoader())(argv=sys.argv)
finally:
    from xpcom import _xpcom
    _xpcom.NS_ShutdownXPCOM() 
    ni = _xpcom._GetInterfaceCount()
    ng = _xpcom._GetGatewayCount()
    if ni or ng:
        
        
        
        
        if ni == 6 and ng == 1:
            print "Sadly, there are 6/1 leaks, but these appear normal and benign"
        else:
            print "********* WARNING - Leaving with %d/%d objects alive" % (ni,ng)
    else:
        print "yay! Our leaks have all vanished!"
