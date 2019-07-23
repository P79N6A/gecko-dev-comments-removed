



































 
function cleanUpFormHist() {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var formhist = Components.classes["@mozilla.org/satchel/form-history;1"].
                 getService(Components.interfaces.nsIFormHistory2);
  formhist.removeAllEntries();
}
cleanUpFormHist();
