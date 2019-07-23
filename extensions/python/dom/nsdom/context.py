



import sys, types, new, logging

import domcompile

from xpcom.client import Component
from xpcom import components, primitives, COMException, nsError, _xpcom

import _nsdom

GlobalWindowIIDs = [_nsdom.IID_nsIScriptGlobalObject,
                    components.interfaces.nsIDOMWindow]

IID_nsIDOMXULElement = components.interfaces.nsIDOMXULElement


logger = logging.getLogger("xpcom.nsdom")


















xul_element_interfaces = {
    
    'textbox':      [components.interfaces.nsIDOMXULTextBoxElement],
    'button':       [components.interfaces.nsIDOMXULButtonElement],
    'checkbox':     [components.interfaces.nsIDOMXULCheckboxElement],
    'image':        [components.interfaces.nsIDOMXULImageElement],
    'tree':         [components.interfaces.nsIDOMXULTreeElement,
                     components.interfaces.nsIDOMXULMultiSelectControlElement],
    'listbox':      [components.interfaces.nsIDOMXULMultiSelectControlElement],
    'menu':         [components.interfaces.nsIDOMXULSelectControlItemElement],
    'popup':        [components.interfaces.nsIDOMXULPopupElement],
    'radiogroup':   [components.interfaces.nsIDOMXULSelectControlElement],
}

def dump(fmt, *args):
    """A global 'dump' function, available in global namespaces.  Similar to
    The JS version, but this supports % message formatting.

    Main advantage over 'print' is that it catches the possible IO error
    when there is no available console.
    """
    try:
        sys.stderr.write(fmt % args)
        sys.stderr.write("\n")
    except IOError:
        pass


class EventListener:
    _com_interfaces_ = components.interfaces.nsIDOMEventListener
    def __init__(self, context, evt_name, handler, globs):
        
        
        self.context = context
        if callable(handler):
            self.func = handler
        else:
            
            func_name = "on" + evt_name
            
            
            
            arg_names = ('event',)
            co = domcompile.compile_function(handler,
                                             "inline event '%s'" % evt_name,
                                             func_name,
                                             arg_names)
            
            
            g = {}
            exec co in g
            self.func = g[func_name]
        self.globals = globs

    def handleEvent(self, event):
        
        
        if self.globals is not None:
            f = new.function(self.func.func_code, self.globals, 
                            self.func.func_name, self.func.func_closure)
        else:
            f = self.func
        
        
        
        event = _nsdom.MakeDOMObject(self.context, event)
        
        args = (event,)
        
        
        
        args = args[:f.func_code.co_argcount]
        return f(*args)

class WrappedNative(Component):
    """Implements the xpconnect concept of 'wrapped natives' and 'expandos'.

    DOM objects can have arbitrary values set on them.  Once this is done for
    the first time, it gets stored in a map in the context.  This leads to
    cycles, which must be cleaned up when the context is closed.
    """
    def __init__(self, context, obj, iid):
        
        
        
        self.__dict__['_context_'] = context
        
        
        
        self.__dict__['_expandos_'] = {}
        Component.__init__(self, obj, iid)

    def __repr__(self):
        iface_desc = self._get_classinfo_repr_()
        return "<DOM object '%s' (%s)>" % (self._object_name_,iface_desc)
        
    def __getattr__(self, attr):
        
        if attr.startswith("__"):
            raise AttributeError, attr
        
        
        expandos = self._expandos_
        if expandos.has_key(attr):
            return expandos[attr]
        return Component.__getattr__(self, attr)

    def __setattr__(self, attr, value):
        try:
            Component.__setattr__(self, attr, value)
        except AttributeError:
            
            
            logger.debug("setting expando %s.%r=%r for object %r",
                         self, attr, value, self._comobj_)
            
            if attr.startswith("on"):
                
                target = self._comobj_
                if _nsdom.IsOuterWindow(target):
                    target = _nsdom.GetCurrentInnerWindow(target)
                go = self._context_.globalObject
                scope = self._context_.GetNativeGlobal()
                if callable(value):
                    
                    self._expandos_[attr] = value
                    _nsdom.RegisterScriptEventListener(go, scope, target, attr)
                else:
                    _nsdom.AddScriptEventListener(target, attr, value, False, False)
                    _nsdom.CompileScriptEventListener(go, scope, target, attr)
            else:
                self._expandos_[attr] = value
            self._context_._remember_object(self)

    def _build_all_supported_interfaces_(self):
        
        
        Component._build_all_supported_interfaces_(self)
        
        
        ii = self.__dict__['_interface_infos_']
        if ii.has_key(IID_nsIDOMXULElement):
            
            tagName = self.tagName
            interfaces = xul_element_interfaces.get(tagName, [])
            for interface in interfaces:
                if not ii.has_key(interface):
                        self._remember_interface_info(interface)
        else:
            logger.info("Not a DOM element - not hooking extra interfaces")

    def addEventListener(self, event, handler, useCapture=False, globs=None):
        
        
        logger.debug("addEventListener for %r, event=%r, handler=%r, cap=%s",
                     self, event, handler, useCapture)

        if not hasattr(handler, "handleEvent"): 
            
            

            
            
            
            
            if globs is None and not callable(handler):
                globs = sys._getframe().f_back.f_globals
            handler = EventListener(self._context_, event, handler, globs)
        else:
            assert not globs, "can't specify a handler instance and globals"

        base = self.__getattr__('addEventListener')
        base(event, handler, useCapture)

