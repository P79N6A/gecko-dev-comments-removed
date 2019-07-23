function test() {
  
  let hasPrivileges = true;

  
  try {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefBranch);
  }
  catch (e) {
    hasPrivileges = false;
  }

  
  ok(hasPrivileges, "running with chrome privileges");
}
