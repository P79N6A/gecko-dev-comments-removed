





































from xpcom import components, _xpcom
import xpcom.server, xpcom.client
from pyxpcom_test_tools import suite_from_functions, testmain

try:
    from sys import gettotalrefcount
except ImportError:
    
    gettotalrefcount = lambda: 0

num_alive = 0

class koTestSimple:
    _com_interfaces_ = [components.interfaces.nsIInputStream]
    def __init__(self):
        global num_alive
        num_alive += 1
    def __del__(self):
        global num_alive
        num_alive -= 1
    def close( self ):
        pass

def test():
    ob = xpcom.server.WrapObject( koTestSimple(), components.interfaces.nsIInputStream)

    if num_alive != 1: raise RuntimeError, "Eeek - there are %d objects alive" % (num_alive,)

    
    wr = xpcom.client.WeakReference(ob)
    if num_alive != 1: raise RuntimeError, "Eeek - there are %d objects alive" % (num_alive,)

    
    if wr() is None: raise RuntimeError, "Our weak-reference is returning None before it should!"
    wr().close()

    ob  = None 
    if num_alive != 0: raise RuntimeError, "Eeek - there are %d objects alive" % (num_alive,)
    if wr() is not None: raise RuntimeError, "Our weak-reference is not returning None when it should!"

    
    
    
    ob = xpcom.server.WrapObject( koTestSimple(), components.interfaces.nsISupports)
    if num_alive != 1: raise RuntimeError, "Eeek - there are %d objects alive" % (num_alive,)
    wr = xpcom.client.WeakReference(ob, components.interfaces.nsIInputStream)
    if num_alive != 1: raise RuntimeError, "Eeek - there are %d objects alive" % (num_alive,)
    wr() 
    ob  = None 
    if num_alive != 0: raise RuntimeError, "Eeek - there are %d objects alive" % (num_alive,)
    if wr() is not None: raise RuntimeError, "Our weak-reference is not returning None when it should!"

def test_refcount(num_loops=-1):
    
    if num_loops == -1: num_loops = 10
    for i in xrange(num_loops):
        test()

        if i==0:
            
            
            num_refs = gettotalrefcount()

    lost = gettotalrefcount() - num_refs
    
    
    
    if abs(lost)>2:
        print "*** Lost %d references" % (lost,)


def suite():
    return suite_from_functions(test_refcount)

if __name__=='__main__':
    testmain()
