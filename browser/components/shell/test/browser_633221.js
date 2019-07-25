function test() {
  let osString = Cc["@mozilla.org/xre/app-info;1"].
                 getService(Ci.nsIXULRuntime).OS;

  
  if (osString != "Linux")
    return;

  let shell = Cc["@mozilla.org/browser/shell-service;1"].
              getService(Ci.nsIShellService);

  ok(!shell.isDefaultBrowser(true), "we're not the default browser");
  shell.setDefaultBrowser(true, false);
  ok(shell.isDefaultBrowser(true), "we're the default browser");
}
