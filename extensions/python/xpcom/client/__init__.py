




































import os
import new
import logging
from xpcom import xpt, COMException, nsError, logger


from xpcom._xpcom import IID_nsISupports, IID_nsIClassInfo, \
    IID_nsISupportsCString, IID_nsISupportsString, \
    IID_nsISupportsWeakReference, IID_nsIWeakReference, \
    XPTI_GetInterfaceInfoManager, GetComponentManager, NS_InvokeByIndex



_special_getattr_names = ["__del__", "__len__", "__nonzero__", "__eq__", "__neq__"]

_just_int_interfaces = ["nsISupportsPRInt32", "nsISupportsPRInt16", "nsISupportsPRUint32", "nsISupportsPRUint16", "nsISupportsPRUint8", "nsISupportsPRBool"]
_just_long_interfaces = ["nsISupportsPRInt64", "nsISupportsPRUint64"]
_just_float_interfaces = ["nsISupportsDouble", "nsISupportsFloat"]

_int_interfaces = _just_int_interfaces + _just_float_interfaces
_long_interfaces = _just_long_interfaces + _just_int_interfaces + _just_float_interfaces
_float_interfaces = _just_float_interfaces + _just_long_interfaces + _just_int_interfaces

method_template = """
def %s(self, %s):
    return NS_InvokeByIndex(self._comobj_, %d, (%s, (%s)))
"""
def _MakeMethodCode(method):
    
    param_no = 0
    param_decls = []
    param_flags = []
    param_names = []
    used_default = 0
    for param in method.params:
        param_no = param_no + 1
        param_name = "Param%d" % (param_no,)
        param_default = ""
        if not param.hidden_indicator and param.IsIn() and not param.IsDipper():
            if param.IsOut() or used_default: 
                param_default = " = None"
                used_default = 1 
            param_decls.append(param_name + param_default)
            param_names.append(param_name)

        type_repr = xpt.MakeReprForInvoke(param)
        param_flags.append( (param.param_flags,) +  type_repr )
    sep = ", "
    param_decls = sep.join(param_decls)
    if len(param_names)==1: 
        param_names = param_names[0] + ","
    else:
        param_names = sep.join(param_names)
    
    return method_template % (method.name, param_decls, method.method_index, tuple(param_flags), param_names)


interface_cache = {}

interface_method_cache = {}


contractid_info_cache = {}
have_shutdown = 0

def _shutdown():
    interface_cache.clear()
    interface_method_cache.clear()
    contractid_info_cache.clear()
    global have_shutdown
    have_shutdown = 1


def BuildMethod(method_info, iid):
    name = method_info.name
    try:
        return interface_method_cache[iid][name]
    except KeyError:
        pass
    
    assert not (method_info.IsSetter() or method_info.IsGetter()), "getters and setters should have been weeded out by now"
    method_code = _MakeMethodCode(method_info)
    
    
    


    codeObject = compile(method_code, "<XPCOMObject method '%s'>" % (name,), "exec")
    
    tempNameSpace = {}
    exec codeObject in globals(), tempNameSpace
    ret = tempNameSpace[name]
    if not interface_method_cache.has_key(iid):
        interface_method_cache[iid] = {}
    interface_method_cache[iid][name] = ret
    return ret

from xpcom.xpcom_consts import XPT_MD_GETTER, XPT_MD_SETTER, XPT_MD_NOTXPCOM, XPT_MD_CTOR, XPT_MD_HIDDEN
FLAGS_TO_IGNORE = XPT_MD_NOTXPCOM | XPT_MD_CTOR | XPT_MD_HIDDEN



