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












function buildUpdate(update) {
  var updateStr = "n:1000\n";

  for (var tableName in update) {
    updateStr += "i:" + tableName + "\n";
    var chunks = update[tableName];
    for (var j = 0; j < chunks.length; j++) {
      var chunk = chunks[j];
      var chunkType = chunk.chunkType ? chunk.chunkType : 'a';
      var chunkNum = chunk.chunkNum ? chunk.chunkNum : j;
      updateStr += chunkType + ':' + chunkNum;

      if (chunk.urls) {
        var chunkData = chunk.urls.join("\n");
        updateStr += ":" + chunkData.length + "\n" + chunkData;
      }

      updateStr += "\n";
    }
  }

  return updateStr;
}

function buildPhishingUpdate(chunks) {
  return buildUpdate({"test-phish-simple" : chunks});
}




function doSimpleUpdate(updateText, success, failure) {
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
    updateError: function(errorCode) { failure(errorCode); },
    updateSuccess: function(requestedTimeout) { success(requestedTimeout); }
  };

  dbservice.beginUpdate(listener);
  dbservice.beginStream();
  dbservice.updateStream(updateText);
  dbservice.finishStream();
  dbservice.finishUpdate();
}





function doStreamUpdate(updateText, success, failure, downloadFailure) {
  var dataUpdate = "data:," + encodeURIComponent(updateText);

  if (!downloadFailure)
    downloadFailure = failure;

  streamUpdater.updateUrl = dataUpdate;
  streamUpdater.downloadUpdates("", success, failure, downloadFailure);
}

var gAssertions = {

tableData : function(expectedTables, cb)
{
  dbservice.getTables(function(tables) {
      
      var parts = tables.split("\n");
      parts.sort();
      tables = parts.join("\n");

      
      
      do_check_eq(tables, "\n" + expectedTables);
      cb();
    });
},

checkUrls: function(urls, expected, cb)
{
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


function doUpdateTest(updates, assertions, successCallback, errorCallback) {
  dbservice.resetDatabase();

  var runUpdate = function() {
    if (updates.length > 0) {
      var update = updates.shift();
      doStreamUpdate(update, runUpdate, errorCallback);
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

  gTests[gNextTest++]();
}

function runTests(tests)
{
  gTests = tests;
  runNextTest();
}

cleanUp();
