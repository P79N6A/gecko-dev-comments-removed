"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

function readTestData(testFile) {
  var testcase = {};

  
  var istream = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
  try {
    istream.init(testFile, 0x01, parseInt("0444", 8), 0);
    istream.QueryInterface(Ci.nsILineInputStream);

    var hasmore = false;
    do {
      var line = {};
      hasmore = istream.readLine(line);

      if (line.value.indexOf('Description:') > -1) {
        testcase.desc = line.value.substring(line.value.indexOf(':')+1).trim();
      }

      if (line.value.indexOf('Expect:') > -1) {
        testcase.expect = line.value.substring(line.value.indexOf(':')+1).trim();
      }

      if (line.value.indexOf('Base:') > -1) {
        testcase.base = NetUtil.newURI(line.value.substring(line.value.indexOf(':')+1).trim());
      }

      if (testcase.expect && testcase.desc) {
        testcase.path = 'xml/' + testFile.parent.leafName + '/' + testFile.leafName;
        testcase.file = testFile;
        break;
      }

    } while (hasmore);

  } catch(e) {
    Assert.ok(false, "FAILED! Error reading testFile case in file " + testFile.leafName  + " ---- " + e);
  } finally {
    istream.close();
  }

  return testcase;
}

function iterateDir(dir, recurse, callback) {
  do_print("Iterate " + dir.leafName);
  let entries = dir.directoryEntries;

  
  while (entries.hasMoreElements()) {
    let entry = entries.getNext();
    entry.QueryInterface(Ci.nsILocalFile);

    if (entry.isDirectory()) {
      if (recurse) {
        iterateDir(entry, recurse, callback);
      }
    } else {
      callback(entry);
    }
  }
}

function isIID(a, iid){
  try {
    a.QueryInterface(iid);
    return true;
  } catch(e) { }

  return false;
}
