



































function test() {
  
  ok(Services, "Services object exists");
  checkServices();
}

function checkServices() {
  ok(Services.prefs instanceof Ci.nsIPrefBranch2, "Services.prefs is an nsIPrefBranch2");
  ok(Services.prefs instanceof Ci.nsIPrefService, "Services.prefs is an nsIPrefService");
  ok(Services.wm instanceof Ci.nsIWindowMediator, "Services.wm is an nsIWindowMediator");
  ok(Services.pm instanceof Ci.nsIPermissionManager, "Services.pm is an nsIPermissionManager");
  ok(Services.io instanceof Ci.nsIIOService, "Services.io is an nsIIOService");
  ok(Services.io instanceof Ci.nsIIOService2, "Services.io is an nsIIOService2");
  ok(Services.prompt instanceof Ci.nsIPromptService, "Services.prompt is an nsIPromptService");
  ok(Services.search instanceof Ci.nsIBrowserSearchService, "Services.search is an nsIBrowserSearchService");
}
