




































import sys
from xpcom import xpcom_consts, _xpcom, client, nsError, logger
from xpcom import ServerException, COMException
import xpcom
import xpcom.server
import operator
import types
import logging


IID_nsISupports = _xpcom.IID_nsISupports
IID_nsIVariant = _xpcom.IID_nsIVariant
XPT_MD_IS_GETTER = xpcom_consts.XPT_MD_IS_GETTER
XPT_MD_IS_SETTER = xpcom_consts.XPT_MD_IS_SETTER

VARIANT_INT_TYPES = xpcom_consts.VTYPE_INT8, xpcom_consts.VTYPE_INT16, xpcom_consts.VTYPE_INT32, \
                    xpcom_consts.VTYPE_UINT8, xpcom_consts.VTYPE_UINT16, xpcom_consts.VTYPE_INT32
VARIANT_LONG_TYPES = xpcom_consts.VTYPE_INT64, xpcom_consts.VTYPE_UINT64
VARIANT_FLOAT_TYPES = xpcom_consts.VTYPE_FLOAT, xpcom_consts.VTYPE_DOUBLE
VARIANT_STRING_TYPES = xpcom_consts.VTYPE_CHAR, xpcom_consts.VTYPE_CHAR_STR, xpcom_consts.VTYPE_STRING_SIZE_IS, \
                       xpcom_consts.VTYPE_CSTRING
VARIANT_UNICODE_TYPES = xpcom_consts.VTYPE_WCHAR, xpcom_consts.VTYPE_DOMSTRING, xpcom_consts.VTYPE_WSTRING_SIZE_IS, \
                        xpcom_consts.VTYPE_ASTRING 

_supports_primitives_map_ = {} 

_interface_sequence_types_ = types.TupleType, types.ListType
_string_types_ = types.StringType, types.UnicodeType
XPTI_GetInterfaceInfoManager = _xpcom.XPTI_GetInterfaceInfoManager

def _GetNominatedInterfaces(obj):
    ret = getattr(obj, "_com_interfaces_", None)
    if ret is None: return None
    
    if type(ret) not in _interface_sequence_types_:
        ret = [ret]
    real_ret = []
    
    iim = XPTI_GetInterfaceInfoManager()
    for interface in ret:
        
        interface_info = None
        if type(interface) in _string_types_:
            try:
                interface_info = iim.GetInfoForName(interface)
            except COMException:
                pass
        if interface_info is None:
            
            interface_info = iim.GetInfoForIID(interface)
        real_ret.append(interface_info.GetIID())
        parent = interface_info.GetParent()
        while parent is not None:
            parent_iid = parent.GetIID()
            if parent_iid == IID_nsISupports:
                break
            real_ret.append(parent_iid)
            parent = parent.GetParent()
    return real_ret





class_info_cache = {}

def GetClassInfoForObject(ob):
    if xpcom.server.tracer_unwrap is not None:
        ob = xpcom.server.tracer_unwrap(ob)
    klass = ob.__class__
    ci = class_info_cache.get(klass)
    if ci is None:
        ci = DefaultClassInfo(klass)
        ci = xpcom.server.WrapObject(ci, _xpcom.IID_nsIClassInfo, bWrapClient = 0)
        class_info_cache[klass] = ci
    return ci

class DefaultClassInfo:
    _com_interfaces_ = _xpcom.IID_nsIClassInfo
    def __init__(self, klass):
        self.klass = klass
        self.contractID = getattr(klass, "_reg_contractid_", None)
        self.classDescription = getattr(klass, "_reg_desc_", None)
        self.classID = getattr(klass, "_reg_clsid_", None)
        self.implementationLanguage = 3 
        self.flags = 0 
        self.interfaces = None

    def get_classID(self):
        if self.classID is None:
            raise ServerException(nsError.NS_ERROR_NOT_IMPLEMENTED, "Class '%r' has no class ID" % (self.klass,))
        return self.classID

    def getInterfaces(self):
        if self.interfaces is None:
            self.interfaces = _GetNominatedInterfaces(self.klass)
        return self.interfaces

    def getHelperForLanguage(self, language):
        return None 

