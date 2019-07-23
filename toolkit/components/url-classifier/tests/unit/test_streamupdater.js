var gClientKeyRaw="TESTCLIENTKEY";

var gClientKey = "VEVTVENMSUVOVEtFWQ==";

function MAC(content, clientKey)
{
  var hmac = Cc["@mozilla.org/security/hmac;1"].createInstance(Ci.nsICryptoHMAC);
  var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";

  var keyObject = Cc["@mozilla.org/security/keyobjectfactory;1"]
    .getService(Ci.nsIKeyObjectFactory).keyFromString(Ci.nsIKeyObject.HMAC, clientKey);
  hmac.init(Ci.nsICryptoHMAC.SHA1, keyObject);

  var data = converter.convertToByteArray(content);
  hmac.update(data, data.length);
  return hmac.finish(true);
}

function doTest(updates, assertions, expectError, clientKey)
{
  if (expectError) {
    doUpdateTest(updates, assertions, updateError, runNextTest, clientKey);
  } else {
    doUpdateTest(updates, assertions, runNextTest, updateError, clientKey);
  }
}

function testSimpleForward() {
  var add1Urls = [ "foo.com/a", "bar.com/c" ];
  var add2Urls = [ "foo.com/b" ];
  var add3Urls = [ "bar.com/d" ];

  var update = "n:1000\n";
  update += "i:test-phish-simple\n";

  var update1 = buildBareUpdate(
    [{ "chunkNum" : 1,
       "urls" : add1Urls }]);
  update += "u:data:," + encodeURIComponent(update1) + "\n";

  var update2 = buildBareUpdate(
    [{ "chunkNum" : 2,
       "urls" : add2Urls }]);
  update += "u:data:," + encodeURIComponent(update2) + "\n";

  var update3 = buildBareUpdate(
    [{ "chunkNum" : 3,
       "urls" : add3Urls }]);
  update += "u:data:," + encodeURIComponent(update3) + "\n";

  var assertions = {
    "tableData" : "test-phish-simple;a:1-3",
    "urlsExist" : add1Urls.concat(add2Urls).concat(add3Urls)
  };

  doTest([update], assertions, false);
}



function testNestedForward() {
  var add1Urls = [ "foo.com/a", "bar.com/c" ];
  var add2Urls = [ "foo.com/b" ];

  var update = "n:1000\n";
  update += "i:test-phish-simple\n";

  var update1 = buildBareUpdate(
    [{ "chunkNum" : 1,
       "urls" : add1Urls }]);
  update += "u:data:," + encodeURIComponent(update1) + "\n";

  var update2 = buildBareUpdate(
    [{ "chunkNum" : 2 }]);
  var update3 = buildBareUpdate(
    [{ "chunkNum" : 3,
       "urls" : add1Urls }]);

  update2 += "u:data:," + encodeURIComponent(update3) + "\n";

  update += "u:data:," + encodeURIComponent(update2) + "\n";

  var assertions = {
    "tableData" : "",
    "urlsDontExist" : add1Urls.concat(add2Urls)
  };

  doTest([update], assertions, true);
}


function testInvalidUrlForward() {
  var add1Urls = [ "foo.com/a", "bar.com/c" ];

  var update = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "urls" : add1Urls }]);
  update += "u:asdf://blah/blah\n";  

  

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : add1Urls
  };

  doTest([update], assertions, false);
}


function testErrorUrlForward() {
  var add1Urls = [ "foo.com/a", "bar.com/c" ];

  var update = buildPhishingUpdate(
    [{ "chunkNum" : 1,
       "urls" : add1Urls }]);
  update += "u:http://test.invalid/asdf/asdf\n";  

  

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : add1Urls
  };

  doTest([update], assertions, false);
}

function testMultipleTables() {
  var add1Urls = [ "foo.com/a", "bar.com/c" ];
  var add2Urls = [ "foo.com/b" ];
  var add3Urls = [ "bar.com/d" ];

  var update = "n:1000\n";
  update += "i:test-phish-simple\n";

  var update1 = buildBareUpdate(
    [{ "chunkNum" : 1,
       "urls" : add1Urls }]);
  update += "u:data:," + encodeURIComponent(update1) + "\n";

  var update2 = buildBareUpdate(
    [{ "chunkNum" : 2,
       "urls" : add2Urls }]);
  update += "u:data:," + encodeURIComponent(update2) + "\n";

  update += "i:test-malware-simple\n";

  var update3 = buildBareUpdate(
    [{ "chunkNum" : 3,
       "urls" : add3Urls }]);
  update += "u:data:," + encodeURIComponent(update3) + "\n";

  var assertions = {
    "tableData" : "test-malware-simple;a:3\ntest-phish-simple;a:1-2",
    "urlsExist" : add1Urls.concat(add2Urls),
    "malwareUrlsExist" : add3Urls
  };

  doTest([update], assertions, false);
}


function testValidMAC() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);
  update = "m:" + MAC(update, gClientKeyRaw) + "\n" + update;

  var assertions = {
    "tableData" : "test-phish-simple;a:1",
    "urlsExist" : addUrls
  };

  doTest([update], assertions, false, gClientKey);
}


function testInvalidMAC() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);
  update = "m:INVALIDMAC\n" + update;

  var assertions = {
    "tableData" : "",
    "urlsDontExist" : addUrls
  };

  doTest([update], assertions, true, gClientKey);
}



function testNoMAC() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);

  var assertions = {
    "tableData" : "",
    "urlsDontExist" : addUrls
  };

  doTest([update], assertions, true, gClientKey);
}


