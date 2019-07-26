



const port = 8099;
const file = require("sdk/io/file");
const { pathFor } = require("sdk/system");

exports.testBasicHTTPServer = function(test) {
  
  
  let basePath = pathFor("ProfD");
  let filePath = file.join(basePath, 'test-httpd.txt');
  let content = "This is the HTTPD test file.\n";
  let fileStream = file.open(filePath, 'w');
  fileStream.write(content);
  fileStream.close();

  let { startServerAsync } = require("sdk/test/httpd");
  let srv = startServerAsync(port, basePath);

  test.waitUntilDone();

  
  let Request = require('sdk/request').Request;
  Request({
    url: "http://localhost:" + port + "/test-httpd.txt",
    onComplete: function (response) {
      test.assertEqual(response.text, content);
      done();
    }
  }).get();

  function done() {
    srv.stop(function() {
      test.done();
    });
  }
};

exports.testDynamicServer = function (test) {
  let content = "This is the HTTPD test file.\n";

  let { startServerAsync } = require("sdk/test/httpd");
  let srv = startServerAsync(port);

  
  
  
  srv.registerPathHandler("/test-httpd.txt", function handle(request, response) {
    
    response.setHeader("Content-Type", "text/plain", false);
    response.write(content);
  });

  test.waitUntilDone();

  
  let Request = require('sdk/request').Request;
  Request({
    url: "http://localhost:" + port + "/test-httpd.txt",
    onComplete: function (response) {
      test.assertEqual(response.text, content);
      done();
    }
  }).get();

  function done() {
    srv.stop(function() {
      test.done();
    });
  }

}
