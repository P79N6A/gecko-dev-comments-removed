





const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

var sSame;
var sOther;

const BUGID = "676059";
const OTHERBUGID = "696849";

const pSame = 4444;
const pOther = 4445;

function createXHR(async, method, path)
{
  var xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
            .createInstance(Ci.nsIXMLHttpRequest);
  xhr.open(method, "http://localhost:" + pSame + path, async);
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
  
  sSame = new HttpServer();

  
  sSame.registerPathHandler("/bug" + BUGID + "-redirect301", bug676059redirect301);
  sSame.registerPathHandler("/bug" + BUGID + "-redirect302", bug676059redirect302);
  sSame.registerPathHandler("/bug" + BUGID + "-redirect303", bug676059redirect303);
  sSame.registerPathHandler("/bug" + BUGID + "-redirect307", bug676059redirect307);
  sSame.registerPathHandler("/bug" + BUGID + "-redirect308", bug676059redirect308);

  
  sSame.registerPathHandler("/bug" + OTHERBUGID + "-redirect301", bug696849redirect301);
  sSame.registerPathHandler("/bug" + OTHERBUGID + "-redirect302", bug696849redirect302);
  sSame.registerPathHandler("/bug" + OTHERBUGID + "-redirect303", bug696849redirect303);
  sSame.registerPathHandler("/bug" + OTHERBUGID + "-redirect307", bug696849redirect307);
  sSame.registerPathHandler("/bug" + OTHERBUGID + "-redirect308", bug696849redirect308);

  
  sSame.registerPathHandler("/bug" + BUGID + "-target", echoMethod);
  sSame.start(pSame);

  
  sOther = new HttpServer();
  sOther.registerPathHandler("/bug" + OTHERBUGID + "-target", echoMethod);
  sOther.start(pOther);

  
  
  
  
  
  
  
  
  
  
  var tests = [
    
    [301, "DELETE", null, 301],
    [301, "GET", "GET", 200],
    [301, "HEAD", "HEAD", 200],
    [301, "POST", "GET", 200],
    [301, "PUT", null, 301],
    [301, "PROPFIND", "PROPFIND", 200],
    
    [302, "DELETE", null, 302],
    [302, "GET", "GET", 200],
    [302, "HEAD", "HEAD", 200],
    [302, "POST", "GET", 200],
    [302, "PUT", null, 302],
    [302, "PROPFIND", "PROPFIND", 200],
    
    [303, "DELETE", "GET", 200],
    [303, "GET", "GET", 200],
    [303, "HEAD", "HEAD", 200],
    [303, "POST", "GET", 200],
    [303, "PUT", "GET", 200],
    [303, "PROPFIND", "GET", 200],
    
    [307, "DELETE", null, 307],
    [307, "GET", "GET", 200],
    [307, "HEAD", "HEAD", 200],
    [307, "POST", null, 307],
    [307, "PUT", null, 307],
    [307, "PROPFIND", "PROPFIND", 200],
    
    [308, "DELETE", null, 308],
    [308, "GET", "GET", 200],
    [308, "HEAD", "HEAD", 200],
    [308, "POST", null, 308],
    [308, "PUT", null, 308],
    [308, "PROPFIND", "PROPFIND", 200],
  ];

  
  var othertests = tests; 

  var xhr;
  
  for (var i = 0; i < tests.length; ++i) {
    dump("Testing " + tests[i] + "\n");
    xhr = createXHR(false, tests[i][1], "/bug" + BUGID + "-redirect" + tests[i][0]);
    xhr.send(null);
    checkResults(xhr, tests[i][2], tests[i][3]);
  }  

  for (var i = 0; i < othertests.length; ++i) {
    dump("Testing " + othertests[i] + " (cross-origin)\n");
    xhr = createXHR(false, othertests[i][1], "/bug" + OTHERBUGID + "-redirect" + othertests[i][0]);
    xhr.send(null);
    checkResults(xhr, othertests[i][2], tests[i][3]);
  }  

  sSame.stop(do_test_finished);
  sOther.stop(do_test_finished);
}
 
function redirect(metadata, response, status, port, bugid) {
  
  
  var reason;
  if (status == 301) {
    reason = "Moved Permanently";
  }
  else if (status == 302) {
    reason = "Found";
  }
  else if (status == 303) {
    reason = "See Other";
  }
  else if (status == 307) {
    reason = "Temporary Redirect";
  }
  else if (status == 308) {
    reason = "Permanent Redirect";
  }
  
  response.setStatusLine(metadata.httpVersion, status, reason);
  response.setHeader("Location", "http://localhost:" + port + "/bug" + bugid + "-target");
} 
 

function bug676059redirect301(metadata, response) {
  redirect(metadata, response, 301, pSame, BUGID);
}


function bug696849redirect301(metadata, response) {
  redirect(metadata, response, 301, pOther, OTHERBUGID);
}


function bug676059redirect302(metadata, response) {
  redirect(metadata, response, 302, pSame, BUGID);
}


function bug696849redirect302(metadata, response) {
  redirect(metadata, response, 302, pOther, OTHERBUGID);
}


function bug676059redirect303(metadata, response) {
  redirect(metadata, response, 303, pSame, BUGID);
}


function bug696849redirect303(metadata, response) {
  redirect(metadata, response, 303, pOther, OTHERBUGID);
}


function bug676059redirect307(metadata, response) {
  redirect(metadata, response, 307, pSame, BUGID);
}


function bug676059redirect308(metadata, response) {
  redirect(metadata, response, 308, pSame, BUGID);
}


function bug696849redirect307(metadata, response) {
  redirect(metadata, response, 307, pOther, OTHERBUGID);
}


function bug696849redirect308(metadata, response) {
  redirect(metadata, response, 308, pOther, OTHERBUGID);
}


function echoMethod(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("X-Received-Method", metadata.method);
}
