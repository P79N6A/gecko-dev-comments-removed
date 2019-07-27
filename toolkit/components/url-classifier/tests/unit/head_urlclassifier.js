
function dumpn(s) {
  dump(s + "\n");
}

const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_USER_PROFILE_LOCAL_50_DIR = "ProfLD";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

do_get_profile();

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);

var iosvc = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

var secMan = Cc["@mozilla.org/scriptsecuritymanager;1"]
               .getService(Ci.nsIScriptSecurityManager);


var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                 getService(Ci.nsIPrefBranch);
prefBranch.setIntPref("urlclassifier.gethashnoise", 0);


prefBranch.setBoolPref("browser.safebrowsing.malware.enabled", true);
prefBranch.setBoolPref("browser.safebrowsing.enabled", true);


prefBranch.setCharPref("urlclassifier.disallow_completions", "");


prefBranch.setIntPref("urlclassifier.gethash.timeout_ms", 5000);

function delFile(name) {
  try {
    
    var file = dirSvc.get('ProfLD', Ci.nsIFile);
    file.append(name);
    if (file.exists())
      file.remove(false);
  } catch(e) {
  }
}

function cleanUp() {
  delFile("urlclassifier3.sqlite");
  delFile("safebrowsing/classifier.hashkey");
  delFile("safebrowsing/test-phish-simple.sbstore");
  delFile("safebrowsing/test-malware-simple.sbstore");
  delFile("safebrowsing/test-unwanted-simple.sbstore");
  delFile("safebrowsing/test-phish-simple.cache");
  delFile("safebrowsing/test-malware-simple.cache");
  delFile("safebrowsing/test-unwanted-simple.cache");
  delFile("safebrowsing/test-phish-simple.pset");
  delFile("safebrowsing/test-malware-simple.pset");
  delFile("safebrowsing/test-unwanted-simple.pset");
  delFile("testLarge.pset");
  delFile("testNoDelta.pset");
}

var allTables = "test-phish-simple,test-malware-simple,test-unwanted-simple";

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

function buildUnwantedUpdate(chunks, hashSize) {
  return buildUpdate({"test-unwanted-simple" : chunks}, hashSize);
}

function buildBareUpdate(chunks, hashSize) {
  return buildUpdate({"" : chunks}, hashSize);
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
    streamFinished: function(status) { },
    updateError: function(errorCode) { failure(errorCode); },
    updateSuccess: function(requestedTimeout) { success(requestedTimeout); }
  };

  dbservice.beginUpdate(listener,
                        "test-phish-simple,test-malware-simple,test-unwanted-simple");
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
    streamFinished: function(status) { },
    updateError: function(errorCode) { success(errorCode); },
    updateSuccess: function(requestedTimeout) { failure(requestedTimeout); }
  };

  dbservice.beginUpdate(listener, tables, null);
  dbservice.beginStream("", "");
  dbservice.cancelUpdate();
}





function doStreamUpdate(updateText, success, failure, downloadFailure) {
  var dataUpdate = "data:," + encodeURIComponent(updateText);

  if (!downloadFailure) {
    downloadFailure = failure;
  }

  streamUpdater.downloadUpdates("test-phish-simple,test-malware-simple,test-unwanted-simple", "",
                                dataUpdate, success, failure, downloadFailure);
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
      var principal = secMan.getNoAppCodebasePrincipal(iosvc.newURI("http://" + fragment, null, null));
      dbservice.lookup(principal, allTables,
                                function(arg) {
                                  do_check_eq(expected, arg);
                                  doLookup();
                                }, true);
    } else {
      cb();
    }
  };
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

unwantedUrlsExist: function(urls, cb)
{
  this.checkUrls(urls, 'test-unwanted-simple', cb);
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
  var errorUpdate = function() {
    checkAssertions(assertions, errorCallback);
  }

  var runUpdate = function() {
    if (updates.length > 0) {
      var update = updates.shift();
      doStreamUpdate(update, runUpdate, errorUpdate, null);
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

  let test = gTests[gNextTest++];
  dump("running " + test.name + "\n");
  test();
}

function runTests(tests)
{
  gTests = tests;
  runNextTest();
}

var timerArray = [];

function Timer(delay, cb) {
  this.cb = cb;
  var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(this, delay, timer.TYPE_ONE_SHOT);
  timerArray.push(timer);
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





function LFSRgenerator(seed) {
  
  seed = +seed;
  
  if (seed == 0)
    seed = 1;

  this._value = seed;
}
LFSRgenerator.prototype = {
  
  nextNum: function(bits) {
    if (!bits)
      bits = 32;

    let val = this._value;
    
    let bit = ((val >>> 0) ^ (val >>> 10) ^ (val >>> 30) ^ (val >>> 31)) & 1;
    val = (val >>> 1) | (bit << 31);
    this._value = val;

    return (val >>> (32 - bits));
  },
};

cleanUp();
