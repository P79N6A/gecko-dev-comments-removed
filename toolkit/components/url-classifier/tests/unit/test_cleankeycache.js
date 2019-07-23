
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



function run_test()
{
  runTests([
             testCleanHostKeys,
             testDirtyHostKeys,
             testUpdate,
  ]);
}

do_test_pending();
