




function DummyCompleter() {
  this.fragments = {};
  this.queries = [];
  this.cachable = true;
  this.tableName = "test-phish-simple";
}

DummyCompleter.prototype =
{
QueryInterface: function(iid)
{
  if (!iid.equals(Ci.nsISupports) &&
      !iid.equals(Ci.nsIUrlClassifierHashCompleter)) {
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
  return this;
},

complete: function(partialHash, cb)
{
  this.queries.push(partialHash);
  var fragments = this.fragments;
  var self = this;
  var doCallback = function() {
      if (self.alwaysFail) {
        cb.completionFinished(1);
        return;
      }
      var results;
      if (fragments[partialHash]) {
        for (var i = 0; i < fragments[partialHash].length; i++) {
          var chunkId = fragments[partialHash][i][0];
          var hash = fragments[partialHash][i][1];
          cb.completion(hash, self.tableName, chunkId, self.cachable);
        }
      }
    cb.completionFinished(0);
  }
  var timer = new Timer(0, doCallback);
},

getHash: function(fragment)
{
  var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
  createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  var data = converter.convertToByteArray(fragment);
  var ch = Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
  ch.init(ch.SHA256);
  ch.update(data, data.length);
  var hash = ch.finish(false);
  return hash.slice(0, 32);
},

addFragment: function(chunkId, fragment)
{
  this.addHash(chunkId, this.getHash(fragment));
},



addConflict: function(chunkId, fragment)
{
  var realHash = this.getHash(fragment);
  var invalidHash = this.getHash("blah blah blah blah blah");
  this.addHash(chunkId, realHash.slice(0, 4) + invalidHash.slice(4, 32));
},

addHash: function(chunkId, hash)
{
  var partial = hash.slice(0, 4);
  if (this.fragments[partial]) {
    this.fragments[partial].push([chunkId, hash]);
  } else {
    this.fragments[partial] = [[chunkId, hash]];
  }
},

compareQueries: function(fragments)
{
  var expectedQueries = [];
  for (var i = 0; i < fragments.length; i++) {
    expectedQueries.push(this.getHash(fragments[i]).slice(0, 4));
  }
  expectedQueries.sort();
  this.queries.sort();
  for (var i = 0; i < this.queries.length; i++) {
    do_check_eq(this.queries[i], expectedQueries[i]);
  }
  do_check_eq(this.queries.length, expectedQueries.length);
}
};

function setupCompleter(table, hits, conflicts)
{
  var completer = new DummyCompleter();
  completer.tableName = table;
  for (var i = 0; i < hits.length; i++) {
    var chunkId = hits[i][0];
    var fragments = hits[i][1];
    for (var j = 0; j < fragments.length; j++) {
      completer.addFragment(chunkId, fragments[j]);
    }
  }
  for (var i = 0; i < conflicts.length; i++) {
    var chunkId = conflicts[i][0];
    var fragments = conflicts[i][1];
    for (var j = 0; j < fragments.length; j++) {
      completer.addConflict(chunkId, fragments[j]);
    }
  }

  dbservice.setHashCompleter(table, completer);

  return completer;
}

function installCompleter(table, fragments, conflictFragments)
{
  return setupCompleter(table, fragments, conflictFragments);
}

function installFailingCompleter(table) {
  var completer = setupCompleter(table, [], []);
  completer.alwaysFail = true;
  return completer;
}

function installUncachableCompleter(table, fragments, conflictFragments)
{
  var completer = setupCompleter(table, fragments, conflictFragments);
  completer.cachable = false;
  return completer;
}


gAssertions.completerQueried = function(data, cb)
{
  var completer = data[0];
  completer.compareQueries(data[1]);
  cb();
}

function doTest(updates, assertions)
{
  doUpdateTest(updates, assertions, runNextTest, updateError);
}


function testPartialAdds() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
    4);


  var completer = installCompleter('test-phish-simple', [[1, addUrls]], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : addUrls,
    "completerQueried" : [completer, addUrls]
  };


  doTest([update], assertions);
}

function testPartialAddsWithConflicts() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
    4);

  
  var completer = installCompleter('test-phish-simple',
                                   [[1, addUrls]],
                                   [[1, addUrls]]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : addUrls,
    "completerQueried" : [completer, addUrls]
  };

  doTest([update], assertions);
}

function testFalsePositives() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
    4);

  
  
  var completer = installCompleter('test-phish-simple', [], [[1, addUrls]]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsDontExist" : addUrls,
    "completerQueried" : [completer, addUrls]
  };

  doTest([update], assertions);
}

