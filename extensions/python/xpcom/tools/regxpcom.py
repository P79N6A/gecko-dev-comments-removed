







































from xpcom import components, _xpcom
from xpcom.client import Component
import sys
import os

registrar = Component(_xpcom.GetComponentRegistrar(),
                      components.interfaces.nsIComponentRegistrar)

def ProcessArgs(args):

    unregister = 0
    for arg in args:
        spec = components.classes['@mozilla.org/file/local;1'].createInstance()
        spec = spec.QueryInterface(components.interfaces.nsILocalFile)
        spec.initWithPath(os.path.abspath(arg))
        if arg == "-u":
            registrar.autoUnregister(spec)
            print "Successfully unregistered", spec.path
        else:
            registrar.autoRegister(spec)
            print "Successfully registered", spec.path

if len(sys.argv) < 2:
    registrar.autoRegister( None)
else:
    ProcessArgs(sys.argv[1:])


