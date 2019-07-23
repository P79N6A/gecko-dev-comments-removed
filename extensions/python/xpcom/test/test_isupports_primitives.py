









































from xpcom import components
from xpcom import primitives
import xpcom.server, xpcom.client
from pyxpcom_test_tools import testmain
import unittest

class NoSupportsString:
    _com_interfaces_ = [components.interfaces.nsISupports]

class ImplicitSupportsString:
    _com_interfaces_ = [components.interfaces.nsISupports]
    def __str__(self):
        return "<MyImplicitStrObject>"

class ExplicitSupportsString:
    _com_interfaces_ = [components.interfaces.nsISupportsPrimitive,
                        components.interfaces.nsISupportsCString]
    type = components.interfaces.nsISupportsPrimitive.TYPE_CSTRING
    test_data = "<MyExplicitStrObject>"
    
    def __str__(self):
        return "<MyImplicitStrObject>"
    
    def get_data(self):
        return self.test_data
    def toString(self):
        return self.test_data

class ImplicitSupportsUnicode:
    _com_interfaces_ = [components.interfaces.nsISupports]
    test_data = u"Copyright \xa9 the initial developer"
    def __unicode__(self):
        
        return self.test_data

class ExplicitSupportsUnicode:
    _com_interfaces_ = [components.interfaces.nsISupportsPrimitive,
                        components.interfaces.nsISupportsString]
    type = components.interfaces.nsISupportsPrimitive.TYPE_STRING
    
    test_data = u"Copyright \xa9 the initial developer"
    def __unicode__(self):
        return self.test_data
    def get_data(self):
        return self.test_data

class ImplicitSupportsInt:
    _com_interfaces_ = [components.interfaces.nsISupports]
    def __int__(self):
        return 99

class ExplicitSupportsInt:
    _com_interfaces_ = [components.interfaces.nsISupportsPrimitive,
                        components.interfaces.nsISupportsPRInt32]
    type = components.interfaces.nsISupportsPrimitive.TYPE_PRINT32
    def get_data(self):
        return 99

class ImplicitSupportsLong:
    _com_interfaces_ = [components.interfaces.nsISupports]
    def __long__(self):
        return 99L

class ExplicitSupportsLong:
    _com_interfaces_ = [components.interfaces.nsISupportsPrimitive,
                        components.interfaces.nsISupportsPRInt64]
    type = components.interfaces.nsISupportsPrimitive.TYPE_PRINT64
    def get_data(self):
        return 99

class ExplicitSupportsFloat:
    _com_interfaces_ = [components.interfaces.nsISupportsPrimitive,
                        components.interfaces.nsISupportsDouble]
    type = components.interfaces.nsISupportsPrimitive.TYPE_DOUBLE
    def get_data(self):
        return 99.99

class ImplicitSupportsFloat:
    _com_interfaces_ = [components.interfaces.nsISupports]
    def __float__(self):
        return 99.99

class PrimitivesTestCase(unittest.TestCase):
    def testNoSupports(self):
        ob = xpcom.server.WrapObject( NoSupportsString(), components.interfaces.nsISupports)
        if not str(ob).startswith("<XPCOM "):
            raise RuntimeError, "Wrong str() value: %s" % (ob,)

    def testImplicitString(self):
        ob = xpcom.server.WrapObject( ImplicitSupportsString(), components.interfaces.nsISupports)
        self.failUnlessEqual(str(ob), "<MyImplicitStrObject>")

    def testExplicitString(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsString(), components.interfaces.nsISupports)
        self.failUnlessEqual(str(ob), "<MyExplicitStrObject>")

    def testImplicitUnicode(self):
        ob = xpcom.server.WrapObject( ImplicitSupportsUnicode(), components.interfaces.nsISupports)
        self.failUnlessEqual(unicode(ob), ImplicitSupportsUnicode.test_data)

    def testExplicitUnicode(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsUnicode(), components.interfaces.nsISupports)
        self.failUnlessEqual(unicode(ob), ExplicitSupportsUnicode.test_data)

    def testConvertInt(self):
        
        ob = xpcom.server.WrapObject( ExplicitSupportsString(), components.interfaces.nsISupports)
        self.failUnlessRaises( ValueError, int, ob)

    def testExplicitInt(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsInt(), components.interfaces.nsISupports)
        self.failUnlessAlmostEqual(float(ob), 99.0)
        self.failUnlessEqual(int(ob), 99)

    def testImplicitInt(self):
        ob = xpcom.server.WrapObject( ImplicitSupportsInt(), components.interfaces.nsISupports)
        self.failUnlessAlmostEqual(float(ob), 99.0)
        self.failUnlessEqual(int(ob), 99)

    def testExplicitLong(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsLong(), components.interfaces.nsISupports)
        if long(ob) != 99 or not repr(long(ob)).endswith("L"):
            raise RuntimeError, "Bad value: %s" % (repr(long(ob)),)
        self.failUnlessAlmostEqual(float(ob), 99.0)

    def testImplicitLong(self):
        ob = xpcom.server.WrapObject( ImplicitSupportsLong(), components.interfaces.nsISupports)
        if long(ob) != 99 or not repr(long(ob)).endswith("L"):
            raise RuntimeError, "Bad value: %s" % (repr(long(ob)),)
        self.failUnlessAlmostEqual(float(ob), 99.0)

    def testExplicitFloat(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsFloat(), components.interfaces.nsISupports)
        self.failUnlessEqual(float(ob), 99.99)
        self.failUnlessEqual(int(ob), 99)

    def testImplicitFloat(self):
        ob = xpcom.server.WrapObject( ImplicitSupportsFloat(), components.interfaces.nsISupports)
        self.failUnlessEqual(float(ob), 99.99)
        self.failUnlessEqual(int(ob), 99)

class PrimitivesModuleTestCase(unittest.TestCase):
    def testExplicitString(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsString(), components.interfaces.nsISupports)
        self.failUnlessEqual(primitives.GetPrimitive(ob), "<MyExplicitStrObject>")

    def testExplicitUnicode(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsUnicode(), components.interfaces.nsISupports)
        self.failUnlessEqual(primitives.GetPrimitive(ob), ExplicitSupportsUnicode.test_data)
        self.failUnlessEqual(type(primitives.GetPrimitive(ob)), unicode)

    def testExplicitInt(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsInt(), components.interfaces.nsISupports)
        self.failUnlessEqual(primitives.GetPrimitive(ob), 99)

    def testExplicitLong(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsLong(), components.interfaces.nsISupports)
        self.failUnlessEqual(primitives.GetPrimitive(ob), 99)

    def testExplicitFloat(self):
        ob = xpcom.server.WrapObject( ExplicitSupportsFloat(), components.interfaces.nsISupports)
        self.failUnlessEqual(primitives.GetPrimitive(ob), 99.99)

if __name__=='__main__':
    testmain()
