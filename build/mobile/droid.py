






































from devicemanagerADB import DeviceManagerADB
from devicemanagerSUT import DeviceManagerSUT
import StringIO

class DroidMixin(object):
  """Mixin to extend DeviceManager with Android-specific functionality"""

  def launchApplication(self, app, activity="App",
                        intent="android.intent.action.VIEW", env=None,
                        url=None, extra_args=None):
    """
    Launches an Android application
    returns:
    success: True
    failure: False
    """
    
    if self.processExist(app):
      return False

    acmd = [ "am", "start", "-a", intent, "-W", "-n", "%s/.%s" % (app, activity)]

    if extra_args:
      acmd.extend(["--es", "args", " ".join(extra_args)])

    if env:
      envCnt = 0
      
      for envkey, envval in env.iteritems():
        acmd.extend(["--es", "env" + str(envCnt), envkey + "=" + envval])
        envCnt += 1

    if url:
      acmd.extend(["-d", ''.join(["'", url, "'"])])

    
    
    
    shellOutput = StringIO.StringIO()
    if self.shell(acmd, shellOutput) == 0:
      return True

    return False

class DroidADB(DeviceManagerADB, DroidMixin):
  pass

class DroidSUT(DeviceManagerSUT, DroidMixin):
  pass
