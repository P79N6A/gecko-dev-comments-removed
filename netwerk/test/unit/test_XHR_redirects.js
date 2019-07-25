





do_load_httpd_js();

var server;
const BUGID = "676059";

function createXHR(async, method, path)
{
  var xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
            .createInstance(Ci.nsIXMLHttpRequest);
  xhr.open(method, "http://localhost:4444" + path, async);
  return xhr;
}

function checkResults(xhr, method, status)
{
  if (xhr.readyState != 4)
    return false;

  do_check_eq(xhr.status, status);
  
  if (status == 200) {
    
    do_check_eq(xhr.getResponseHeader("X-Received-Method"), method);
  }
  
  return true;
}

function run_test() {
  
  server = new nsHttpServer();

  server.registerPathHandler("/bug" + BUGID + "-redirect301", bug676059redirect301);
  server.registerPathHandler("/bug" + BUGID + "-redirect302", bug676059redirect302);
  server.registerPathHandler("/bug" + BUGID + "-redirect303", bug676059redirect303);
  server.registerPathHandler("/bug" + BUGID + "-redirect307", bug676059redirect307);
  server.registerPathHandler("/bug" + BUGID + "-target", bug676059target);

  server.start(4444);

  
  
  
  
  
  
  
  
    
  var tests = [
    
    [301, "DELETE", "GET", 200], 
    [301, "GET", "GET", 200],
    [301, "HEAD", "GET", 200], 
    [301, "POST", "GET", 200],
    [301, "PUT", "GET", 200], 
    [301, "PROPFIND", "GET", 200], 
    
    [302, "DELETE", "GET", 200], 
    [302, "GET", "GET", 200],
    [302, "HEAD", "GET", 200], 
    [302, "POST", "GET", 200],
    [302, "PUT", "GET", 200], 
    [302, "PROPFIND", "GET", 200], 
    
    [303, "DELETE", "GET", 200],
    [303, "GET", "GET", 200],
    [303, "HEAD", "GET", 200],
    [303, "POST", "GET", 200],
    [303, "PUT", "GET", 200],
    [303, "PROPFIND", "GET", 200],
    
    [307, "DELETE", null, 307],
    [307, "GET", "GET", 200],
    [307, "HEAD", "HEAD", 200],
    [307, "POST", null, 307],
    [307, "PUT", null, 307],
    [307, "PROPFIND", "PROPFIND", 200],
  ];

  var xhr;
  
  for (var i = 0; i < tests.length; ++i) {
    dump("Testing " + tests[i] + "\n");
    xhr = createXHR(false, tests[i][1], "/bug" + BUGID + "-redirect" + tests[i][0]);
    xhr.send(null);
    checkResults(xhr, tests[i][2], tests[i][3]);
  }  

  server.stop(do_test_finished);
}
 
 
function bug676059redirect301(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 301, "Moved Permanently");
  response.setHeader("Location", "http://localhost:4444/bug" + BUGID + "-target");
}

 
function bug676059redirect302(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 302, "Found");
  response.setHeader("Location", "http://localhost:4444/bug" + BUGID + "-target");
}

 
function bug676059redirect303(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 303, "See Other");
  response.setHeader("Location", "http://localhost:4444/bug" + BUGID + "-target");
}

 
function bug676059redirect307(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 307, "Temporary Redirect");
  response.setHeader("Location", "http://localhost:4444/bug" + BUGID + "-target");
}

 
function bug676059target(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("X-Received-Method", metadata.method);
}