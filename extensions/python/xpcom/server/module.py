



































from xpcom import components
from xpcom import ServerException, Exception
from xpcom import nsError

import factory

import types
import os

class Module:
    _com_interfaces_ = components.interfaces.nsIModule
    def __init__(self, comps):
        
        c = self.components = {}
        for klass in comps:
            c[components.ID(klass._reg_clsid_)] = klass
        self.klassFactory = factory.Factory

    def getClassObject(self, compMgr, clsid, iid):
        
        try:
            klass = self.components[clsid]
        except KeyError:
            raise ServerException(nsError.NS_ERROR_FACTORY_NOT_REGISTERED)
        
        
        return self.klassFactory(klass)

    def registerSelf(self, compMgr, location, loaderStr, componentType):
        
        fname = os.path.basename(location.path)
        for klass in self.components.values():
            reg_contractid = klass._reg_contractid_
            try:
                print "Registering '%s' (%s)" % (reg_contractid, fname)
            except IOError:
                pass 
            reg_desc = getattr(klass, "_reg_desc_", reg_contractid)
            compMgr = compMgr.queryInterface(components.interfaces.nsIComponentRegistrar)
            compMgr.registerFactoryLocation(klass._reg_clsid_,
                                              reg_desc,
                                              reg_contractid,
                                              location,
                                              loaderStr,
                                              componentType)

            
            extra_func = getattr(klass, "_reg_registrar_", (None,None))[0]
            if extra_func is not None:
                extra_func(klass, compMgr, location, loaderStr, componentType)

    def unregisterSelf(self, compMgr, location, loaderStr):
        
        for klass in self.components.values():
            ok = 1
            try:
                compMgr.unregisterComponentSpec(klass._reg_clsid_, location)
            except Exception:
                ok = 0
            
            extra_func = getattr(klass, "_reg_registrar_", (None,None))[1]
            if extra_func is not None:
                try:
                    extra_func(klass, compMgr, location, loaderStr)
                except Exception:
                    ok = 0
            try:
                if ok:
                    print "Successfully unregistered", klass.__name__
                else:
                    print "Unregistration of", klass.__name__, "failed (not previously registered?)"
            except IOError:
                pass

    def canUnload(self, compMgr):
        
        return 0 
