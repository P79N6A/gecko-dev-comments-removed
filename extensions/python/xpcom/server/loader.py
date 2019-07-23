




































import xpcom
from xpcom import components, nsError
import xpcom.shutdown

import module

import os, types

def _has_good_attr(object, attr):
    
    return getattr(object, attr, None) is not None

def FindCOMComponents(py_module):
    
    comps = []
    for name, object in py_module.__dict__.items():
        if type(object)==types.ClassType and \
           _has_good_attr(object, "_com_interfaces_") and \
           _has_good_attr(object, "_reg_clsid_") and \
           _has_good_attr(object, "_reg_contractid_"):
            comps.append(object)
    return comps

def register_self(klass, compMgr, location, registryLocation, componentType):
    pcl = ModuleLoader
    from xpcom import _xpcom
    svc = _xpcom.GetServiceManager().getServiceByContractID("@mozilla.org/categorymanager;1", components.interfaces.nsICategoryManager)
    
    
    svc.addCategoryEntry("module-loader", pcl._reg_component_type_, pcl._reg_contractid_, 1, 1)




class ModuleLoader:
    _com_interfaces_ = components.interfaces.nsIModuleLoader
    _reg_clsid_ = "{945BFDA9-0226-485e-8AE3-9A2F68F6116A}" 
    _reg_contractid_ = "@mozilla.org/module-loader/python;1"
    _reg_desc_ = "Python module loader"
    
    
    _reg_registrar_ = (register_self,None)
    
    _reg_component_type_ = "application/x-python"

    def __init__(self):
        self.com_modules = {} 
        self.moduleFactory = module.Module
        xpcom.shutdown.register(self._on_shutdown)

    def _on_shutdown(self):
        self.com_modules.clear()

    def loadModule(self, aFile):
        return self._getCOMModuleForLocation(aFile)

    def _getCOMModuleForLocation(self, componentFile):
        fqn = componentFile.path
        if not fqn.endswith(".py"):
            raise xpcom.ServerException(nsError.NS_ERROR_INVALID_ARG)
        mod = self.com_modules.get(fqn)
        if mod is not None:
            return mod
        import ihooks, sys
        base_name = os.path.splitext(os.path.basename(fqn))[0]
        loader = ihooks.ModuleLoader()

        module_name_in_sys = "component:%s" % (base_name,)
        stuff = loader.find_module(base_name, [componentFile.parent.path])
        assert stuff is not None, "Couldn't find the module '%s'" % (base_name,)
        py_mod = loader.load_module( module_name_in_sys, stuff )

        
        comps = FindCOMComponents(py_mod)
        mod = self.moduleFactory(comps)
        
        self.com_modules[fqn] = mod
        return mod
