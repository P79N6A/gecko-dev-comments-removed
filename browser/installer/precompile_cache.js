








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

function setenv(name, val) {
  try {
    var environment = Components.classes["@mozilla.org/process/environment;1"].
      getService(Components.interfaces.nsIEnvironment);
    environment.set(name, val);
  } catch(e) {
    displayError("setenv", e);
  }
}

function load(url) {
  print(url);
  try {
    Cu.import(url, null);
  } catch(e) {
    dump("Failed to import " + url + ":" + e + "\n");
  }
}

function load_entries(entries, prefix) {
  while (entries.hasMore()) {
    var c = entries.getNext();
    
    
    if (c.indexOf("services-sync") >= 0)
      continue;
    if (c.indexOf("services-crypto") >= 0)
      continue;
    load(prefix + c);
  }
}

function load_custom_entries(entries, subst) {
  while (entries.hasMore()) {
    var c = entries.getNext();
    load("resource://" + subst + "/" + c.replace("modules/" + subst + "/", ""));
  }
}

function getGreDir() {
  return Cc["@mozilla.org/file/directory_service;1"].
    getService(Ci.nsIProperties).get("GreD", Ci.nsIFile);
}

function openJar(file) {
  var zipreader = Cc["@mozilla.org/libjar/zip-reader;1"].
    createInstance(Ci.nsIZipReader);
  zipreader.open(file);
  return zipreader;
}

function populate_startupcache(omnijarName, startupcacheName) {
  var file = getGreDir();
  file.append(omnijarName);
  zipreader = openJar(file);

  var scFile = getGreDir();
  scFile.append(startupcacheName);
  setenv("MOZ_STARTUP_CACHE", scFile.path);

  
  let ioService = Cc["@mozilla.org/network/io-service;1"].
    getService(Ci.nsIIOService);
  let uri = ioService.newURI("resource:///modules/services-sync/",
                             null, null);
  let resProt = ioService.getProtocolHandler("resource")
    .QueryInterface(Ci.nsIResProtocolHandler);
  resProt.setSubstitution("services-sync", uri);

  uri = ioService.newURI("resource:///modules/services-crypto/",
                         null, null);
  resProt.setSubstitution("services-crypto", uri);

  load_entries(zipreader.findEntries("components/*js"), "resource://gre/");

  load_custom_entries(zipreader.findEntries("modules/services-sync/*js"),
                      "services-sync");
  load_custom_entries(zipreader.findEntries("modules/services-crypto/*js"),
                      "services-crypto");

  load_entries(zipreader.findEntries("modules/*js"), "resource://gre/");
  load_entries(zipreader.findEntries("modules/*jsm"), "resource://gre/");
  zipreader.close();
}
