




var testURI = "webcal://127.0.0.1/rheeeeet.html";

const Cc = SpecialPowers.Cc;

function test() {

  const isOSXMtnLion = navigator.userAgent.indexOf("Mac OS X 10.8") != -1;
  if (isOSXMtnLion) {
    todo(false, "This test fails on OS X 10.8, see bug 786938");
    SimpleTest.finish();
    return;
  }

  
  var webHandler = Cc["@mozilla.org/uriloader/web-handler-app;1"].
    createInstance(SpecialPowers.Ci.nsIWebHandlerApp);
  webHandler.name = "Test Web Handler App";
  webHandler.uriTemplate =
      "http://mochi.test:8888/tests/uriloader/exthandler/tests/mochitest/" + 
      "handlerApp.xhtml?uri=%s";
  
  
  var ioService = Cc["@mozilla.org/network/io-service;1"].
    getService(SpecialPowers.Ci.nsIIOService);
  var uri = ioService.newURI(testURI, null, null);

  
  var newWindow = window.open("", "handlerWindow", "height=300,width=300");
  var windowContext = 
    SpecialPowers.wrap(newWindow).QueryInterface(SpecialPowers.Ci.nsIInterfaceRequestor).
    getInterface(SpecialPowers.Ci.nsIWebNavigation).
    QueryInterface(SpecialPowers.Ci.nsIDocShell);
 
  webHandler.launchWithURI(uri, windowContext); 

  
  
  ok(true, "webHandler launchWithURI (existing window/tab) started");

  
  webHandler.launchWithURI(uri);
  
  
  ok(true, "webHandler launchWithURI (new window/tab) test started");

  
  var localHandler = Cc["@mozilla.org/uriloader/local-handler-app;1"].
    createInstance(SpecialPowers.Ci.nsILocalHandlerApp);
  localHandler.name = "Test Local Handler App";
  
  
  var osString = Cc["@mozilla.org/xre/app-info;1"].
                 getService(SpecialPowers.Ci.nsIXULRuntime).OS;

  var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
               getService(SpecialPowers.Ci.nsIDirectoryServiceProvider);
  if (osString == "WINNT") {
    var windowsDir = dirSvc.getFile("WinD", {});
    var exe = windowsDir.clone().QueryInterface(SpecialPowers.Ci.nsILocalFile);
    exe.appendRelativePath("SYSTEM32\\HOSTNAME.EXE");

  } else if (osString == "Darwin") { 
    var localAppsDir = dirSvc.getFile("LocApp", {});
    exe = localAppsDir.clone();
    exe.append("iCal.app"); 
                            
                            
                            

    if (navigator.userAgent.match(/ SeaMonkey\//)) {
      
      
      todo(false, "On SeaMonkey, testing OS X as generic Unix. (Bug 749872)");

      
      exe = Cc["@mozilla.org/file/local;1"].
            createInstance(SpecialPowers.Ci.nsILocalFile);
      exe.initWithPath("/bin/echo");
    }
  } else {
    
    exe = Cc["@mozilla.org/file/local;1"].
          createInstance(SpecialPowers.Ci.nsILocalFile);
    exe.initWithPath("/bin/echo");
  }

  localHandler.executable = exe;
  localHandler.launchWithURI(ioService.newURI(testURI, null, null));

  
  ok(true, "localHandler launchWithURI test");

  
  
  if (osString == "NOTDarwin") {

    var killall = Cc["@mozilla.org/file/local;1"].
                  createInstance(SpecialPowers.Ci.nsILocalFile);
    killall.initWithPath("/usr/bin/killall");
  
    var process = Cc["@mozilla.org/process/util;1"].
                  createInstance(SpecialPowers.Ci.nsIProcess);
    process.init(killall);
    
    var args = ['iCal'];
    process.run(false, args, args.length);
  }

  SimpleTest.waitForExplicitFinish();
}

test();