def BuildInterfaceInfo(iid):
    assert not have_shutdown, "Can't build interface info after a shutdown"
    ret = interface_cache.get(iid, None)
    if ret is None:
        
        method_code_blocks = []
        getters = {}
        setters = {}
        method_infos = {}
        
        interface = xpt.Interface(iid)
        for m in interface.methods:
            flags = m.flags
            if flags & FLAGS_TO_IGNORE == 0:
                if flags & XPT_MD_SETTER:
                    param_flags = map(lambda x: (x.param_flags,) + xpt.MakeReprForInvoke(x), m.params)
                    setters[m.name] = m.method_index, param_flags
                elif flags & XPT_MD_GETTER:
                    param_flags = map(lambda x: (x.param_flags,) + xpt.MakeReprForInvoke(x), m.params)
                    getters[m.name] = m.method_index, param_flags
                else:
                    method_infos[m.name] = m

        
        constants = {}
        for c in interface.constants:
            constants[c.name] = c.value
        ret = method_infos, getters, setters, constants
        interface_cache[iid] = ret
    return ret

class _XPCOMBase:
    def __cmp__(self, other):
        try:
            other = other._comobj_
        except AttributeError:
            pass
        return cmp(self._comobj_, other)

    def __hash__(self):
        return hash(self._comobj_)

    
    def __eq__(self, other):
        try:
            other = other._comobj_
        except AttributeError:
            pass
        return self._comobj_ == other

    def __neq__(self, other):
        try:
            other = other._comobj_
        except AttributeError:
            pass
        return self._comobj_ != other

    
    def __str__(self):
        try:
            self._comobj_.QueryInterface(IID_nsISupportsCString, 0)
            return str(self._comobj_)
        except COMException:
            return self.__repr__()

    def __unicode__(self):
        try:
            prin = self._comobj_.QueryInterface(IID_nsISupportsString)
        except COMException:
            return unicode(str(self))
        return prin.data

    
    def _do_conversion(self, interface_names, cvt):
        iim = XPTI_GetInterfaceInfoManager()
        for interface_name in interface_names:
            iid = iim.GetInfoForName(interface_name).GetIID()
            try:
                prim = self._comobj_.QueryInterface(iid)
                return cvt(prim.data)
            except COMException:
                pass
        raise ValueError, "This object does not support automatic numeric conversion to this type"

    def __int__(self):
        return self._do_conversion(_int_interfaces, int)

    def __long__(self):
        return self._do_conversion(_long_interfaces, long)

    def __float__(self):
        return self._do_conversion(_float_interfaces, float)
    
