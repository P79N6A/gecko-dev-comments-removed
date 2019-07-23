function dumpn(s) {
  dump(s + "\n");
}

const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_USER_PROFILE_LOCAL_50_DIR = "ProfLD";
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;


var dirSvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
var profileDir = null;
try {
  profileDir = dirSvc.get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);
} catch (e) {}

if (!profileDir) {
  
  
  var provider = {
    getFile: function(prop, persistent) {
      dump("getting file " + prop + "\n");
      persistent.value = true;
      if (prop == NS_APP_USER_PROFILE_50_DIR ||
          prop == NS_APP_USER_PROFILE_LOCAL_50_DIR) {
        return dirSvc.get("CurProcD", Ci.nsIFile);
      }
      throw Cr.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}

var iosvc = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);


var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                 getService(Ci.nsIPrefBranch);
prefBranch.setIntPref("urlclassifier.gethashnoise", 0);


prefBranch.setBoolPref("browser.safebrowsing.malware.enabled", true);
prefBranch.setBoolPref("browser.safebrowsing.enabled", true);

function cleanUp() {
  try {
    
    var file = dirSvc.get('ProfLD', Ci.nsIFile);
    file.append("urlclassifier3.sqlite");
    if (file.exists())
      file.remove(false);
  } catch (e) {}
}

var dbservice = Cc["@mozilla.org/url-classifier/dbservice;1"].getService(Ci.nsIUrlClassifierDBService);
var streamUpdater = Cc["@mozilla.org/url-classifier/streamupdater;1"]
                    .getService(Ci.nsIUrlClassifierStreamUpdater);












function buildUpdate(update, hashSize) {
  if (!hashSize) {
    hashSize = 32;
  }
  var updateStr = "n:1000\n";

  for (var tableName in update) {
    if (tableName != "")
      updateStr += "i:" + tableName + "\n";
    var chunks = update[tableName];
    for (var j = 0; j < chunks.length; j++) {
      var chunk = chunks[j];
      var chunkType = chunk.chunkType ? chunk.chunkType : 'a';
      var chunkNum = chunk.chunkNum ? chunk.chunkNum : j;
      updateStr += chunkType + ':' + chunkNum + ':' + hashSize;

      if (chunk.urls) {
        var chunkData = chunk.urls.join("\n");
        updateStr += ":" + chunkData.length + "\n" + chunkData;
      }

      updateStr += "\n";
    }
  }

  return updateStr;
}

function buildPhishingUpdate(chunks, hashSize) {
  return buildUpdate({"test-phish-simple" : chunks}, hashSize);
}

function buildMalwareUpdate(chunks, hashSize) {
  return buildUpdate({"test-malware-simple" : chunks}, hashSize);
}

function buildBareUpdate(chunks, hashSize) {
  return buildUpdate({"" : chunks}, hashSize);
}




function doSimpleUpdate(updateText, success, failure, clientKey) {
  var listener = {
    QueryInterface: function(iid)
    {
      if (iid.equals(Ci.nsISupports) ||
          iid.equals(Ci.nsIUrlClassifierUpdateObserver))
        return this;
      throw Cr.NS_ERROR_NO_INTERFACE;
    },

    updateUrlRequested: function(url) { },
    streamFinished: function(status) { },
    updateError: function(errorCode) { failure(errorCode); },
    updateSuccess: function(requestedTimeout) { success(requestedTimeout); }
  };

  dbservice.beginUpdate(listener,
                        "test-phish-simple,test-malware-simple",
                        clientKey);
  dbservice.beginStream("", "");
  dbservice.updateStream(updateText);
  dbservice.finishStream();
  dbservice.finishUpdate();
}




function doErrorUpdate(tables, success, failure) {
  var listener = {
    QueryInterface: function(iid)
    {
      if (iid.equals(Ci.nsISupports) ||
          iid.equals(Ci.nsIUrlClassifierUpdateObserver))
        return this;
      throw Cr.NS_ERROR_NO_INTERFACE;
    },

    updateUrlRequested: function(url) { },
    streamCompleted: function() { },
    updateError: function(errorCode) { success(errorCode); },
    updateSuccess: function(requestedTimeout) { failure(requestedTimeout); }
  };

  dbservice.beginUpdate(listener, tables, null);
  dbservice.beginStream("", "");
  dbservice.cancelUpdate();
}





function doStreamUpdate(updateText, success, failure, downloadFailure, clientKey) {
  var dataUpdate = "data:," + encodeURIComponent(updateText);

  if (!downloadFailure)
    downloadFailure = failure;

  streamUpdater.updateUrl = dataUpdate;
  streamUpdater.downloadUpdates("test-phish-simple,test-malware-simple", "",
                                clientKey, success, failure, downloadFailure);
}

var gAssertions = {

tableData : function(expectedTables, cb)
{
  dbservice.getTables(function(tables) {
      
      var parts = tables.split("\n");
      while (parts[parts.length - 1] == '') {
        parts.pop();
      }
      parts.sort();
      tables = parts.join("\n");

      do_check_eq(tables, expectedTables);
      cb();
    });
},

checkUrls: function(urls, expected, cb)
{
  
  urls = urls.slice(0);
  var doLookup = function() {
    if (urls.length > 0) {
      var fragment = urls.shift();
      dbservice.lookup("http://" + fragment,
                       function(arg) {
                         do_check_eq(expected, arg);
                         doLookup();
                       }, true);
    } else {
      cb();
    }
  }
  doLookup();
},

urlsDontExist: function(urls, cb)
{
  this.checkUrls(urls, '', cb);
},

urlsExist: function(urls, cb)
{
  this.checkUrls(urls, 'test-phish-simple', cb);
},

malwareUrlsExist: function(urls, cb)
{
  this.checkUrls(urls, 'test-malware-simple', cb);
},

subsDontExist: function(urls, cb)
{
  
  cb();
},

subsExist: function(urls, cb)
{
  
  cb();
}

};




function checkAssertions(assertions, doneCallback)
{
  var checkAssertion = function() {
    for (var i in assertions) {
      var data = assertions[i];
      delete assertions[i];
      gAssertions[i](data, checkAssertion);
      return;
    }

    doneCallback();
  }

  checkAssertion();
}

function updateError(arg)
{
  do_throw(arg);
}


function doUpdateTest(updates, assertions, successCallback, errorCallback, clientKey) {
  var errorUpdate = function() {
    checkAssertions(assertions, errorCallback);
  }

  var runUpdate = function() {
    if (updates.length > 0) {
      var update = updates.shift();
      doStreamUpdate(update, runUpdate, errorUpdate, null, clientKey);
    } else {
      checkAssertions(assertions, successCallback);
    }
  }

  runUpdate();
}

var gTests;
var gNextTest = 0;

function runNextTest()
{
  if (gNextTest >= gTests.length) {
    do_test_finished();
    return;
  }

  dbservice.resetDatabase();
  dbservice.setHashCompleter('test-phish-simple', null);
  dumpn("running " + gTests[gNextTest]);

  gTests[gNextTest++]();
}

function runTests(tests)
{
  gTests = tests;
  runNextTest();
}

function Timer(delay, cb) {
  this.cb = cb;
  var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(this, delay, timer.TYPE_ONE_SHOT);
}

Timer.prototype = {
QueryInterface: function(iid) {
    if (!iid.equals(Ci.nsISupports) && !iid.equals(Ci.nsITimerCallback)) {
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },
notify: function(timer) {
    this.cb();
  }
}

cleanUp();
