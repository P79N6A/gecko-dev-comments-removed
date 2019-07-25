


const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


let (commonFile = do_get_file("../head_common.js", false)) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}



const kDBName = "places.sqlite";









function setPlacesDatabase(aFileName)
{
  let file = do_get_file(aFileName);

  
  let (dbFile = gProfD.clone()) {
    dbFile.append(kDBName);
    do_check_false(dbFile.exists());
  }

  file.copyToFollowingLinks(gProfD, kDBName);
}
