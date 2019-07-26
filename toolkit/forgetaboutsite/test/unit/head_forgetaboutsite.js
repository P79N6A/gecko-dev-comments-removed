



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
var profileDir = do_get_profile();




function cleanUp()
{
  let files = [
    "downloads.sqlite",
    "places.sqlite",
    "cookies.sqlite",
    "signons.sqlite",
    "permissions.sqlite"
  ];

  for (let i = 0; i < files.length; i++) {
    let file = dirSvc.get("ProfD", Ci.nsIFile);
    file.append(files[i]);
    if (file.exists())
      file.remove(false);
  }
}
cleanUp();

function oldDownloadManagerDisabled()
{
  try {
    
    Services.downloads.activeDownloadCount;
  } catch (ex) {
    return true;
  }
  return false;
}
