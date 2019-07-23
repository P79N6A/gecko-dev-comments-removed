var gExpectedCookies;
var gExpectedLoads;

var gPopup;

var gLoads = 0;

function setupTest(uri, cookies, loads) {
  SimpleTest.waitForExplicitFinish();

  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  Components.classes["@mozilla.org/preferences-service;1"]
            .getService(Components.interfaces.nsIPrefBranch)
            .setIntPref("network.cookie.cookieBehavior", 1);

  var cs = Components.classes["@mozilla.org/cookiemanager;1"]
                     .getService(Components.interfaces.nsICookieManager2);
  cs.removeAll();

  gExpectedCookies = cookies;
  gExpectedLoads = loads;

  
  window.addEventListener("message", messageReceiver, false);

  
  
  gPopup = window.open(uri, 'hai', 'width=100,height=100');
}




function messageReceiver(evt)
{
  is(evt.data, "message", "message data received from popup");
  if (evt.data != "message") {
    gPopup.close();
    window.removeEventListener("message", messageReceiver, false);

    SimpleTest.finish();
    return;
  }

  
  if (++gLoads == gExpectedLoads) {
    gPopup.close();
    window.removeEventListener("message", messageReceiver, false);

    runTest();
  }
}



function runTest() {
  
  document.cookie = "oh=hai";

  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  var cs = Components.classes["@mozilla.org/cookiemanager;1"]
                     .getService(Components.interfaces.nsICookieManager);
  var count = 0;
  for(var list = cs.enumerator; list.hasMoreElements(); list.getNext())
    ++count;
  is(count, gExpectedCookies, "total number of cookies");
  cs.removeAll();

  SimpleTest.finish();
}
