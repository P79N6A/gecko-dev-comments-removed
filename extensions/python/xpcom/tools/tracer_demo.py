












































import profile

p = profile.Profile()
getters = {}
setters = {}



class TracerDelegate:
    def __init__(self, callme):
        self.callme = callme
    def __call__(self, *args):
        return p.runcall(self.callme, *args)




class Tracer:
    def __init__(self, ob):
        self.__dict__['_ob'] = ob
    def __repr__(self):
        return "<Tracer around %r>" % (self._ob,)
    def __str__(self):
        return "<Tracer around %r>" % (self._ob,)
    def __getattr__(self, attr):
        ret = getattr(self._ob, attr) 
        if callable(ret):
            return TracerDelegate(ret)
        else:
            if not attr.startswith("_com_") and not attr.startswith("_reg_"):
                getters[attr] = getters.setdefault(attr,0) + 1
            return ret
    def __setattr__(self, attr, val):
        if self.__dict__.has_key(attr):
            self.__dict__[attr] = val
            return
        setters[attr] = setters.setdefault(attr,0) + 1
        setattr(self._ob, attr, val)



def MakeTracer(ob):
    
    if isinstance(ob, Tracer):
        return ob
    return Tracer(ob)

def test():
    import xpcom.server, xpcom.components
    xpcom.server.tracer = MakeTracer
    contractid = "Python.TestComponent"
    for i in range(100):
        c = xpcom.components.classes[contractid].createInstance().queryInterface(xpcom.components.interfaces.nsIPythonTestInterface)
        c.boolean_value = 0
        a = c.boolean_value
        c.do_boolean(0,1)
    print "Finshed"
    p.print_stats()
    print "%-30s%s" % ("Attribute Gets", "Number")
    print "-" * 36
    for name, num in getters.items():
        print "%-30s%d" % (name, num)
    print "%-30s%s" % ("Attribute Sets", "Number")
    print "-" * 36
    for name, num in setters.items():
        print "%-30s%d" % (name, num)

test()
