





































from xpcom import components, nsError, ServerException, COMException, logger
from xpcom.server import WrapObject
from pyxpcom_test_tools import testmain

import unittest
import logging

class PythonFailingComponent:
    
    _com_interfaces_ = components.interfaces.nsIPythonTestInterfaceExtra

    def do_boolean(self, p1, p2):
        
        raise ServerException()

    def do_octet(self, p1, p2):
        
        raise ServerException(nsError.NS_ERROR_NOT_IMPLEMENTED)

    def do_short(self, p1, p2):
        
        raise COMException(nsError.NS_ERROR_NOT_IMPLEMENTED)

    def do_unsigned_short(self, p1, p2):
        
        raise "Foo"

    def do_long(self, p1, p2):
        
        raise ServerException

    def do_unsigned_long(self, p1, p2):
        
        raise ServerException, nsError.NS_ERROR_NOT_IMPLEMENTED

    def do_long_long(self, p1, p2):
        
        raise ServerException, (nsError.NS_ERROR_NOT_IMPLEMENTED, "testing")

    def do_unsigned_long_long(self, p1, p2):
        
        raise ServerException, "A bad exception param"

class TestHandler(logging.Handler):
    def __init__(self, level=logging.ERROR): 
        logging.Handler.__init__(self, level)
        self.records = []
    
    def reset(self):
        self.records = []

    def handle(self, record):
        self.records.append(record)

class ExceptionTests(unittest.TestCase):

    def _testit(self, expected_errno, num_tracebacks, func, *args):

        
        old_handlers = logger.handlers
        test_handler = TestHandler()
        logger.handlers = [test_handler]

        try:
            try:
                apply(func, args)
            except COMException, what:
                if what.errno != expected_errno:
                    raise
        finally:
            logger.handlers = old_handlers
        self.failUnlessEqual(num_tracebacks, len(test_handler.records))

    def testEmAll(self):
        ob = WrapObject( PythonFailingComponent(), components.interfaces.nsIPythonTestInterfaceExtra)
        self._testit(nsError.NS_ERROR_FAILURE, 0, ob.do_boolean, 0, 0)
        self._testit(nsError.NS_ERROR_NOT_IMPLEMENTED, 0, ob.do_octet, 0, 0)
        self._testit(nsError.NS_ERROR_FAILURE, 1, ob.do_short, 0, 0)
        self._testit(nsError.NS_ERROR_FAILURE, 1, ob.do_unsigned_short, 0, 0)
        self._testit(nsError.NS_ERROR_FAILURE, 0, ob.do_long, 0, 0)
        self._testit(nsError.NS_ERROR_NOT_IMPLEMENTED, 0, ob.do_unsigned_long, 0, 0)
        self._testit(nsError.NS_ERROR_NOT_IMPLEMENTED, 0, ob.do_long_long, 0, 0)
        self._testit(nsError.NS_ERROR_FAILURE, 1, ob.do_unsigned_long_long, 0, 0)

if __name__=='__main__':
    testmain()
