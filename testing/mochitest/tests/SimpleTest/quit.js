












































var ipcMode = false;
if (typeof(TestRunner) != "undefined") {
  ipcMode = TestRunner.ipcMode;
}

function quitHook()
{
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "http://" + location.host + "/server/shutdown", true);
  xhr.onreadystatechange = function (event)
    {
      if (xhr.readyState == 4)
        goQuitApplication();
    };
  xhr.send(null);
}

function canQuitApplication()
{
  var os = Components.classes["@mozilla.org/observer-service;1"]
    .getService(Components.interfaces.nsIObserverService);
  if (!os) 
  {
    return true;
  }
  
  try 
  {
    var cancelQuit = Components.classes["@mozilla.org/supports-PRBool;1"]
      .createInstance(Components.interfaces.nsISupportsPRBool);
    os.notifyObservers(cancelQuit, "quit-application-requested", null);
    
    
    if (cancelQuit.data)
    {
      return false;
    }
  }
  catch (ex) 
  {
  }
  return true;
}

function goQuitApplication()
{
  if (ipcMode) {
    contentAsyncEvent("QuitApplication");
    return;
  }

  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  if (!canQuitApplication()) {
    return false;
  }
  
  var appService = Components.classes['@mozilla.org/toolkit/app-startup;1']
                             .getService(Components.interfaces.nsIAppStartup);
  appService.quit(Components.interfaces.nsIAppStartup.eForceQuit);
  return true;
}
