








































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
    load(prefix + c);
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

  let prefix = "resource:///";

  load_entries(zipreader.findEntries("components/*js"), prefix);
  load_entries(zipreader.findEntries("modules/*js"), prefix);
  load_entries(zipreader.findEntries("modules/*jsm"), prefix);
  zipreader.close();
}
