




































"""Tests the "xpcom.components" object.
"""

import xpcom.components
from pyxpcom_test_tools import suite_from_functions, testmain

if not __debug__:
    raise RuntimeError, "This test uses assert, so must be run in debug mode"

def test_interfaces():
    "Test the xpcom.components.interfaces object"

    iid = xpcom.components.interfaces.nsISupports
    assert iid == xpcom._xpcom.IID_nsISupports, "Got the wrong IID!"
    iid = xpcom.components.interfaces['nsISupports']
    assert iid == xpcom._xpcom.IID_nsISupports, "Got the wrong IID!"
    
    
    num_fetched = num_nsisupports = 0
    for name, iid in xpcom.components.interfaces.items():
        num_fetched = num_fetched + 1
        if name == "nsISupports":
            num_nsisupports = num_nsisupports + 1
            assert iid == xpcom._xpcom.IID_nsISupports, "Got the wrong IID!"
        assert xpcom.components.interfaces[name] == iid
    
    assert len(xpcom.components.interfaces.keys()) == len(xpcom.components.interfaces.values()) == \
               len(xpcom.components.interfaces.items()) == len(xpcom.components.interfaces) == \
               num_fetched, "The collection lengths were wrong"
    assert num_nsisupports == 1, "Didn't find exactly 1 nsiSupports!"

def test_classes():
    
    prog_id = "@mozilla.org/supports-array;1"
    clsid = xpcom.components.ID("{bda17d50-0d6b-11d3-9331-00104ba0fd40}")

    
    klass = xpcom.components.classes[prog_id]
    instance = klass.createInstance()
    
    
    num_fetched = num_mine = 0
    for name, klass in xpcom.components.classes.items():
        num_fetched = num_fetched + 1
        if name == prog_id:
            assert klass.clsid == clsid, "Eeek - didn't get the correct IID - got %s" %klass.clsid
            num_mine = num_mine + 1



    
    if len(xpcom.components.classes.keys()) == len(xpcom.components.classes.values()) == \
               len(xpcom.components.classes.items()) == len(xpcom.components.classes) == \
               num_fetched:
        pass
    else:
        raise RuntimeError, "The collection lengths were wrong"
    if num_fetched <= 0:
        raise RuntimeError, "Didn't get any classes!!!"
    if num_mine != 1:
        raise RuntimeError, "Didn't find exactly 1 of my contractid! (%d)" % (num_mine,)
    
def test_id():
    id = xpcom.components.ID(str(xpcom._xpcom.IID_nsISupports))
    assert id == xpcom._xpcom.IID_nsISupports
    

def suite():
    return suite_from_functions(test_interfaces, test_classes, test_id)

if __name__=='__main__':
    testmain()