class WrappedNativeGlobal(WrappedNative):
    
    
    
    def __repr__(self):
        iface_desc = self._get_classinfo_repr_()
        outer = _nsdom.IsOuterWindow(self._comobj_)
        return "<GlobalWindow outer=%s %s>" % (outer, iface_desc)

    
    def openDialog(self, url, name, features="", *args):
        svc = components.classes['@mozilla.org/embedcomp/window-watcher;1'] \
                .getService(components.interfaces.nsIWindowWatcher)
        
        
        args = _nsdom.MakeArray(args)
        ret = svc.openWindow(self._comobj_, url, name, features, args)
        
        return _nsdom.MakeDOMObject(self._context_, ret)

    
    open = openDialog

    
    def setTimeout(self, interval, handler, *args):
        return _nsdom.SetTimeoutOrInterval(self._comobj_, interval, handler,
                                           args, False)

    
    def setInterval(self, handler, interval, *args):
        return _nsdom.SetTimeoutOrInterval(self._comobj_, interval, handler,
                                           args, True)

    def clearTimeout(self, tid):
        return _nsdom.ClearTimeoutOrInterval(self._comobj_, tid)

    def clearInterval(self, tid):
        return _nsdom.ClearTimeoutOrInterval(self._comobj_, tid)



class ScriptContext:
    def __init__(self):
        self.globalNamespace = {} 
        self._remembered_objects_ = {} 
        self._reset()

    def _reset(self):
        
        
        
        for ro in self._remembered_objects_.itervalues():
            ro._expandos_.clear()
        self._remembered_objects_.clear()
        
        
        
        self.globalObject = None
        self.globalNamespace.clear()

    def __repr__(self):
        return "<ScriptContext at %d>" % id(self)

    
    def MakeInterfaceResult(self, object, iid):
        if 0 and __debug__: 
            logger.debug("MakeInterfaceResult for %r (remembered=%s)",
                         object,
                         self._remembered_objects_.has_key(object))
        assert not hasattr(object, "_comobj_"), "should not be wrapped!"
        
        try:
            return self._remembered_objects_[object]
        except KeyError:
            
            

            
            
        
            if iid in GlobalWindowIIDs:
                klass = WrappedNativeGlobal
            else:
                klass = WrappedNative
            return klass(self, object, iid)

    
    
    
    
    
    
    def _remember_object(self, object):
        
        
        
        assert self._remembered_objects_.get(object._comobj_, object)==object, \
               "Previously remembered object is not this object!"
        self._remembered_objects_[object._comobj_] = object
        if __debug__:
            logger.debug("%s remembering object %r - now %d items", self,
                         object, len(self._remembered_objects_))

    def _fixArg(self, arg):
        if arg is None:
            return []
        
        
        if type(arg) == types.TupleType:
            return arg
        try:
            argv = arg.QueryInterface(components.interfaces.nsIArray)
        except COMException, why:
            if why.errno != nsError.NS_NOINTERFACE:
                raise
            
            try:
                var = arg.queryInterface(components.interfaces.nsIVariant)
                parent = None
                if self.globalObject is not None:
                    parent = self.globalObject._comobj_
                if parent is None:
                    logger.warning("_fixArg for context with no global??")
                return _xpcom.GetVariantValue(var, parent)
            except COMException, why:
                if why.errno != nsError.NS_NOINTERFACE:
                    raise
                try:
                    return primitives.GetPrimitive(arg)
                except COMException, why:
                    if why.errno != nsError.NS_NOINTERFACE:
                        raise
                return arg
        
        ret = []
        for i in range(argv.length):
            val = argv.queryElementAt(i, components.interfaces.nsISupports)
            ret.append(self._fixArg(val))
        return ret

    def GetNativeGlobal(self):
        return self.globalNamespace

    def CreateNativeGlobalForInner(self, globalObject, isChrome):
        ret = dict()
        if __debug__:
            logger.debug("%r CreateNativeGlobalForInner returning %d",
                         self, id(ret))
        return ret

    def ConnectToInner(self, newInner, globalScope, innerScope):
        if __debug__:
            logger.debug("%r ConnectToInner(%r, %d, %d)",
                         self, newInner, id(globalScope), id(innerScope))
        globalScope['_inner_'] = innerScope 

        self._prepareGlobalNamespace(newInner, innerScope)

    def WillInitializeContext(self):
        if __debug__:
            logger.debug("%r WillInitializeContent", self)

    def DidInitializeContext(self):
        if __debug__:
            logger.debug("%r DidInitializeContent", self)

    def DidSetDocument(self, doc, scope):
        if __debug__:
            logger.debug("%r DidSetDocument doc=%r scope=%d",
                         self, doc, id(scope))
        scope['document'] = doc

    def _prepareGlobalNamespace(self, globalObject, globalNamespace):
        assert isinstance(globalObject, WrappedNativeGlobal), \
               "Our global should have been wrapped in WrappedNativeGlobal"
        if __debug__:
            logger.debug("%r initializing (outer=%s), ns=%d", self,
                         _nsdom.IsOuterWindow(globalObject),
                         id(globalNamespace))
        assert len(globalObject._expandos_)==0, \
               "already initialized this global?"
        ns = globalObject.__dict__['_expandos_'] = globalNamespace
        self._remember_object(globalObject)
        ns['window'] = globalObject
        
        
        
        ns['dump'] = dump

    def InitContext(self, globalObject):
        self._reset()
        self.globalObject = globalObject
        if globalObject is None:
            logger.debug("%r initializing with NULL global, ns=%d", self,
                         id(self.globalNamespace))
        else:
            self._prepareGlobalNamespace(globalObject, self.globalNamespace)

    def FinalizeClasses(self, globalObject):
        self._reset()

    def ClearScope(self, globalObject):
        if __debug__:
            logger.debug("%s.ClearScope (%d)", self, id(globalObject))
        globalObject.clear()

    def FinalizeContext(self):
        if __debug__:
            logger.debug("%s.FinalizeContext", self)
        self._reset()

    def EvaluateString(self, script, glob, principal, url, lineno, ver):
        
        
        
        
        assert type(glob) == types.DictType
        
        co = domcompile.compile(script, url, lineno=lineno-1)
        exec co in glob

    def ExecuteScript(self, scriptObject, scopeObject):
        if __debug__:
            logger.debug("%s.ExecuteScript %r in scope %s",
                         self, scriptObject, id(scopeObject))
        if scopeObject is None:
            scopeObject = self.GetNativeGlobal()
        assert type(scriptObject) == types.CodeType, \
               "Script object should be a code object (got %r)" % (scriptObject,)
        exec scriptObject in scopeObject

    def CompileScript(self, text, scopeObject, principal, url, lineno, version):
        
        return domcompile.compile(text, url, lineno=lineno-1)

    def CompileEventHandler(self, name, argNames, body, url, lineno):
        if __debug__:
            logger.debug("%s.CompileEventHandler %s %s:%s ('%s')",
                         self, name, url, lineno, body[:100])
        co = domcompile.compile_function(body, url, name, argNames,
                                         lineno=lineno-1)
        g = {}
        exec co in g
        handler = g[name]
        
        
        handler._nsdom_compiled = True
        return g[name]

    def CallEventHandler(self, target, scope, handler, argv):
        if __debug__:
            logger.debug("CallEventHandler %r on target %s in scope %s",
                         handler, target, id(scope))
        
        
        
        if hasattr(handler, '_nsdom_compiled'):
            globs = scope
            
            f = new.function(handler.func_code, globs, handler.func_name,
                             handler.func_defaults)
            handler = f
        args = tuple(self._fixArg(argv))
        
        args = args[:handler.func_code.co_argcount]
        return handler(*args)

    def BindCompiledEventHandler(self, target, scope, name, handler):
        if __debug__:
            logger.debug("%s.BindCompiledEventHandler (%s=%r) on target %s in scope %s",
                         self, name, handler, target, id(scope))
        
        self._remember_object(target)
        ns = target._expandos_
        ns[name] = handler

    def GetBoundEventHandler(self, target, scope, name):
        if __debug__:
            logger.debug("%s.GetBoundEventHandler for '%s' on target %s in scope %s",
                         self, name, target, id(scope))
        ns = target._expandos_
        return ns[name]

    def SetProperty(self, target, name, value):
        if __debug__:
            logger.debug("%s.SetProperty for %s=%r", self, name, value)
        target[name] = self._fixArg(value)
