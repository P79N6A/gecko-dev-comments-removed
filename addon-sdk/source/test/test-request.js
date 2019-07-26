



const { Request } = require("sdk/request");
const { pathFor } = require("sdk/system");
const file = require("sdk/io/file");

const { Loader } = require("sdk/test/loader");
const options = require("@test/options");

const loader = Loader(module);
const httpd = loader.require("sdk/test/httpd");
if (options.parseable || options.verbose)
  loader.sandbox("sdk/test/httpd").DEBUG = true;
const { startServerAsync } = httpd;

const { Cc, Ci, Cu } = require("chrome");
const { Services } = Cu.import("resource://gre/modules/Services.jsm");



const basePath = pathFor("ProfD")
const port = 8099;


exports.testOptionsValidator = function(test) {
  
  test.assertRaises(function () {
    Request({
      url: null
    });
  }, 'The option "url" must be one of the following types: string');

  
  let req = Request({
    url: "http://playground.zpao.com/jetpack/request/text.php",
    onComplete: function () {}
  });
  test.assertRaises(function () {
    req.url = null;
  }, 'The option "url" must be one of the following types: string');
  
  test.assertEqual(req.url, "http://playground.zpao.com/jetpack/request/text.php");
}

exports.testContentValidator = function(test) {
  test.waitUntilDone();
  Request({
    url: "data:text/html;charset=utf-8,response",
    content: { 'key1' : null, 'key2' : 'some value' },
    onComplete: function(response) {
      test.assertEqual(response.text, "response?key1=null&key2=some+value");
      test.done();
    }
  }).get();
};


exports.testStatus200 = function (test) {
  let srv = startServerAsync(port, basePath);
  let content = "Look ma, no hands!\n";
  let basename = "test-request.txt"
  prepareFile(basename, content);

  test.waitUntilDone();
  var req = Request({
    url: "http://localhost:" + port + "/" + basename,
    onComplete: function (response) {
      test.assertEqual(this, req, "`this` should be request");
      test.assertEqual(response.status, 200);
      test.assertEqual(response.statusText, "OK");
      test.assertEqual(response.headers["Content-Type"], "text/plain");
      test.assertEqual(response.text, content);
      srv.stop(function() test.done());
    }
  }).get();
}


exports.testStatus404 = function (test) {
  var srv = startServerAsync(port, basePath);

  test.waitUntilDone();
  Request({
    
    url: "http://localhost:" + port + "/test-request-404.txt",
    onComplete: function (response) {
      test.assertEqual(response.status, 404);
      test.assertEqual(response.statusText, "Not Found");
      srv.stop(function() test.done());
    }
  }).get();
}


exports.testKnownHeader = function (test) {
  var srv = startServerAsync(port, basePath);

 
  let content = "This tests adding headers to the server's response.\n";
  let basename = "test-request-headers.txt";
  let headerContent = "x-jetpack-header: Jamba Juice\n";
  let headerBasename = "test-request-headers.txt^headers^";
  prepareFile(basename, content);
  prepareFile(headerBasename, headerContent);

  test.waitUntilDone();
  Request({
    url: "http://localhost:" + port + "/test-request-headers.txt",
    onComplete: function (response) {
      test.assertEqual(response.headers["x-jetpack-header"], "Jamba Juice");
      srv.stop(function() test.done());
    }
  }).get();
}


exports.testComplexHeader = function (test) {
  let srv = startServerAsync(port, basePath);

  let basename = "test-request-complex-headers.sjs";
  let content = handleRequest.toString();
  prepareFile(basename, content);

  let headers = {
    "x-jetpack-header": "Jamba Juice is: delicious",
    "x-jetpack-header-2": "foo,bar",
    "x-jetpack-header-3": "sup dawg, i heard you like x, so we put a x in " +
      "yo x so you can y while you y",
    "Set-Cookie": "foo=bar\nbaz=foo"
  }

  test.waitUntilDone();
  Request({
    url: "http://localhost:" + port + "/test-request-complex-headers.sjs",
    onComplete: function (response) {
      for (k in headers) {
        test.assertEqual(response.headers[k], headers[k]);
      }
      srv.stop(function() test.done());
    }
  }).get();
}


