



































function test() {
  
  ok(Services, "Services object exists");
  checkServices();
}

function checkService(service, interface) {
  ok(service in Services, "Services." + service + " exists");
  ok(Services[service] instanceof interface, "Services." + service + " is an " + interface);
}

function checkServices() {
  checkService("prefs", Ci.nsIPrefBranch2);
  checkService("prefs", Ci.nsIPrefService);
  checkService("contentPrefs", Ci.nsIContentPrefService);
  checkService("wm", Ci.nsIWindowMediator);
  checkService("obs", Ci.nsIObserverService);
  checkService("perms", Ci.nsIPermissionManager);
  checkService("io", Ci.nsIIOService);
  checkService("io", Ci.nsIIOService2);
  checkService("appinfo", Ci.nsIXULAppInfo);
  checkService("appinfo", Ci.nsIXULRuntime);
  checkService("dirsvc", Ci.nsIDirectoryService);
  checkService("dirsvc", Ci.nsIProperties);
  checkService("prompt", Ci.nsIPromptService);
  if ("nsIBrowserSearchService" in Ci)
    checkService("search", Ci.nsIBrowserSearchService);
  checkService("storage", Ci.mozIStorageService);
  checkService("vc", Ci.nsIVersionComparator);
  checkService("locale", Ci.nsILocaleService);
  checkService("scriptloader", Ci.mozIJSSubScriptLoader);
  checkService("ww", Ci.nsIWindowWatcher);
  checkService("tm", Ci.nsIThreadManager);
  checkService("droppedLinkHandler", Ci.nsIDroppedLinkHandler);
  checkService("strings", Ci.nsIStringBundleService);
  checkService("urlFormatter", Ci.nsIURLFormatter);
  checkService("eTLD", Ci.nsIEffectiveTLDService);
  checkService("cookies", Ci.nsICookieManager2);
  checkService("logins", Ci.nsILoginManager);
}
