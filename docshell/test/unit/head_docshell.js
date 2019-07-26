



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);
var profileDir = do_get_profile();

function cleanup()
{
  
  try {
    if (profileDir.exists())
      profileDir.remove(true);
  } catch (e) {
    
    
    
    
  }
}


cleanup();


profileDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
