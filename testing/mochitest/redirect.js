







































function redirect(aURL)
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  
  const Cc = Components.classes;
  const Ci = Components.interfaces;

  
  
  var webNav = window.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIWebNavigation);
  webNav.loadURI(aURL + location.search,
                 null, null, null, null);
}