class DefaultPolicy:
    def __init__(self, instance, iid):
        self._obj_ = instance
        self._nominated_interfaces_ = ni = _GetNominatedInterfaces(instance)
        self._iid_ = iid
        if ni is None:
            raise ValueError, "The object '%r' can not be used as a COM object" % (instance,)
        
        
        
        
        if __debug__:
            if iid != IID_nsISupports and iid not in ni:
                
                delegate_qi = getattr(instance, "_query_interface_", None)
                
                
                if delegate_qi is None or not delegate_qi(iid):
                    raise ServerException(nsError.NS_ERROR_NO_INTERFACE)
        
        self._interface_info_ = None
        self._interface_iid_map_ = {} 

    def _QueryInterface_(self, com_object, iid):
        
        
        if iid in self._nominated_interfaces_:
            
            
            
            
            
            
            
            
            return xpcom.server.WrapObject(self._obj_, iid, bWrapClient = 0)

        
        if iid == _xpcom.IID_nsIClassInfo:
            return GetClassInfoForObject(self._obj_)

        
        
        delegate = getattr(self._obj_, "_query_interface_", None)
        if delegate is not None:
            
            
            
            
            
            return delegate(iid)
        
        if not _supports_primitives_map_:
            iim = _xpcom.XPTI_GetInterfaceInfoManager()
            for (iid_name, attr, cvt) in _supports_primitives_data_:
                special_iid = iim.GetInfoForName(iid_name).GetIID()
                _supports_primitives_map_[special_iid] = (attr, cvt)
        attr, cvt = _supports_primitives_map_.get(iid, (None,None))
        if attr is not None and hasattr(self._obj_, attr):
            return xpcom.server.WrapObject(SupportsPrimitive(iid, self._obj_, attr, cvt), iid, bWrapClient = 0)
        
        return None 

    def _MakeInterfaceParam_(self, interface, iid, method_index, mi, param_index):
        
        
        if iid is None:
            
            if self._interface_info_ is None:
                import xpcom.xpt
                self._interface_info_ = xpcom.xpt.Interface( self._iid_ )
            iid = self._interface_iid_map_.get( (method_index, param_index))
            if iid is None:
                iid = self._interface_info_.GetIIDForParam(method_index, param_index)
                self._interface_iid_map_[(method_index, param_index)] = iid
        
        if iid == IID_nsIVariant:
            interface = interface.QueryInterface(iid)
            dt = interface.dataType
            if dt in VARIANT_INT_TYPES:
                return interface.getAsInt32()
            if dt in VARIANT_LONG_TYPES:
                return interface.getAsInt64()
            if dt in VARIANT_FLOAT_TYPES:
                return interface.getAsFloat()
            if dt in VARIANT_STRING_TYPES:
                return interface.getAsStringWithSize()
            if dt in VARIANT_UNICODE_TYPES:
                return interface.getAsWStringWithSize()
            if dt == xpcom_consts.VTYPE_BOOL:
                return interface.getAsBool()
            if dt == xpcom_consts.VTYPE_INTERFACE:
                return interface.getAsISupports()
            if dt == xpcom_consts.VTYPE_INTERFACE_IS:
                return interface.getAsInterface()
            if dt == xpcom_consts.VTYPE_EMPTY or dt == xpcom_consts.VTYPE_VOID:
                return None
            if dt == xpcom_consts.VTYPE_ARRAY:
                return interface.getAsArray()
            if dt == xpcom_consts.VTYPE_EMPTY_ARRAY:
                return []
            if dt == xpcom_consts.VTYPE_ID:
                return interface.getAsID()
            
            logger.warning("Warning: nsIVariant type %d not supported - returning a string", dt)
            try:
                return interface.getAsString()
            except COMException:
                logger.exception("Error: failed to get Variant as a string - returning variant object")
                return interface
            
        return client.Component(interface, iid)
    
    def _CallMethod_(self, com_object, index, info, params):
        
        flags, name, param_descs, ret = info
        assert ret[1][0] == xpcom_consts.TD_UINT32, "Expected an nsresult (%s)" % (ret,)
        if XPT_MD_IS_GETTER(flags):
            
            func = getattr(self._obj_, "get_" + name, None)
            if func is None:
                assert len(param_descs)==1 and len(params)==0, "Can only handle a single [out] arg for a default getter"
                ret = getattr(self._obj_, name) 
            else:
                ret = func(*params)
            return 0, ret
        elif XPT_MD_IS_SETTER(flags):
            
            func = getattr(self._obj_, "set_" + name, None)
            if func is None:
                assert len(param_descs)==1 and len(params)==1, "Can only handle a single [in] arg for a default setter"
                setattr(self._obj_, name, params[0]) 
            else:
                func(*params)
            return 0
        else:
            
            func = getattr(self._obj_, name)
            return 0, func(*params)

    def _doHandleException(self, func_name, exc_info):
        exc_val = exc_info[1]
        is_server_exception = isinstance(exc_val, ServerException)
        if is_server_exception:
            
            
            

            if sys.version_info < (2,4):
                
                
                
                if logger.isEnabledFor(logging.DEBUG):
                    try:
                        raise exc_info[0], exc_info[1], exc_info[2]
                    except:
                        logger.debug("'%s' raised COM Exception %s",
                                 func_name, exc_val, exc_info = 1)
            else:
                logger.debug("'%s' raised COM Exception %s",
                             func_name, exc_val, exc_info=exc_info)

            return exc_val.errno
        
        
        if sys.version_info < (2,4):
            try:
                raise exc_info[0], exc_info[1], exc_info[2]
            except:
                logger.exception("Unhandled exception calling '%s'", func_name)
        else:
            logger.error("Unhandled exception calling '%s'", func_name,
                         exc_info=exc_info)
        return nsError.NS_ERROR_FAILURE

    
    
    
    
    def _CallMethodException_(self, com_object, index, info, params, exc_info):
        
        
        
        flags, name, param_descs, ret = info
        exc_typ, exc_val, exc_tb = exc_info
        
        
        try:
            import xpcom.xpt
            m = xpcom.xpt.Method(info, index, None)
            func_repr = m.Describe().lstrip()
        except COMException:
            func_repr = "%s(%r)" % (name, param_descs)
        except:
            
            self._doHandleException("<building method repr>", sys.exc_info())
            
        return self._doHandleException(func_repr, exc_info)

    
    
    
    def _GatewayException_(self, name, exc_info):
        return self._doHandleException(name, exc_info)

