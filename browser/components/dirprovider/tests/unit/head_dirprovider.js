



































const Cc = Components.classes;
const Ci = Components.interfaces;

var gProfD = do_get_profile();
var gDirSvc = Cc["@mozilla.org/file/directory_service;1"].
              getService(Ci.nsIProperties);
var gPrefSvc = Cc["@mozilla.org/preferences-service;1"].
               getService(Ci.nsIPrefBranch);

function writeTestFile(aParent, aName) {
  let file = aParent.clone();
  file.append(aName);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0644);
  return file;
}

