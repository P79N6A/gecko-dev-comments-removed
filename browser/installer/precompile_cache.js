








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

function setenv(name, val) {
  try {
        var environment = Components.classes["@mozilla.org/process/environment;1"].
          getService(Components.interfaces.nsIEnvironment);
        environment.set(name, val);
  }
  catch(e) {
    displayError("setenv", e);
  }
}

function load(url) {
  print(url)
  try {
    Cu.import(url, null)
  } catch(e) {
    dump("Failed to import "+url + ":"+e+"\n");
  }
}

function load_entries(entries, prefix) {
  while(entries.hasMore()) {
    var c = entries.getNext();
    load(prefix + c);
  }
}

function getGreDir() {
  return Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties).get("GreD", Ci.nsIFile);
}

function openJar(file) {
  var zipreader = Cc["@mozilla.org/libjar/zip-reader;1"].createInstance(Ci.nsIZipReader);
  zipreader.open(file);
  return zipreader;
}


function populate_omnijar() {
  var file = getGreDir();
  file.append("omni.jar");
  setenv("MOZ_STARTUP_CACHE", file.path);
  zipreader = openJar(file);

  load_entries(zipreader.findEntries("components/*js"), "resource://gre/");
  load_entries(zipreader.findEntries("modules/*jsm"), "resource://gre/");

  
  let ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  let uri = ioService.newURI("resource:///modules/services-sync/",
                             null, null);
  let resProt = ioService.getProtocolHandler("resource")
    .QueryInterface(Ci.nsIResProtocolHandler);
  resProt.setSubstitution("services-sync", uri);

  var entries = zipreader.findEntries("modules/services-sync/*js");
  while(entries.hasMore()) {
    var c = entries.getNext();
    load("resource://services-sync/" + c.replace("modules/services-sync/", ""));
  }
  zipreader.close();
}

function extract_files(jar, pattern, dest) {
  var entries = jar.findEntries(pattern);
  while(entries.hasMore()) {
    var c = entries.getNext();
    var file = dest.clone();
    for each(name in c.split("/"))
      file.append(name);

    if (!file.parent.exists()) { 
      file.parent.create(1 , 0700);
      print("Created " + file.parent.path)
    }

    if (jar.getEntry(c).isDirectory)
      continue;
    
    try {
      jar.extract(c, file);
      print("extracted "+file.path+":"+file.fileSize);
    }catch(e) {
      
      
      print("Failed to extract " + file.path);
      print(e)
    }
  }

}

function extract_jsloader_to_dist_bin() {
  var dist_bin = getGreDir().parent;
  dist_bin.append("bin");

  var file = getGreDir();
  file.append("omni.jar");

  var zipReader = openJar(file);
  
  extract_files(zipReader, "jsloader/*", dist_bin);
  zipReader.close();
}
