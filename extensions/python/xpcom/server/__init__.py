






































from policy import DefaultPolicy
from xpcom import _xpcom










tracer = tracer_unwrap = None


def WrapObject(ob, iid, policy = None, bWrapClient = 1):
    """Called by the framework to attempt to wrap
    an object in a policy.
    If iid is None, it will use the first interface the object indicates it supports.
    """
    if policy is None:
        policy = DefaultPolicy
    if tracer is not None:
        ob = tracer(ob)
    return _xpcom.WrapObject(policy( ob, iid ), iid, bWrapClient)


def UnwrapObject(ob):
    if ob is None:
        return None
    ret = _xpcom.UnwrapObject(ob)._obj_
    if tracer_unwrap is not None:
        ret = tracer_unwrap(ret)
    return ret




def NS_GetModule( serviceManager, nsIFile ):
    import loader, module
    iid = _xpcom.IID_nsIModule
    return WrapObject(module.Module([loader.ModuleLoader]), iid, bWrapClient = 0)

def _shutdown():
    from policy import _shutdown
    _shutdown()
