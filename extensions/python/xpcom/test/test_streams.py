




































import xpcom
from xpcom import _xpcom, components, COMException, ServerException, nsError
from StringIO import StringIO
import unittest
from pyxpcom_test_tools import testmain

test_data = "abcdefeghijklmnopqrstuvwxyz"

class koTestSimpleStream:
    _com_interfaces_ = [components.interfaces.nsIInputStream]
    

    def __init__(self):
        self.data=StringIO(test_data)
        self._non_blocking = False

    def close( self ):
        pass

    def available( self ):
        return self.data.len-self.data.pos

    def readStr( self, amount):
        return self.data.read(amount)

    read=readStr

    def get_observer( self ):
        raise ServerException(nsError.NS_ERROR_NOT_IMPLEMENTED)

    def set_observer( self, param0 ):
        raise ServerException(nsError.NS_ERROR_NOT_IMPLEMENTED)

    def isNonBlocking(self):
        return self._non_blocking

def get_test_input():
    
    
    
    import xpcom.server, xpcom.client
    ob = xpcom.server.WrapObject( koTestSimpleStream(), _xpcom.IID_nsISupports)
    ob = xpcom.client.Component(ob._comobj_, components.interfaces.nsIInputStream)
    return ob

class StreamTests(unittest.TestCase):
    def testInput(self):
        self.do_test_input( get_test_input() )

    def do_test_input(self, myStream):
        self.failUnlessEqual(str(myStream.read(5)), test_data[:5])
        self.failUnlessEqual(str(myStream.read(0)), '')
        self.failUnlessEqual(str(myStream.read(5)), test_data[5:10])
        self.failUnlessEqual(str(myStream.read(-1)), test_data[10:])
        self.failIf(myStream.isNonBlocking(), "Expected default to be blocking")
        

if __name__=='__main__':
    testmain()
