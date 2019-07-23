
function testCleanHostKeys() {
  var addUrls = [ "foo.com/a" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);

  doStreamUpdate(update, function() {
      var ios = Components.classes["@mozilla.org/network/io-service;1"].
        getService(Components.interfaces.nsIIOService);

      
      var uri = ios.newURI("http://bar.com/a", null, null);

      
      
      
      var classifier = dbservice.QueryInterface(Ci.nsIURIClassifier);
      var result = classifier.classify(uri, function(errorCode) {
          var result2 = classifier.classify(uri, function() {
              do_throw("shouldn't get a callback");
            });
          
          do_check_eq(result2, false);

          runNextTest();
        });

      
      do_check_eq(result, true);
    }, updateError);
}


function testDirtyHostKeys() {
  var addUrls = [ "foo.com/a" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);

  doStreamUpdate(update, function() {
      var ios = Components.classes["@mozilla.org/network/io-service;1"].
        getService(Components.interfaces.nsIIOService);

      
      var uri = ios.newURI("http://foo.com/b", null, null);

      
      
      
      var classifier = dbservice.QueryInterface(Ci.nsIURIClassifier);
      var result = classifier.classify(uri, function(errorCode) {
          var result2 = classifier.classify(uri, function() {
              runNextTest();
            });
          
          do_check_eq(result2, true);
        });

      
      do_check_eq(result, true);
    }, updateError);
}


function testUpdate() {
  var ios = Components.classes["@mozilla.org/network/io-service;1"].
    getService(Components.interfaces.nsIIOService);

  
  var uri = ios.newURI("http://foo.com/a", null, null);

  
  
  
  var classifier = dbservice.QueryInterface(Ci.nsIURIClassifier);
  var result = classifier.classify(uri, function(errorCode) {
      
      
      do_check_eq(errorCode, Cr.NS_OK);

      
      var addUrls = [ "foo.com/a" ];
      var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);
      doStreamUpdate(update, function() {
          
          
          var result2 = classifier.classify(uri, function(errorCode) {
              do_check_neq(errorCode, Cr.NS_OK);
              runNextTest();
            });
          
          do_check_eq(result2, true);
        }, updateError);
    });
}

function testResetFullCache() {
  
  var ios = Components.classes["@mozilla.org/network/io-service;1"].
    getService(Components.interfaces.nsIIOService);

  
  
  
  var classifier = dbservice.QueryInterface(Ci.nsIURIClassifier);

  var uris1 = [
    "www.foo.com/",
    "www.bar.com/",
    "www.blah.com/",
    "www.site.com/",
    "www.example.com/",
    "www.test.com/",
    "www.malware.com/",
    "www.phishing.com/",
    "www.clean.com/" ];

  var uris2 = [];

  var runSecondLookup = function() {
    if (uris2.length == 0) {
      runNextTest();
      return;
    }

    var spec = uris2.pop();
    var uri = ios.newURI("http://" + spec, null, null);

    var result = classifier.classify(uri, function(errorCode) {
        runSecondLookup();
      });
    
  }

  var runInitialLookup = function() {
    if (uris1.length == 0) {
      
      
      var addUrls = [ "notgoingtocheck.com/a" ];
      var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);
      doStreamUpdate(update, function() {
          runSecondLookup();
        }, updateError);
      return;
    }
    var spec = uris1.pop();

    uris2.push(spec);
    var uri = ios.newURI("http://" + spec, null, null);
    var result = classifier.classify(uri, function(errorCode) {
        runInitialLookup();
      });
    
    do_check_eq(result, true);
    if (result) {
      doNextTest();
    }
  }

  
  
  
  var t = new Timer(3000, runInitialLookup);
}

function run_test()
{
  runTests([
             
             
             
             testUpdate,
             testCleanHostKeys,
             testDirtyHostKeys,
             testResetFullCache,
  ]);
}

do_test_pending();