_supports_primitives_data_ = [
    ("nsISupportsCString", "__str__", str),
    ("nsISupportsString", "__unicode__", unicode),
    ("nsISupportsPRUint64", "__long__", long),
    ("nsISupportsPRInt64", "__long__", long),
    ("nsISupportsPRUint32", "__int__", int),
    ("nsISupportsPRInt32", "__int__", int),
    ("nsISupportsPRUint16", "__int__", int),
    ("nsISupportsPRInt16", "__int__", int),
    ("nsISupportsPRUint8", "__int__", int),
    ("nsISupportsPRBool", "__nonzero__", operator.truth),
    ("nsISupportsDouble", "__float__", float),
    ("nsISupportsFloat", "__float__", float),
]


class SupportsPrimitive:
    _com_interfaces_ = ["nsISupports"]
    def __init__(self, iid, base_ob, attr_name, converter):
        self.iid = iid
        self.base_ob = base_ob
        self.attr_name = attr_name
        self.converter = converter
    def _query_interface_(self, iid):
        if iid == self.iid:
            return 1
        return None
    def get_data(self):
        method = getattr(self.base_ob, self.attr_name)
        val = method()
        return self.converter(val)
    def set_data(self, val):
        raise ServerException(nsError.NS_ERROR_NOT_IMPLEMENTED)
    def toString(self):
        return str(self.get_data())

def _shutdown():
    class_info_cache.clear()