function testEmptyCompleter() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
    4);

  
  var completer = installCompleter('test-phish-simple', [], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsDontExist" : addUrls,
    "completerQueried" : [completer, addUrls]
  };

  doTest([update], assertions);
}

function testCompleterFailure() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
    4);

  
  var completer = installFailingCompleter('test-phish-simple');

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsDontExist" : addUrls,
    "completerQueried" : [completer, addUrls]
  };

  doTest([update], assertions);
}

function testMixedSizesSameDomain() {
  var add1Urls = [ "foo.com/a" ];
  var add2Urls = [ "foo.com/b" ];

  var update1 = buildPhishingUpdate(
    [
      { "chunkNum" : 1,
        "urls" : add1Urls }],
    4);
  var update2 = buildPhishingUpdate(
    [
      { "chunkNum" : 2,
        "urls" : add2Urls }],
    32);

  
  var completer = installCompleter('test-phish-simple', [[1, add1Urls]], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1-2",
    
    "urlsExist" : add1Urls.concat(add2Urls),
    
    "completerQueried" : [completer, add1Urls]
  };

  doTest([update1, update2], assertions);
}

function testMixedSizesDifferentDomains() {
  var add1Urls = [ "foo.com/a" ];
  var add2Urls = [ "bar.com/b" ];

  var update1 = buildPhishingUpdate(
    [
      { "chunkNum" : 1,
        "urls" : add1Urls }],
    4);
  var update2 = buildPhishingUpdate(
    [
      { "chunkNum" : 2,
        "urls" : add2Urls }],
    32);

  
  var completer = installCompleter('test-phish-simple', [[1, add1Urls]], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1-2",
    
    "urlsExist" : add1Urls.concat(add2Urls),
    
    "completerQueried" : [completer, add1Urls]
  };

  doTest([update1, update2], assertions);
}

function testInvalidHashSize()
{
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
        12); 

  var completer = installCompleter('test-phish-simple', [[1, addUrls]], []);

  var assertions = {
    "tableData" : "",
    "urlsDontExist" : addUrls
  };

  
  doUpdateTest([update], assertions, updateError, runNextTest);
}

function testWrongTable()
{
  var addUrls = [ "foo.com/a" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
        4);
  var completer = installCompleter('test-malware-simple', 
                                   [[1, addUrls]], []);

  
  
  dbservice.setHashCompleter("test-phish-simple", completer);


  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    
    
    "malwareUrlsExist" : addUrls,
    
    "completerQueried" : [completer, addUrls]
  };

  doUpdateTest([update], assertions,
               function() {
                 
                 var timer = new Timer(3000, function() {
                     
                     
                     var newCompleter = installCompleter('test-malware-simple', [[1, addUrls]], []);

                     
                     
                     
                     dbservice.setHashCompleter("test-phish-simple",
                                                newCompleter);


                     var assertions = {
                       "malwareUrlsExist" : addUrls,
                       "completerQueried" : [newCompleter, addUrls]
                     };
                     checkAssertions(assertions, runNextTest);
                   });
               }, updateError);
}

function testWrongChunk()
{
  var addUrls = [ "foo.com/a" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
        4);
  var completer = installCompleter('test-phish-simple',
                                   [[2, 
                                     addUrls]], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : addUrls,
    
    "completerQueried" : [completer, addUrls]
  };

  doUpdateTest([update], assertions,
               function() {
                 
                 var timer = new Timer(3000, function() {
                     
                     
                     var newCompleter = installCompleter('test-phish-simple', [[2, addUrls]], []);

                     var assertions = {
                       "urlsExist" : addUrls,
                       "completerQueried" : [newCompleter, addUrls]
                     };
                     checkAssertions(assertions, runNextTest);
                   });
               }, updateError);
}

function setupCachedResults(addUrls, part2)
{
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
        4);

  var completer = installCompleter('test-phish-simple', [[1, addUrls]], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    
    "urlsExist" : addUrls,
    
    "completerQueried" : [completer, addUrls]
  };

  doUpdateTest([update], assertions,
               function() {
                 
                 var timer = new Timer(3000, part2);
               }, updateError);
}

function testCachedResults()
{
  setupCachedResults(["foo.com/a"], function(add) {
      
      

      
      var newCompleter = installCompleter('test-phish-simple', [[1, []]], []);

      var assertions = {
        "urlsExist" : ["foo.com/a"],
        "completerQueried" : [newCompleter, []]
      };
      checkAssertions(assertions, runNextTest);
    });
}

