







































function redirect(aURL)
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  
  const Cc = Components.classes;
  const Ci = Components.interfaces;
  
  var windowMediator = Cc['@mozilla.org/appshell/window-mediator;1'].
  getService(Ci.nsIWindowMediator);
  var win = windowMediator.getMostRecentWindow("navigator:browser");
  win.getWebNavigation().loadURI(aURL + location.search,
                                 null, null, null, null);
}
