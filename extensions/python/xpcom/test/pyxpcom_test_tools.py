
from xpcom import _xpcom
import unittest





try:
    import win32api
    import win32pdh
    import win32pdhutil
    have_pdh = 1
except ImportError:
    have_pdh = 0






def FindMyCounter():
    pid_me = win32api.GetCurrentProcessId()

    object = "Process"
    items, instances = win32pdh.EnumObjectItems(None,None,object, -1)
    for instance in instances:
        
        counter = "ID Process"
        format = win32pdh.PDH_FMT_LONG

        hq = win32pdh.OpenQuery()
        path = win32pdh.MakeCounterPath( (None,object,instance, None, -1,"ID Process") )
        hc1 = win32pdh.AddCounter(hq, path)
        path = win32pdh.MakeCounterPath( (None,object,instance, None, -1,"Working Set") )
        hc2 = win32pdh.AddCounter(hq, path)
        win32pdh.CollectQueryData(hq)
        type, pid = win32pdh.GetFormattedCounterValue(hc1, format)
        if pid==pid_me:
            win32pdh.RemoveCounter(hc1) 
            return hq, hc2
        
        win32pdh.RemoveCounter(hc1)
        win32pdh.RemoveCounter(hc2)
        win32pdh.CloseQuery(hq)
    else:
        raise RuntimeError, "Can't find myself!?"

def CloseCounter(hq, hc):
    win32pdh.RemoveCounter(hc)
    win32pdh.CloseQuery(hq)

def GetCounterValue(hq, hc):
    win32pdh.CollectQueryData(hq)
    format = win32pdh.PDH_FMT_LONG
    type, val = win32pdh.GetFormattedCounterValue(hc, format)
    return val

g_pdh_data = None

def pdh_getmemusage():
    global g_pdh_data
    if g_pdh_data is None:
        hq, hc = FindMyCounter()
        g_pdh_data = hq, hc
    hq, hc = g_pdh_data
    return GetCounterValue(hq, hc)


if have_pdh:
    getmemusage = pdh_getmemusage
else:
    def getmemusage():
        return 0


class TestLoader(unittest.TestLoader):
    def loadTestsFromTestCase(self, testCaseClass):
        """Return a suite of all tests cases contained in testCaseClass"""
        leak_tests = []
        for name in self.getTestCaseNames(testCaseClass):
            real_test = testCaseClass(name)
            leak_test = self._getTestWrapper(real_test)
            leak_tests.append(leak_test)
        return self.suiteClass(leak_tests)
    def _getTestWrapper(self, test):
        
        return test
    def loadTestsFromModule(self, mod):
        if hasattr(mod, "suite"):
            ret = mod.suite()
        else:
            ret = unittest.TestLoader.loadTestsFromModule(self, mod)
        assert ret.countTestCases() > 0, "No tests in %r" % (mod,)
        return ret
    def loadTestsFromName(self, name, module=None):
        test = unittest.TestLoader.loadTestsFromName(self, name, module)
        if isinstance(test, unittest.TestSuite):
            pass 
        elif isinstance(test, unittest.TestCase):
            test = self._getTestWrapper(test)
        else:
            print "XXX - what is", test
        return test


TestCase = unittest.TestCase

def suite_from_functions(*funcs):
    suite = unittest.TestSuite()
    for func in funcs:
        suite.addTest(unittest.FunctionTestCase(func))
    return suite

def testmain(*args, **kw):
    new_kw = kw.copy()
    if not new_kw.has_key('testLoader'):
        new_kw['testLoader'] = TestLoader()
    try:
        unittest.main(*args, **new_kw)
    finally:
        _xpcom.NS_ShutdownXPCOM()
        ni = _xpcom._GetInterfaceCount()
        ng = _xpcom._GetGatewayCount()
        if ni or ng:
            print "********* WARNING - Leaving with %d/%d objects alive" % (ni,ng)