function testValidForwardMAC() {
  var add1Urls = [ "foo.com/a", "bar.com/c" ];
  var add2Urls = [ "foo.com/b" ];
  var add3Urls = [ "bar.com/d" ];

  var update = "n:1000\n";
  update += "i:test-phish-simple\n";

  var update1 = buildBareUpdate(
    [{ "chunkNum" : 1,
       "urls" : add1Urls }]);
  update += "u:data:," + encodeURIComponent(update1) +
    "," + MAC(update1, gClientKeyRaw) + "\n";

  var update2 = buildBareUpdate(
    [{ "chunkNum" : 2,
       "urls" : add2Urls }]);
  update += "u:data:," + encodeURIComponent(update2) +
    "," + MAC(update2, gClientKeyRaw) + "\n";


  var update3 = buildBareUpdate(
    [{ "chunkNum" : 3,
       "urls" : add3Urls }]);
  update += "u:data:," + encodeURIComponent(update3) +
    "," + MAC(update3, gClientKeyRaw) + "\n";

  var assertions = {
    "tableData" : "test-phish-simple;a:1-3",
    "urlsExist" : add1Urls.concat(add2Urls).concat(add3Urls)
  };

  update = "m:" + MAC(update, gClientKeyRaw) + "\n" + update;

  doTest([update], assertions, false, gClientKey);
}



function testInvalidForwardMAC() {
  var add1Urls = [ "foo.com/a", "bar.com/c" ];
  var add2Urls = [ "foo.com/b" ];
  var add3Urls = [ "bar.com/d" ];

  var update = "n:1000\n";
  update += "i:test-phish-simple\n";

  var update1 = buildBareUpdate(
    [{ "chunkNum" : 1,
       "urls" : add1Urls }]);
  update += "u:data:," + encodeURIComponent(update1) +
    ",BADMAC\n";

  var update2 = buildBareUpdate(
    [{ "chunkNum" : 2,
       "urls" : add2Urls }]);
  update += "u:data:," + encodeURIComponent(update2) +
    ",BADMAC\n";


  var update3 = buildBareUpdate(
    [{ "chunkNum" : 3,
       "urls" : add3Urls }]);
  update += "u:data:," + encodeURIComponent(update3) +
    ",BADMAC\n";

  var assertions = {
    "tableData" : "",
    "urlsDontExist" : add1Urls.concat(add2Urls).concat(add3Urls)
  };

  update = "m:" + MAC(update, gClientKeyRaw) + "\n" + update;

  doTest([update], assertions, true, gClientKey);
}



function testNoForwardMAC() {
  var add1Urls = [ "foo.com/a", "bar.com/c" ];
  var add2Urls = [ "foo.com/b" ];
  var add3Urls = [ "bar.com/d" ];

  var update = "n:1000\n";
  update += "i:test-phish-simple\n";

  
  
  

  
  
  

  var update1 = buildBareUpdate(
    [{ "chunkNum" : 1,
       "urls" : add1Urls }]);
  update += "u:data:" + encodeURIComponent(update1) + "\n";

  var update2 = buildBareUpdate(
    [{ "chunkNum" : 2,
       "urls" : add2Urls }]);
  update += "u:data:" + encodeURIComponent(update2) + "\n";

  var update3 = buildBareUpdate(
    [{ "chunkNum" : 3,
       "urls" : add3Urls }]);
  update += "u:data:" + encodeURIComponent(update3) + "\n";

  var assertions = {
    "tableData" : "",
    "urlsDontExist" : add1Urls.concat(add2Urls).concat(add3Urls)
  };

  update = "m:" + MAC(update, gClientKeyRaw) + "\n" + update;

  doTest([update], assertions, true, gClientKey);
}

function Observer(callback) {
  this.observe = callback;
}

Observer.prototype =
{
QueryInterface: function(iid)
{
  if (!iid.equals(Ci.nsISupports) &&
      !iid.equals(Ci.nsIObserver)) {
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
  return this;
}
};

var gGotRekey;

gAssertions.gotRekey = function(data, cb)
{
  do_check_eq(gGotRekey, data);
  cb();
}


function testRekey() {
  var addUrls = [ "foo.com/a", "foo.com/b", "bar.com/c" ];
  var update = buildPhishingUpdate(
        [
          { "chunkNum" : 1,
            "urls" : addUrls
          }]);
  update = "e:pleaserekey\n" + update;

  var assertions = {
    "tableData" : "",
    "urlsDontExist" : addUrls,
    "gotRekey" : true
  };

  gGotRekey = false;
  var observerService = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);

  observerService.addObserver(new Observer(function(subject, topic, data) {
        if (topic == "url-classifier-rekey-requested") {
          gGotRekey = true;
        }
      }),
    "url-classifier-rekey-requested",
    false);

  doTest([update], assertions, true, gClientKey);
}


function testReset() {
  var addUrls1 = [ "foo.com/a", "foo.com/b" ];
  var update1 = buildPhishingUpdate(
    [
      { "chunkNum" : 1,
        "urls" : addUrls1
      }]);

  var update2 = "n:1000\nr:pleasereset\n";

  var addUrls3 = [ "bar.com/a", "bar.com/b" ];
  var update3 = buildPhishingUpdate(
    [
      { "chunkNum" : 3,
        "urls" : addUrls3
      }]);

  var assertions = {
    "tableData" : "test-phish-simple;a:3",
    "urlsExist" : addUrls3,
    "urlsDontExist" : addUrls1
  };

  doTest([update1, update2, update3], assertions, false);
}


function run_test()
{
  runTests([
    testSimpleForward,
    testNestedForward,
    testInvalidUrlForward,
    testErrorUrlForward,
    testMultipleTables,
    testValidMAC,
    testInvalidMAC,
    testNoMAC,
    testValidForwardMAC,
    testInvalidForwardMAC,
    testNoForwardMAC,
    testRekey,
    testReset,
  ]);
}

do_test_pending();