exports.test3rdPartyCookies = function (test) {
  let srv = startServerAsync(port, basePath);

  let basename = "test-request-3rd-party-cookies.sjs";

  
  let content = function handleRequest(request, response) {
    var cookiePresent = request.hasHeader("Cookie");
    
    if(!cookiePresent) {
      response.setHeader("Set-Cookie", "cookie=monster;", "true");
      response.setHeader("x-jetpack-3rd-party", "false", "true");
    } else {
      
      response.setHeader("x-jetpack-3rd-party", "true", "true");
    }

    response.write("<html><body>This tests 3rd party cookies.</body></html>");
  }.toString()

  prepareFile(basename, content);

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 1);

  test.waitUntilDone();
  Request({
    url: "http://localhost:" + port + "/test-request-3rd-party-cookies.sjs",
    onComplete: function (response) {
      
      test.assertEqual(response.headers['Set-Cookie'], 'cookie=monster;');

      
      test.assertEqual(response.headers['x-jetpack-3rd-party'], 'false');

      
      
      Request({
        url: "http://localhost:" + port + "/test-request-3rd-party-cookies.sjs",
        onComplete: function (response) {
          test.assertEqual(response.headers['x-jetpack-3rd-party'], 'true');
          srv.stop(function() test.done());
        }
      }).get();
    }
  }).get();
}

exports.testSimpleJSON = function (test) {
  let srv = startServerAsync(port, basePath);
  let json = { foo: "bar" };
  let basename = "test-request.json";
  prepareFile(basename, JSON.stringify(json));

  test.waitUntilDone();
  Request({
    url: "http://localhost:" + port + "/" + basename,
    onComplete: function (response) {
      assertDeepEqual(test, response.json, json);
      srv.stop(function() test.done());
    }
  }).get();
}

exports.testInvalidJSON = function (test) {
  let srv = startServerAsync(port, basePath);
  let basename = "test-request-invalid.json";
  prepareFile(basename, '"this": "isn\'t JSON"');

  test.waitUntilDone();
  Request({
    url: "http://localhost:" + port + "/" + basename,
    onComplete: function (response) {
      test.assertEqual(response.json, null);
      srv.stop(function() test.done());
    }
  }).get();
}

































































































































































function assertDeepEqual(test, obj1, obj2, msg) {
  function equal(o1, o2) {
    
    if (o1 == o2)
      return true;
    if (typeof(o1) != typeof(o2))
      return false;
    if (typeof(o1) != "object")
      return o1 == o2;

    let e = true;
    for (let key in o1) {
      let val = o1[key];
      e = e && key in o2 && equal(o2[key], val);
      if (!e)
        break;
    }
    for (let key in o2) {
      let val = o2[key]
      e = e && key in o1 && equal(o1[key], val);
      if (!e)
        break;
    }
    return e;
  }
  msg = msg || "objects not equal - " + JSON.stringify(obj1) + " != " +
               JSON.stringify(obj2);
  test.assert(equal(obj1, obj2), msg);
}

function prepareFile(basename, content) {
  let filePath = file.join(basePath, basename);
  let fileStream = file.open(filePath, 'w');
  fileStream.write(content);
  fileStream.close();
}


function handleRequest(request, response) {
  
  response.setHeader("x-jetpack-header", "Jamba Juice is: delicious", "true");

  
  response.setHeader("x-jetpack-header-2", "foo", "true");
  response.setHeader("x-jetpack-header-2", "bar", "true");

  
  response.setHeader("x-jetpack-header-3", "sup dawg, i heard you like x, " +
    "so we put a x in yo x so you can y while you y", "true");

  
  response.setHeader("Set-Cookie", "foo=bar", "true");
  response.setHeader("Set-Cookie", "baz=foo", "true");

  response.write("<html><body>This file tests more complex headers.</body></html>");
}

