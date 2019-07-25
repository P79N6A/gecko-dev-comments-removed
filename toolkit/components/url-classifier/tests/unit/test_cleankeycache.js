

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
      let principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                        .getService(Components.interfaces.nsIScriptSecurityManager)
                        .getNoAppCodebasePrincipal(uri);

      
      
      
      var classifier = dbservice.QueryInterface(Ci.nsIURIClassifier);
      var result = classifier.classify(principal, function(errorCode) {
          var result2 = classifier.classify(principal, function() {
              do_throw("shouldn't get a callback");
            });
          
          do_check_eq(result2, false);
        do_throw("shouldn't get a callback");
        });

      
      do_check_eq(result, false);
      runNextTest();
    }, updateError);
}


function testUpdate() {
  var ios = Components.classes["@mozilla.org/network/io-service;1"].
    getService(Components.interfaces.nsIIOService);

  
  var preUrls = [ "foo.com/b" ];
  var preUpdate = buildPhishingUpdate(
    [
      { "chunkNum" : 1,
        "urls" : preUrls
      }]);

  doStreamUpdate(preUpdate, function() {
    
    var uri = ios.newURI("http://foo.com/a", null, null);
    let principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                      .getService(Components.interfaces.nsIScriptSecurityManager)
                      .getNoAppCodebasePrincipal(uri);

    
    
    
    var classifier = dbservice.QueryInterface(Ci.nsIURIClassifier);
    var result = classifier.classify(principal, function(errorCode) {
      
      do_check_eq(errorCode, Cr.NS_OK);
      do_throw("shouldn't get a callback");
    });
    do_check_eq(result, false);

    
    var addUrls = [ "foo.com/a" ];
    var update = buildPhishingUpdate(
      [
        { "chunkNum" : 2,
          "urls" : addUrls
        }]);
    doStreamUpdate(update, function() {
      var result2 = classifier.classify(principal, function(errorCode) {
        do_check_neq(errorCode, Cr.NS_OK);
        runNextTest();
      });
      
      do_check_eq(result2, true);
    }, updateError);
  }, updateError);
}

function testResetFullCache() {
  
  var preUrls = [ "zaz.com/b" ];
  var preUpdate = buildPhishingUpdate(
    [
      { "chunkNum" : 1,
        "urls" : preUrls
      }]);

  doStreamUpdate(preUpdate, function() {
    
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
      let principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                        .getService(Components.interfaces.nsIScriptSecurityManager)
                        .getNoAppCodebasePrincipal(uri);

      var result = classifier.classify(principal, function(errorCode) {
      });
      runSecondLookup();
      
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
      let principal = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                        .getService(Components.interfaces.nsIScriptSecurityManager)
                        .getNoAppCodebasePrincipal(uri);
      var result = classifier.classify(principal, function(errorCode) {
      });
      runInitialLookup();
      
      do_check_eq(result, false);
      if (!result) {
        doNextTest();
      }
    }

    
    
    
    var t = new Timer(3000, runInitialLookup);
  }, updateError);
}

function testBug475436() {
  var addUrls = [ "foo.com/a", "www.foo.com/" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : ["foo.com/a", "foo.com/a" ]
  };

  doUpdateTest([update], assertions, runNextTest, updateError);
}

function run_test()
{
  runTests([
             
             
             
             testUpdate,
             testCleanHostKeys,
             testResetFullCache,
             testBug475436
  ]);
}

do_test_pending();
