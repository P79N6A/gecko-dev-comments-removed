



import StringIO
import threading

from Zeroconf import Zeroconf, ServiceBrowser
from devicemanager import ZeroconfListener, NetworkTools
from devicemanagerADB import DeviceManagerADB
from devicemanagerSUT import DeviceManagerSUT

class DroidMixin(object):
    """Mixin to extend DeviceManager with Android-specific functionality"""

    def launchApplication(self, appName, activityName, intent, url=None,
                          extras=None):
        """
        Launches an Android application
        returns:
        success: True
        failure: False
        """
        
        if self.processExist(appName):
            return False

        acmd = [ "am", "start", "-W", "-n", "%s/%s" % (appName, activityName)]

        if intent:
            acmd.extend(["-a", intent])

        if extras:
            for (key, val) in extras.iteritems():
                if type(val) is int:
                    extraTypeParam = "--ei"
                elif type(val) is bool:
                    extraTypeParam = "--ez"
                else:
                    extraTypeParam = "--es"
                acmd.extend([extraTypeParam, str(key), str(val)])

        if url:
            acmd.extend(["-d", url])

        
        
        
        shellOutput = StringIO.StringIO()
        if self.shell(acmd, shellOutput) == 0:
            return True

        return False

    def launchFennec(self, appName, intent="android.intent.action.VIEW",
                                      mozEnv=None, extraArgs=None, url=None):
        """
        Convenience method to launch Fennec on Android with various debugging
        arguments
        WARNING: FIXME: This would go better in mozrunner. Please do not
        use this method if you are not comfortable with it going away sometime
        in the near future
        returns:
        success: True
        failure: False
        """
        extras = {}

        if mozEnv:
            
            
            for (envCnt, (envkey, envval)) in enumerate(mozEnv.iteritems()):
                extras["env" + str(envCnt)] = envkey + "=" + envval

        
        
        if extraArgs:
            extras['args'] = " ".join(extraArgs)

        return self.launchApplication(appName, ".App", intent, url=url,
                                                                    extras=extras)

class DroidADB(DeviceManagerADB, DroidMixin):
    pass

class DroidSUT(DeviceManagerSUT, DroidMixin):
    pass

def DroidConnectByHWID(hwid, timeout=30, **kwargs):
    """Try to connect to the given device by waiting for it to show up using mDNS with the given timeout."""
    nt = NetworkTools()
    local_ip = nt.getLanIp()

    zc = Zeroconf(local_ip)

    evt = threading.Event()
    listener = ZeroconfListener(hwid, evt)
    sb = ServiceBrowser(zc, "_sutagent._tcp.local.", listener)
    foundIP = None
    if evt.wait(timeout):
        
        foundIP = listener.ip
    sb.cancel()
    zc.close()

    if foundIP is not None:
        return DroidSUT(foundIP, **kwargs)
    print "Connected via SUT to %s [at %s]" % (hwid, foundIP)

    
    try:
        sut = DroidADB(deviceSerial=hwid, **kwargs)
    except:
        return None

    print "Connected via ADB to %s" % (hwid)
    return sut
