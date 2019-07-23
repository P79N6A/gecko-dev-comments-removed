





































import xpt
import xpcom, _xpcom
import xpcom.client
import xpcom.server
import types

StringTypes = [types.StringType, types.UnicodeType]

def _get_good_iid(iid):
    if iid is None:
        iid = _xpcom.IID_nsISupports
    elif type(iid) in StringTypes and len(iid)>0 and iid[0] != "{":
        iid = getattr(interfaces, iid)
    return iid


manager = xpcom.client.Component(_xpcom.GetComponentManager(), _xpcom.IID_nsIComponentManager)


registrar = xpcom.client.Component(_xpcom.GetComponentManager(), _xpcom.IID_nsIComponentRegistrar)


interfaceInfoManager = _xpcom.XPTI_GetInterfaceInfoManager()


serviceManager = _xpcom.GetServiceManager()


Exception = xpcom.COMException




class _ComponentCollection:
    
    
    
    def __init__(self):
        self._dict_data = None
    def keys(self):
        if self._dict_data is None:
            self._dict_data = self._build_dict()
        return self._dict_data.keys()
    def items(self):
        if self._dict_data is None:
            self._dict_data = self._build_dict()
        return self._dict_data.items()
    def values(self):
        if self._dict_data is None:
            self._dict_data = self._build_dict()
        return self._dict_data.values()
    def has_key(self, key):
        if self._dict_data is None:
            self._dict_data = self._build_dict()
        return self._dict_data.has_key(key)

    def __len__(self):
        if self._dict_data is None:
            self._dict_data = self._build_dict()
        return len(self._dict_data)

    def __getattr__(self, attr):
        if self._dict_data is not None and self._dict_data.has_key(attr):
            return self._dict_data[attr]
        return self._get_one(attr)
    def __getitem__(self, item):
        if self._dict_data is not None and self._dict_data.has_key(item):
            return self._dict_data[item]
        return self._get_one(item)

_constants_by_iid_map = {}

class _Interface:
    
    def __init__(self, name, iid):
        
        d = self.__dict__
        d['_iidobj_'] = iid 
        d['name'] = name
    def __cmp__(self, other):
        this_iid = self._iidobj_
        other_iid = getattr(other, "_iidobj_", other)
        return cmp(this_iid, other_iid)
    def __hash__(self):
        return hash(self._iidobj_)
    def __str__(self):
        return str(self._iidobj_)
    def __getitem__(self, item):
        raise TypeError, "components.interface objects are not subscriptable"
    def __setitem__(self, item, value):
        raise TypeError, "components.interface objects are not subscriptable"
    def __setattr__(self, attr, value):
        raise AttributeError, "Can not set attributes on components.Interface objects"
    def __getattr__(self, attr):
        
        c = _constants_by_iid_map.get(self._iidobj_)
        if c is None:
            c = {}
            i = xpt.Interface(self._iidobj_)
            for c_ob in i.constants:
                c[c_ob.name] = c_ob.value
            _constants_by_iid_map[self._iidobj_] = c
        if c.has_key(attr):
            return c[attr]
        raise AttributeError, "'%s' interfaces do not define a constant '%s'" % (self.name, attr)


class _Interfaces(_ComponentCollection):
    def _get_one(self, name):
        try:
            item = interfaceInfoManager.GetInfoForName(name)
        except xpcom.COMException, why:
            
            import nsError
            raise xpcom.COMException(nsError.NS_ERROR_NO_INTERFACE, "The interface '%s' does not exist" % (name,))
        return _Interface(item.GetName(), item.GetIID())

    def _build_dict(self):
        ret = {}
        enum = interfaceInfoManager.EnumerateInterfaces()
        while not enum.IsDone():
            
            items = enum.FetchBlock(500, _xpcom.IID_nsIInterfaceInfo)
            
            for item in items:
                ret[item.GetName()] = _Interface(item.GetName(), item.GetIID())
        return ret


interfaces = _Interfaces()

del _Interfaces 


class _Class:
    def __init__(self, contractid):
        self.contractid = contractid
    def __getattr__(self, attr):
        if attr == "clsid":
            rc = registrar.contractIDToCID(self.contractid)
            
            self.clsid = rc
            return rc
        raise AttributeError, "%s class has no attribute '%s'" % (self.contractid, attr)
    def createInstance(self, iid = None):
        import xpcom.client
        try:
            return xpcom.client.Component(self.contractid, _get_good_iid(iid))
        except xpcom.COMException, details:
            import nsError
            
            if details.errno == nsError.NS_ERROR_FACTORY_NOT_REGISTERED:
                raise xpcom.COMException(details.errno, "No such component '%s'" % (self.contractid,))
            raise 
    def getService(self, iid = None):
        return serviceManager.getServiceByContractID(self.contractid, _get_good_iid(iid))

class _Classes(_ComponentCollection):
    def __init__(self):
        _ComponentCollection.__init__(self)
    def _get_one(self, name):
        
        return _Class(name)

    def _build_dict(self):
        ret = {}
        enum = registrar.enumerateContractIDs()
        while enum.hasMoreElements():
            
            items = enum.fetchBlock(2000, _xpcom.IID_nsISupportsCString)
            for item in items:
                name = str(item.data)
                ret[name] = _Class(name)
        return ret

classes = _Classes()

del _Classes

del _ComponentCollection


ID = _xpcom.IID

def _on_shutdown():
    global manager, registrar, classes, interfaces, interfaceInfoManager, serviceManager, _constants_by_iid_map
    manager = registrar = classes = interfaces = interfaceInfoManager = serviceManager = _constants_by_iid_map = None
    
    xpcom.client._shutdown()
    xpcom.server._shutdown()


import shutdown
shutdown.register(_on_shutdown)

del _on_shutdown, shutdown
