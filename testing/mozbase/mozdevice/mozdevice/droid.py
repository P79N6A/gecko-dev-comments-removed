



import StringIO
import re
import threading

from Zeroconf import Zeroconf, ServiceBrowser
from devicemanager import ZeroconfListener, NetworkTools
from devicemanagerADB import DeviceManagerADB
from devicemanagerSUT import DeviceManagerSUT
from devicemanager import DMError

class DroidMixin(object):
    """Mixin to extend DeviceManager with Android-specific functionality"""

    def _getExtraAmStartArgs(self):
        return []

    def launchApplication(self, appName, activityName, intent, url=None,
                          extras=None):
        """
        Launches an Android application

        :param appName: Name of application (e.g. `com.android.chrome`)
        :param activityName: Name of activity to launch (e.g. `.Main`)
        :param intent: Intent to launch application with
        :param url: URL to open
        :param extras: Dictionary of extra arguments to launch application with
        """
        
        if self.processExist(appName):
            raise DMError("Only one instance of an application may be running "
                          "at once")

        acmd = [ "am", "start" ] + self._getExtraAmStartArgs() + \
            ["-W", "-n", "%s/%s" % (appName, activityName)]

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
            return

        shellOutput.seek(0)
        raise DMError("Unable to launch application (shell output: '%s')" % shellOutput.read())

    def launchFennec(self, appName, intent="android.intent.action.VIEW",
                     mozEnv=None, extraArgs=None, url=None):
        """
        Convenience method to launch Fennec on Android with various debugging
        arguments

        :param appName: Name of fennec application (e.g. `org.mozilla.fennec`)
        :param intent: Intent to launch application with
        :param mozEnv: Mozilla specific environment to pass into application
        :param extraArgs: Extra arguments to be parsed by fennec
        :param url: URL to open
        """
        extras = {}

        if mozEnv:
            
            
            for (envCnt, (envkey, envval)) in enumerate(mozEnv.iteritems()):
                extras["env" + str(envCnt)] = envkey + "=" + envval

        
        
        if extraArgs:
            extras['args'] = " ".join(extraArgs)

        self.launchApplication(appName, ".App", intent, url=url, extras=extras)

class DroidADB(DeviceManagerADB, DroidMixin):
    pass

class DroidSUT(DeviceManagerSUT, DroidMixin):

    def _getExtraAmStartArgs(self):
        
        
        
        
        if not hasattr(self, '_userSerial'):
            infoDict = self.getInfo(directive="sutuserinfo")
            if infoDict.get('sutuserinfo') and \
                    len(infoDict['sutuserinfo']) > 0:
               userSerialString = infoDict['sutuserinfo'][0]
               
               m = re.match('User Serial:([0-9]+)', userSerialString)
               if m:
                   self._userSerial = m.group(1)
               else:
                   self._userSerial = None
            else:
                self._userSerial = None

        if self._userSerial is not None:
            return [ "--user", self._userSerial ]

        return []

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
