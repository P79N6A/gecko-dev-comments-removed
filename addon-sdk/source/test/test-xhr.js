



var xhr = require("sdk/net/xhr");
var timer = require("sdk/timers");
var { Loader } = require("sdk/test/loader");
var xulApp = require("sdk/system/xul-app");










exports.testLocalXhr = function(test) {
  var req = new xhr.XMLHttpRequest();
  req.overrideMimeType("text/plain");
  req.open("GET", module.uri);
  req.onreadystatechange = function() {
    if (req.readyState == 4 && (req.status == 0 || req.status == 200)) {
      test.assertMatches(req.responseText,
                         /onreadystatechange/,
                         "XMLHttpRequest should get local files");
      timer.setTimeout(
        function() { test.assertEqual(xhr.getRequestCount(), 0);
                     test.done(); },
        0
      );
    }
  };
  req.send(null);
  test.assertEqual(xhr.getRequestCount(), 1);
  test.waitUntilDone(4000);
};

exports.testUnload = function(test) {
  var loader = Loader(module);
  var sbxhr = loader.require("sdk/net/xhr");
  var req = new sbxhr.XMLHttpRequest();
  req.overrideMimeType("text/plain");
  req.open("GET", module.uri);
  req.send(null);
  test.assertEqual(sbxhr.getRequestCount(), 1);
  loader.unload();
  test.assertEqual(sbxhr.getRequestCount(), 0);
};

exports.testResponseHeaders = function(test) {
  var req = new xhr.XMLHttpRequest();
  req.overrideMimeType("text/plain");
  req.open("GET", module.uri);
  req.onreadystatechange = function() {
    if (req.readyState == 4 && (req.status == 0 || req.status == 200)) {
      var headers = req.getAllResponseHeaders();
      if (xulApp.versionInRange(xulApp.platformVersion, "13.0a1", "*")) {
        headers = headers.split("\r\n");
        if(headers.length == 1) {
          headers = headers[0].split("\n");
        }
        for(let i in headers) {
          if(headers[i] && headers[i].search("Content-Type") >= 0) {
            test.assertEqual(headers[i], "Content-Type: text/plain",
                             "XHR's headers are valid");
          }
        }
      }
      else {
        test.assert(headers === null || headers === "",
                    "XHR's headers are empty");
      }
      test.done();
    }
  };
  req.send(null);
  test.assertEqual(xhr.getRequestCount(), 1);
  test.waitUntilDone(4000);
}

