





Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Preferences.jsm");

var sSame;
var sOther;
var sRedirectPromptPref;

const BUGID = "676059";
const OTHERBUGID = "696849";

XPCOMUtils.defineLazyGetter(this, "pSame", function() {
  return sSame.identity.primaryPort;
});
XPCOMUtils.defineLazyGetter(this, "pOther", function() {
  return sOther.identity.primaryPort;
});

function createXHR(async, method, path)
{
  var xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
            .createInstance(Ci.nsIXMLHttpRequest);
  xhr.open(method, "http://localhost:" + pSame + path, async);
  return xhr;
}

function checkResults(xhr, method, status, unsafe)
{
  if (unsafe) {
    if (sRedirectPromptPref) {
      
      method = null;
    } else {
      
      status = 200;
    }
  }

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
  sSame.start(-1);

  
  sOther = new HttpServer();
  sOther.registerPathHandler("/bug" + OTHERBUGID + "-target", echoMethod);
  sOther.start(-1);

  
  
  
  
  
  
  
  
  
  sRedirectPromptPref = Preferences.get("network.http.prompt-temp-redirect");
  

  
  var tests = [
    
    [301, "DELETE", "DELETE", 301, true],
    [301, "GET", "GET", 200, false],
    [301, "HEAD", "HEAD", 200, false],
    [301, "POST", "GET", 200, false],
    [301, "PUT", "PUT", 301, true],
    [301, "PROPFIND", "PROPFIND", 200, false],
    
    [302, "DELETE", "DELETE", 302, true],
    [302, "GET", "GET", 200, false],
    [302, "HEAD", "HEAD", 200, false],
    [302, "POST", "GET", 200, false],
    [302, "PUT", "PUT", 302, true],
    [302, "PROPFIND", "PROPFIND", 200, false],
    
    [303, "DELETE", "GET", 200, false],
    [303, "GET", "GET", 200, false],
    [303, "HEAD", "HEAD", 200, false],
    [303, "POST", "GET", 200, false],
    [303, "PUT", "GET", 200, false],
    [303, "PROPFIND", "GET", 200, false],
    
    [307, "DELETE", "DELETE", 307, true],
    [307, "GET", "GET", 200, false],
    [307, "HEAD", "HEAD", 200, false],
    [307, "POST", "POST", 307, true],
    [307, "PUT", "PUT", 307, true],
    [307, "PROPFIND", "PROPFIND", 200, false],
    
    [308, "DELETE", "DELETE", 308, true],
    [308, "GET", "GET", 200, false],
    [308, "HEAD", "HEAD", 200, false],
    [308, "POST", "POST", 308, true],
    [308, "PUT", "PUT", 308, true],
    [308, "PROPFIND", "PROPFIND", 200, false],
  ];

  
  var othertests = tests; 

  var xhr;
  
  for (var i = 0; i < tests.length; ++i) {
    dump("Testing " + tests[i] + "\n");
    xhr = createXHR(false, tests[i][1], "/bug" + BUGID + "-redirect" + tests[i][0]);
    xhr.send(null);
    checkResults(xhr, tests[i][2], tests[i][3], tests[i][4]);
  }  

  for (var i = 0; i < othertests.length; ++i) {
    dump("Testing " + othertests[i] + " (cross-origin)\n");
    xhr = createXHR(false, othertests[i][1], "/bug" + OTHERBUGID + "-redirect" + othertests[i][0]);
    xhr.send(null);
    checkResults(xhr, othertests[i][2], tests[i][3], tests[i][4]);
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