class Component(_XPCOMBase):
    def __init__(self, ob, iid = IID_nsISupports):
        assert not hasattr(ob, "_comobj_"), "Should be a raw nsIWhatever, not a wrapped one"
        ob_name = None
        if not hasattr(ob, "IID"):
            ob_name = ob
            cm = GetComponentManager()
            ob = cm.createInstanceByContractID(ob)
            assert not hasattr(ob, "_comobj_"), "The created object should be a raw nsIWhatever, not a wrapped one"
        
        self.__dict__['_comobj_'] = ob
        
        self.__dict__['_interfaces_'] = {} 
        self.__dict__['_interface_names_'] = {} 
        self.__dict__['_interface_infos_'] = {} 
        self.__dict__['_name_to_interface_iid_'] = {}
        self.__dict__['_tried_classinfo_'] = 0

        if ob_name is None:
            ob_name = "<unknown>"
        self.__dict__['_object_name_'] = ob_name
        self.QueryInterface(iid)

    def _build_all_supported_interfaces_(self):
        
        
        assert not self._tried_classinfo_, "already tried to get the class info."
        self.__dict__['_tried_classinfo_'] = 1
        
        try:
            classinfo = self._comobj_.QueryInterface(IID_nsIClassInfo, 0)
        except COMException:
            classinfo = None
        if classinfo is not None:
            try:
                real_cid = classinfo.contractID
            except COMException:
                real_cid = None
            if real_cid:
                self.__dict__['_object_name_'] = real_cid
                contractid_info = contractid_info_cache.get(real_cid)
            else:
                contractid_info = None
            if contractid_info is None:
                try:
                    interface_infos = classinfo.getInterfaces()
                except COMException:
                    interface_infos = []
                for nominated_iid in interface_infos:
                    
                    if not self.__dict__['_interface_infos_'].has_key(nominated_iid):
                        
                        self.queryInterface(nominated_iid)
                if real_cid is not None:
                    contractid_info = {}
                    contractid_info['_name_to_interface_iid_'] = self.__dict__['_name_to_interface_iid_']
                    contractid_info['_interface_infos_'] = self.__dict__['_interface_infos_']
                    contractid_info_cache[real_cid] = contractid_info
            else:
                for key, val in contractid_info.items():
                    self.__dict__[key].update(val)

        self.__dict__['_com_classinfo_'] = classinfo

    def _remember_interface_info(self, iid):
        
        
        
        iis = self.__dict__['_interface_infos_']
        assert not iis.has_key(iid), "Already remembered this interface!"
        try:
            method_infos, getters, setters, constants = BuildInterfaceInfo(iid)
        except COMException, why:
            
            
            logger.info("Failed to build interface info for %s: %s", iid, why)
            
            iis[iid] = None
            return

        
        iis[iid] = method_infos, getters, setters, constants
        names = self.__dict__['_name_to_interface_iid_']
        for name in method_infos.keys(): names[name] = iid
        for name in getters.keys(): names[name] = iid
        for name in setters.keys(): names[name] = iid
        for name in constants.keys():  names[name] = iid

    def QueryInterface(self, iid):
        if self._interfaces_.has_key(iid):
            assert self._interface_names_.has_key(iid.name), "_interfaces_ has the key, but _interface_names_ does not!"
            return self
        
        if not self._interface_infos_.has_key(iid):
            self._remember_interface_info(iid)
        iface_info = self._interface_infos_[iid]
        if iface_info is None:
            
            
            
            
            return self._comobj_.QueryInterface(iid, 0)

        raw_iface = self._comobj_.QueryInterface(iid, 0)

        method_infos, getters, setters, constants = iface_info
        new_interface = _Interface(raw_iface, iid, method_infos,
                                   getters, setters, constants)
        self._interfaces_[iid] = new_interface
        self._interface_names_[iid.name] = new_interface
        
        
        
        return self

    queryInterface = QueryInterface 

    def __getattr__(self, attr):
        if attr in _special_getattr_names:
            raise AttributeError, attr
        
        interface = self.__dict__['_interface_names_'].get(attr, None)
        if interface is not None:
            return interface
        
        iid = self.__dict__['_name_to_interface_iid_'].get(attr, None)
        
        if iid is None and not self._tried_classinfo_:
            self._build_all_supported_interfaces_()
            iid = self.__dict__['_name_to_interface_iid_'].get(attr, None)
            
            
            interface = self.__dict__['_interface_names_'].get(attr, None)
            if interface is not None:
                return interface

        if iid is not None:
            interface = self.__dict__['_interfaces_'].get(iid, None)
            if interface is None:
                self.QueryInterface(iid)
                interface = self.__dict__['_interfaces_'][iid]
            return getattr(interface, attr)
        
        
        for interface in self.__dict__['_interfaces_'].values():
            try:
                ret = getattr(interface, attr)
                self.__dict__['_name_to_interface_iid_'][attr] = interface._iid_
                return ret
            except AttributeError:
                pass
        raise AttributeError, "XPCOM component '%s' has no attribute '%s'" % (self._object_name_, attr)
        
    def __setattr__(self, attr, val):
        iid = self._name_to_interface_iid_.get(attr, None)
        
        if iid is None and not self._tried_classinfo_:
            self._build_all_supported_interfaces_()
            iid = self.__dict__['_name_to_interface_iid_'].get(attr, None)
        if iid is not None:
            interface = self._interfaces_.get(iid, None)
            if interface is None:
                self.QueryInterface(iid)
                interface = self.__dict__['_interfaces_'][iid]
            setattr(interface, attr, val)
            return
        raise AttributeError, "XPCOM component '%s' has no attribute '%s'" % (self._object_name_, attr)

    def _get_classinfo_repr_(self):
        try:
            if not self._tried_classinfo_:
                self._build_all_supported_interfaces_()
            assert self._tried_classinfo_, "Should have tried the class info by now!"
        except COMException:
            
            
            
            self.__dict__['_tried_classinfo_'] = 0

        iface_names = self.__dict__['_interface_names_'].keys()
        try:
            iface_names.remove("nsISupports")
        except ValueError:
            pass
        iface_names.sort()
        
        iface_desc = "implementing %s" % (",".join(iface_names),)
        return iface_desc
        
    def __repr__(self):
        
        iface_desc = self._get_classinfo_repr_()
        return "<XPCOM component '%s' (%s)>" % (self._object_name_,iface_desc)