function testCachedResultsWithSub() {
  setupCachedResults(["foo.com/a"], function() {
      
      var newCompleter = installCompleter('test-phish-simple', [[1, []]], []);

      var removeUpdate = buildPhishingUpdate(
        [ { "chunkNum" : 2,
            "chunkType" : "s",
            "urls": ["1:foo.com/a"] }],
        4);

      var assertions = {
        "urlsDontExist" : ["foo.com/a"],
        "completerQueried" : [newCompleter, []]
      }

      doTest([removeUpdate], assertions);
    });
}

function testCachedResultsWithExpire() {
  setupCachedResults(["foo.com/a"], function() {
      
      var newCompleter = installCompleter('test-phish-simple', [[1, []]], []);

      var expireUpdate =
        "n:1000\n" +
        "i:test-phish-simple\n" +
        "ad:1\n";

      var assertions = {
        "urlsDontExist" : ["foo.com/a"],
        "completerQueried" : [newCompleter, []]
      }
      doTest([expireUpdate], assertions);
    });
}

function setupUncachedResults(addUrls, part2)
{
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
        4);

  var completer = installUncachableCompleter('test-phish-simple', [[1, addUrls]], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    
    "urlsExist" : addUrls,
    
    "completerQueried" : [completer, addUrls]
  };

  doUpdateTest([update], assertions,
               function() {
                 
                 var timer = new Timer(3000, part2);
               }, updateError);
}

function testUncachedResults()
{
  setupUncachedResults(["foo.com/a"], function(add) {
      
      

      
      var newCompleter = installCompleter('test-phish-simple', [[1, ["foo.com/a"]]], []);
      var assertions = {
        "urlsExist" : ["foo.com/a"],
        "completerQueried" : [newCompleter, ["foo.com/a"]]
      };
      checkAssertions(assertions, runNextTest);
    });
}

function testErrorList()
{
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
    32);

  var completer = installCompleter('test-phish-simple', [[1, addUrls]], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : addUrls,
    
    
    "completerQueried" : [completer, addUrls]
  };

  
  doStreamUpdate(update, function() {
      
      
      doErrorUpdate("test-phish-simple,test-malware-simple", function() {
          
          checkAssertions(assertions, runNextTest);
        }, updateError);
    }, updateError);
}


function testStaleList()
{
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
    32);

  var completer = installCompleter('test-phish-simple', [[1, addUrls]], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : addUrls,
    
    
    "completerQueried" : [completer, addUrls]
  };

  
  prefBranch.setIntPref("urlclassifier.confirm-age", 1);

  
  doStreamUpdate(update, function() {
      
      
      new Timer(3000, function() {
          
          checkAssertions(assertions, function() {
              prefBranch.setIntPref("urlclassifier.confirm-age", 2700);
              runNextTest();
            });
        }, updateError);
    }, updateError);
}



function testStaleListEmpty()
{
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }],
        32);

  var completer = installCompleter('test-phish-simple', [], []);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    
    "urlsDontExist" : addUrls,
    
    
    "completerQueried" : [completer, addUrls]
  };

  
  prefBranch.setIntPref("urlclassifier.confirm-age", 1);

  
  doStreamUpdate(update, function() {
      
      
      new Timer(3000, function() {
          
          checkAssertions(assertions, function() {
              prefBranch.setIntPref("urlclassifier.confirm-age", 2700);
              runNextTest();
            });
        }, updateError);
    }, updateError);
}




function testErrorListIndependent()
{
  var phishUrls = [ "phish.com/a" ];
  var malwareUrls = [ "attack.com/a" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : phishUrls
          }],
    32);

  update += buildMalwareUpdate(
        [
          { "chunkNum" : 2,
            "urls" : malwareUrls
          }],
    32);

  var completer = installCompleter('test-phish-simple', [[1, phishUrls]], []);

  var assertions = {
    "tableData" : "test-malware-simple;a:2\ntest-phish-simple;a:1",
    "urlsExist" : phishUrls,
    "malwareUrlsExist" : malwareUrls,
    
    
    "completerQueried" : [completer, phishUrls]
  };

  
  doStreamUpdate(update, function() {
      
      
      
      doErrorUpdate("test-phish-simple", function() {
          
          checkAssertions(assertions, runNextTest);
        }, updateError);
    }, updateError);
}

function run_test()
{
  runTests([
      testPartialAdds,
      testPartialAddsWithConflicts,
      testFalsePositives,
      testEmptyCompleter,
      testCompleterFailure,
      testMixedSizesSameDomain,
      testMixedSizesDifferentDomains,
      testInvalidHashSize,
      testWrongTable,
      testWrongChunk,
      testCachedResults,
      testCachedResultsWithSub,
      testCachedResultsWithExpire,
      testUncachedResults,
      testStaleList,
      testStaleListEmpty,
      testErrorList,
      testErrorListIndependent,
  ]);
}

do_test_pending();
