




var testURI = "webcal://127.0.0.1/rheeeeet.html";

const Cc = SpecialPowers.wrap(Components).classes;

function test() {

  
  var webHandler = Cc["@mozilla.org/uriloader/web-handler-app;1"].
    createInstance(Components.interfaces.nsIWebHandlerApp);
  webHandler.name = "Test Web Handler App";
  webHandler.uriTemplate =
      "http://mochi.test:8888/tests/uriloader/exthandler/tests/mochitest/" + 
      "handlerApp.xhtml?uri=%s";
  
  
  var ioService = Cc["@mozilla.org/network/io-service;1"].
    getService(Components.interfaces.nsIIOService);
  var uri = ioService.newURI(testURI, null, null);

  
  var newWindow = window.open("", "handlerWindow", "height=300,width=300");
  var windowContext = 
    SpecialPowers.wrap(newWindow).QueryInterface(Components.interfaces.nsIInterfaceRequestor).
    getInterface(Components.interfaces.nsIWebNavigation).
    QueryInterface(Components.interfaces.nsIDocShell);
 
  webHandler.launchWithURI(uri, windowContext); 

  
  
  ok(true, "webHandler launchWithURI (existing window/tab) started");

  
  webHandler.launchWithURI(uri);
  
  
  ok(true, "webHandler launchWithURI (new window/tab) test started");

  
  var localHandler = Cc["@mozilla.org/uriloader/local-handler-app;1"].
    createInstance(Components.interfaces.nsILocalHandlerApp);
  localHandler.name = "Test Local Handler App";
  
  
  var osString = Cc["@mozilla.org/xre/app-info;1"].
                 getService(Components.interfaces.nsIXULRuntime).OS;

  var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
               getService(Components.interfaces.nsIDirectoryServiceProvider);
  if (osString == "WINNT") {
    var windowsDir = dirSvc.getFile("WinD", {});
    var exe = windowsDir.clone().QueryInterface(Components.interfaces.nsILocalFile);
    exe.appendRelativePath("SYSTEM32\\HOSTNAME.EXE");

  } else if (osString == "Darwin") { 
    var localAppsDir = dirSvc.getFile("LocApp", {});
    exe = localAppsDir.clone();
    exe.append("iCal.app"); 
                            
                            
                            

    if (navigator.userAgent.match(/ SeaMonkey\//)) {
      
      
      todo(false, "On SeaMonkey, testing OS X as generic Unix. (Bug 749872)");

      
      exe = Cc["@mozilla.org/file/local;1"].
            createInstance(Components.interfaces.nsILocalFile);
      exe.initWithPath("/bin/echo");
    }
  } else {
    
    exe = Cc["@mozilla.org/file/local;1"].
          createInstance(Components.interfaces.nsILocalFile);
    exe.initWithPath("/bin/echo");
  }

  localHandler.executable = exe;
  localHandler.launchWithURI(ioService.newURI(testURI, null, null));

  
  ok(true, "localHandler launchWithURI test");

  
  
  if (osString == "NOTDarwin") {

    var killall = Cc["@mozilla.org/file/local;1"].
                  createInstance(Components.interfaces.nsILocalFile);
    killall.initWithPath("/usr/bin/killall");
  
    var process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Components.interfaces.nsIProcess);
    process.init(killall);
    
    var args = ['iCal'];
    process.run(false, args, args.length);
  }

  SimpleTest.waitForExplicitFinish();
}

test();
