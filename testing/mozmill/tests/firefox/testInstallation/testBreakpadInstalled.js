




































const fileNames = {
                   "darwin" : "crashreporter.app",
                   "win32"  : "crashreporter.exe",
                   "linux"  : "crashreporter"
                  };


const states = {
                "Enabled" : true,
                "ServerURL" : "https://crash-reports.mozilla.com/submit"
               };

var setupModule = function(module)
{
  controller = mozmill.getBrowserController();

  
  var dirService = Cc["@mozilla.org/file/directory_service;1"].
                   getService(Ci.nsIProperties);
  module.appDir = dirService.get("XCurProcD", Ci.nsILocalFile);

  
  module.crashReporter = Cc["@mozilla.org/toolkit/crash-reporter;1"]
                           .getService(Ci.nsICrashReporter);
}




var testBreakpadInstalled = function()
{
  
  var execFile = Cc["@mozilla.org/file/local;1"]
                    .createInstance(Ci.nsILocalFile);
  execFile.initWithPath(appDir.path);
  execFile.append(fileNames[mozmill.platform]);

  controller.assertJS("subject.exists() == true", execFile);

  
  controller.assertJS("subject.reporter.enabled == subject.stateEnabled",
                      {reporter: crashReporter, stateEnabled: states['Enabled']});

  
  controller.assertJS("subject.reporter.serverURL.spec == subject.stateServerURL",
                      {reporter: crashReporter, stateServerURL: states['ServerURL']});
}





