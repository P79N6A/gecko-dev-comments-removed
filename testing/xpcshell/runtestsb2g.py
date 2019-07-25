





import sys
import os
import traceback
from remotexpcshelltests import XPCShellRemote, RemoteXPCShellOptions
from automationutils import *
import devicemanagerADB

DEVICE_TEST_ROOT = '/data/local/tests'

sys.path.insert(0, os.path.abspath(os.path.realpath(os.path.dirname(sys.argv[0]))))

from marionette import Marionette


class B2GXPCShellRemote(XPCShellRemote):

    
    def setupUtilities(self):
        
        self.clean()
        self.device.mkDir(DEVICE_TEST_ROOT)

        XPCShellRemote.setupUtilities(self)

    def clean(self):
        self.device.removeDir(DEVICE_TEST_ROOT)

    
    
    
    def getReturnCode(self, proc):



        return 0


class B2GOptions(RemoteXPCShellOptions):

    def __init__(self):
        RemoteXPCShellOptions.__init__(self)
        defaults = {}

        self.add_option('--b2gpath', action='store',
                        type='string', dest='b2g_path',
                        help="Path to B2G repo or qemu dir")
        defaults['b2g_path'] = None

        self.add_option('--marionette', action='store',
                        type='string', dest='marionette',
                        help="host:port to use when connecting to Marionette")
        defaults['marionette'] = None

        self.add_option('--emulator', action='store',
                        type='string', dest='emulator',
                        help="Architecture of emulator to use: x86 or arm")
        defaults['emulator'] = None

        self.add_option('--no-window', action='store_true',
                        dest='no_window',
                        help="Pass --no-window to the emulator")
        defaults['no_window'] = False

        self.add_option('--adbpath', action='store',
                        type='string', dest='adb_path',
                        help="Path to adb")
        defaults['adb_path'] = 'adb'

        defaults['dm_trans'] = 'adb'
        defaults['debugger'] = None
        defaults['debuggerArgs'] = None

        self.set_defaults(**defaults)


def main():
    parser = B2GOptions()
    options, args = parser.parse_args()

    if options.objdir is None:
        try:
            options.objdir = os.path.join(options.b2g_path, 'objdir-gecko')
        except:
            print >> sys.stderr, "Need to specify a --b2gpath"
            sys.exit(1)

    
    kwargs = {}
    if options.emulator:
        kwargs['emulator'] = options.emulator
        if options.no_window:
            kwargs['noWindow'] = True
    if options.b2g_path:
        kwargs['homedir'] = options.b2g_path
    if options.marionette:
        host, port = options.marionette.split(':')
        kwargs['host'] = host
        kwargs['port'] = int(port)
    marionette = Marionette(**kwargs)

    
    kwargs = {'adbPath': options.adb_path}
    if options.deviceIP:
        kwargs['host'] = options.deviceIP
        kwargs['port'] = options.devicePort
    kwargs['deviceRoot'] = DEVICE_TEST_ROOT
    dm = devicemanagerADB.DeviceManagerADB(**kwargs)

    options.remoteTestRoot = dm.getDeviceRoot()

    xpcsh = B2GXPCShellRemote(dm, options, args)

    try:
        success = xpcsh.runTests(xpcshell='xpcshell', testdirs=args[0:], **options.__dict__)
    except:
        print "TEST-UNEXPECTED-FAIL | %s | Exception caught while running tests." % sys.exc_info()[1]
        traceback.print_exc()
        sys.exit(1)

    sys.exit(int(success))










if __name__ == '__main__':
    main()
