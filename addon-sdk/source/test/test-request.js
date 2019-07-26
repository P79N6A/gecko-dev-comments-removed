



const { Request } = require("sdk/request");
const { pathFor } = require("sdk/system");
const file = require("sdk/io/file");
const { URL } = require("sdk/url");
const { extend } = require("sdk/util/object");
const { Loader } = require("sdk/test/loader");
const options = require("@test/options");

const loader = Loader(module);
const httpd = loader.require("sdk/test/httpd");
if (options.parseable || options.verbose)
  loader.sandbox("sdk/test/httpd").DEBUG = true;
const { startServerAsync } = httpd;

const { Cc, Ci, Cu } = require("chrome");
const { Services } = Cu.import("resource://gre/modules/Services.jsm");



const basePath = pathFor("ProfD");
const port = 8099;


exports.testOptionsValidator = function(assert) {
  
  assert.throws(function () {
    Request({
      url: null
    });
  }, /The option "url" is invalid./);

  
  let req = Request({
    url: "http://playground.zpao.com/jetpack/request/text.php",
    onComplete: function () {}
  });
  assert.throws(function () {
    req.url = 'www.mozilla.org';
  }, /The option "url" is invalid/);
  
  assert.equal(req.url, "http://playground.zpao.com/jetpack/request/text.php");
};

exports.testContentValidator = function(assert, done) {
  runMultipleURLs(null, assert, done, {
    url: "data:text/html;charset=utf-8,response",
    content: { 'key1' : null, 'key2' : 'some value' },
    onComplete: function(response) {
      assert.equal(response.text, "response?key1=null&key2=some+value");
    }
  });
};


exports.testStatus200 = function (assert, done) {
  let srv = startServerAsync(port, basePath);
  let content = "Look ma, no hands!\n";
  let basename = "test-request.txt"
  prepareFile(basename, content);

  var req = Request({
    url: "http://localhost:" + port + "/" + basename,
    onComplete: function (response) {
      assert.equal(this, req, "`this` should be request");
      assert.equal(response.status, 200);
      assert.equal(response.statusText, "OK");
      assert.equal(response.headers["Content-Type"], "text/plain");
      assert.equal(response.text, content);
      srv.stop(done);
    }
  }).get();
};


exports.testStatus404 = function (assert, done) {
  var srv = startServerAsync(port, basePath);

  runMultipleURLs(srv, assert, done, {
    
    url: "http://localhost:" + port + "/test-request-404.txt",
    onComplete: function (response) {
      assert.equal(response.status, 404);
      assert.equal(response.statusText, "Not Found");
    }
  });
};


exports.testKnownHeader = function (assert, done) {
  var srv = startServerAsync(port, basePath);

 
  let content = "This tests adding headers to the server's response.\n";
  let basename = "test-request-headers.txt";
  let headerContent = "x-jetpack-header: Jamba Juice\n";
  let headerBasename = "test-request-headers.txt^headers^";
  prepareFile(basename, content);
  prepareFile(headerBasename, headerContent);

  runMultipleURLs(srv, assert, done, {
    url: "http://localhost:" + port + "/test-request-headers.txt",
    onComplete: function (response) {
      assert.equal(response.headers["x-jetpack-header"], "Jamba Juice");
    }
  });
};


exports.testComplexHeader = function (assert, done) {
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
  };

  runMultipleURLs(srv, assert, done, {
    url: "http://localhost:" + port + "/test-request-complex-headers.sjs",
    onComplete: function (response) {
      for (k in headers) {
        assert.equal(response.headers[k], headers[k]);
      }
    }
  });
};


exports.test3rdPartyCookies = function (assert, done) {
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
  }.toString();

  prepareFile(basename, content);

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 1);

  Request({
    url: "http://localhost:" + port + "/test-request-3rd-party-cookies.sjs",
    onComplete: function (response) {
      
      assert.equal(response.headers['Set-Cookie'], 'cookie=monster;');

      
      assert.equal(response.headers['x-jetpack-3rd-party'], 'false');

      
      
      Request({
        url: "http://localhost:" + port + "/test-request-3rd-party-cookies.sjs",
        onComplete: function (response) {
          assert.equal(response.headers['x-jetpack-3rd-party'], 'true');
          srv.stop(done);
        }
      }).get();
    }
  }).get();
};

exports.testSimpleJSON = function (assert, done) {
  let srv = startServerAsync(port, basePath);
  let json = { foo: "bar" };
  let basename = "test-request.json";
  prepareFile(basename, JSON.stringify(json));

  runMultipleURLs(srv, assert, done, {
    url: "http://localhost:" + port + "/" + basename,
    onComplete: function (response) {
      assert.deepEqual(response.json, json);
    }
  });
};

exports.testInvalidJSON = function (assert, done) {
  let srv = startServerAsync(port, basePath);
  let basename = "test-request-invalid.json";
  prepareFile(basename, '"this": "isn\'t JSON"');

  runMultipleURLs(srv, assert, done, {
    url: "http://localhost:" + port + "/" + basename,
    onComplete: function (response) {
      assert.equal(response.json, null);
    }
  });
};

exports.testHead = function (assert, done) {
  let srv = startServerAsync(port, basePath);

  srv.registerPathHandler("/test-head",
      function handle(request, response) {
    response.setHeader("Content-Type", "text/plain", false);
  });

  Request({
    url: "http://localhost:" + port + "/test-head",
    onComplete: function (response) {
      assert.equal(response.text, "");
      assert.equal(response.statusText, "OK");
      assert.equal(response.headers["Content-Type"], "text/plain");
      srv.stop(done);
    }
  }).head();
};

function runMultipleURLs (srv, assert, done, options) {
  let urls = [options.url, URL(options.url)];
  let cb = options.onComplete;
  let ran = 0;
  let onComplete = function (res) {
    cb(res);
    if (++ran === urls.length)
      srv ? srv.stop(done) : done();
  };
  urls.forEach(function (url) {
    Request(extend(options, { url: url, onComplete: onComplete })).get();
  });
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

require('sdk/test').run(exports);
