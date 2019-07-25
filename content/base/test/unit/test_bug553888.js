




































do_load_httpd_js();

var server = null;

const SERVER_PORT = 4444;
const HTTP_BASE = "http://localhost:" + SERVER_PORT;
const redirectPath = "/redirect";
const headerCheckPath = "/headerCheck";
const redirectURL = HTTP_BASE + redirectPath;
const headerCheckURL = HTTP_BASE + headerCheckPath;

function redirectHandler(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 302, "Found");
  response.setHeader("Location", headerCheckURL, false);
}

function headerCheckHandler(metadata, response) {
  try {
    let headerValue = metadata.getHeader("X-Custom-Header");
    do_check_eq(headerValue, "present");
  } catch(e) {
    do_throw("No header present after redirect");
  }
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("Content-Type", "text/plain");
  response.write("");
}

function run_test() {
  var server = new nsHttpServer();
  server.registerPathHandler(redirectPath, redirectHandler);
  server.registerPathHandler(headerCheckPath, headerCheckHandler);
  server.start(SERVER_PORT);

  do_test_pending();
  var request = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Components.interfaces.nsIXMLHttpRequest);
  request.open("GET", redirectURL, false);
  request.setRequestHeader("X-Custom-Header", "present");
  request.send(null);

  do_check_eq(request.status, 200);

  server.stop(do_test_finished);
}