class _Interface(_XPCOMBase):
    def __init__(self, comobj, iid, method_infos, getters, setters, constants):
        self.__dict__['_comobj_'] = comobj
        self.__dict__['_iid_'] = iid
        self.__dict__['_property_getters_'] = getters
        self.__dict__['_property_setters_'] = setters
        self.__dict__['_method_infos_'] = method_infos 
        self.__dict__['_methods_'] = {} 
        self.__dict__['_object_name_'] = iid.name
        self.__dict__.update(constants)
        
        self.__dict__['_constant_names_'] = constants.keys()

    def __getattr__(self, attr):
        
        if attr in _special_getattr_names:
            raise AttributeError, attr

        ret = getattr(self.__dict__['_comobj_'], attr, None)
        if ret is not None:
            return ret
        
        unbound_method = self.__dict__['_methods_'].get(attr, None)
        if unbound_method is not None:
            return new.instancemethod(unbound_method, self, self.__class__)

        getters = self.__dict__['_property_getters_']
        info = getters.get(attr)
        if info is not None:
            method_index, param_infos = info
            if len(param_infos)!=1: 
                raise RuntimeError, "Can't get properties with this many args!"
            args = ( param_infos, () )
            return NS_InvokeByIndex(self._comobj_, method_index, args)

        
        
        method_info = self.__dict__['_method_infos_'].get(attr, None)
        if method_info is not None:
            unbound_method = BuildMethod(method_info, self._iid_)
            
            self.__dict__['_methods_'][attr] = unbound_method
            return new.instancemethod(unbound_method, self, self.__class__)

        raise AttributeError, "XPCOM component '%s' has no attribute '%s'" % (self._object_name_, attr)

    def __setattr__(self, attr, val):
        
        
        if self.__dict__.has_key(attr) and attr not in self.__dict__['_constant_names_']:
            self.__dict__[attr] = val
            return
        
        setters = self.__dict__['_property_setters_']
        info = setters.get(attr)
        if info is None:
            raise AttributeError, "XPCOM component '%s' can not set attribute '%s'" % (self._object_name_, attr)
        method_index, param_infos = info
        if len(param_infos)!=1: 
            raise RuntimeError, "Can't set properties with this many args!"
        real_param_infos = ( param_infos, (val,) )
        return NS_InvokeByIndex(self._comobj_, method_index, real_param_infos)

    def __repr__(self):
        return "<XPCOM interface '%s'>" % (self._object_name_,)




def MakeInterfaceResult(ob, iid):
    return Component(ob, iid)

class WeakReference:
    """A weak-reference object.  You construct a weak reference by passing
    any COM object you like.  If the object does not support weak
    refs, you will get a standard NS_NOINTERFACE exception.

    Once you have a weak-reference, you can "call" the object to get
    back a strong reference.  Eg:

    >>> some_ob = components.classes['...']
    >>> weak_ref = WeakReference(some_ob)
    >>> new_ob = weak_ref() # new_ob is effectively "some_ob" at this point
    >>> # EXCEPT: new_ob may be None if some_ob has already died - a
    >>> # weak reference does not keep the object alive (that is the point)

    You should never hold onto this resulting strong object for a long time,
    or else you defeat the purpose of the weak-reference.
    """
    def __init__(self, ob, iid = None):
        swr = Component(ob._comobj_, IID_nsISupportsWeakReference)
        self._comobj_ = Component(swr.GetWeakReference()._comobj_, IID_nsIWeakReference)
        if iid is None:
            try:
                iid = ob.IID
            except AttributeError:
                iid = IID_nsISupports
        self._iid_ = iid
    def __call__(self, iid = None):
        if iid is None: iid = self._iid_
        try:
            return Component(self._comobj_.QueryReferent(iid)._comobj_, iid)
        except COMException, details:
            if details.errno != nsError.NS_ERROR_NULL_POINTER:
                raise
            return None
