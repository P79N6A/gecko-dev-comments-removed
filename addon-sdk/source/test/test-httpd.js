



const port = 8099;
const file = require("sdk/io/file");
const { pathFor } = require("sdk/system");
const { Loader } = require("sdk/test/loader");
const options = require("@test/options");

const loader = Loader(module);
const httpd = loader.require("sdk/test/httpd");
if (options.parseable || options.verbose)
  loader.sandbox("sdk/test/httpd").DEBUG = true;

exports.testBasicHTTPServer = function(assert, done) {
  
  
  let basePath = pathFor("ProfD");
  let filePath = file.join(basePath, 'test-httpd.txt');
  let content = "This is the HTTPD test file.\n";
  let fileStream = file.open(filePath, 'w');
  fileStream.write(content);
  fileStream.close();

  let srv = httpd.startServerAsync(port, basePath);

  
  let Request = require('sdk/request').Request;
  Request({
    url: "http://localhost:" + port + "/test-httpd.txt",
    onComplete: function (response) {
      assert.equal(response.text, content);
      srv.stop(done);
    }
  }).get();
};

exports.testDynamicServer = function (assert, done) {
  let content = "This is the HTTPD test file.\n";

  let srv = httpd.startServerAsync(port);

  
  
  
  srv.registerPathHandler("/test-httpd.txt", function handle(request, response) {
    
    response.setHeader("Content-Type", "text/plain", false);
    response.write(content);
  });

  
  let Request = require('sdk/request').Request;
  Request({
    url: "http://localhost:" + port + "/test-httpd.txt",
    onComplete: function (response) {
      assert.equal(response.text, content);
      srv.stop(done);
    }
  }).get();
};

exports.testAutomaticPortSelection = function (assert, done) {
  const srv = httpd.startServerAsync(-1);

  const port = srv.identity.primaryPort;
  assert.ok(0 <= port && port <= 65535);

  srv.stop(done);
};

require('sdk/test').run(exports);
