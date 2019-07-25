








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

var out;

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
    out.writeString(c + "\n");
    load(prefix + c);
  }
}

function load_custom_entries(entries, subst) {
  while (entries.hasMore()) {
    var c = entries.getNext();
    out.writeString(c + "\n");
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

function populate_startupcache(omnijarName, startupcacheName, logName) {
  var file = getGreDir();
  file.append(omnijarName);
  zipreader = openJar(file);

  var scFile = getGreDir();
  scFile.append(startupcacheName);
  setenv("MOZ_STARTUP_CACHE", scFile.path);

  var logFile = getGreDir();
  logFile.append(logName);
  var stream = Cc["@mozilla.org/network/file-output-stream;1"]
                 .createInstance(Ci.nsIFileOutputStream);

  stream.init(logFile, MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE, 0666, 0);

  out = Cc["@mozilla.org/intl/converter-output-stream;1"]
          .createInstance(Ci.nsIConverterOutputStream);

  out.init(stream, "UTF-8", 0, 0);

  
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
  out.close();
}
