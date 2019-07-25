











var loadAsInsecure = false;

var bypassNavigationTest = false;

var navigateToInsecure = false;

var openTwoWindows = false;



var testPage = "";

var testCleanUp = null;



var _windowCount = 0;

window.onload = function onLoad()
{
  if (location.search == "?runtest")
  {
    try
    {
      if (history.length == 1)
        runTest();
      else
        afterNavigationTest();
    }
    catch (ex)
    {
      ok(false, "Exception thrown during test: " + ex);
      finish();
    }
  }
  else
  {
    window.addEventListener("message", onMessageReceived, false);

    var secureTestLocation;
    if (loadAsInsecure)
      secureTestLocation = "http://example.com";
    else
      secureTestLocation = "https://example.com";
    secureTestLocation += location.pathname
    if (testPage != "")
    {
      array = secureTestLocation.split("/");
      array.pop();
      array.push(testPage);
      secureTestLocation = array.join("/");
    }
    secureTestLocation += "?runtest";

    if (openTwoWindows)
    {
      _windowCount = 2;
      window.open(secureTestLocation, "_new1", "");
      window.open(secureTestLocation, "_new2", "");
    }
    else
    {
      _windowCount = 1;
      window.open(secureTestLocation);
    }
  }
}

function onMessageReceived(event)
{
  switch (event.data)
  {
    
    case "done":
      if (--_windowCount == 0)
      {
        if (testCleanUp)
          testCleanUp();
          
        SimpleTest.finish();
      }
      break;

    
    default:
      var failureRegExp = new RegExp("^FAILURE");
      var todoRegExp = new RegExp("^TODO");
      if (event.data.match(todoRegExp))
        SimpleTest.todo(false, event.data);
      else
        SimpleTest.ok(!event.data.match(failureRegExp), event.data);
      break;
  }
}

function postMsg(message)
{
  opener.postMessage(message, "http://mochi.test:8888");
}

function finish()
{
  if (history.length == 1 && !bypassNavigationTest)
  {
    window.setTimeout(function()
    {
      window.location.assign(navigateToInsecure ?
        "http://example.com/tests/security/ssl/mixedcontent/backward.html" :
        "https://example.com/tests/security/ssl/mixedcontent/backward.html");
    }, 0);
  }
  else
  {
    postMsg("done");
    window.close();
  }
}

function ok(a, message)
{
  if (!a)
    postMsg("FAILURE: " + message);
  else
    postMsg(message);
}

function is(a, b, message)
{
  if (a != b)
    postMsg("FAILURE: " + message + ", expected "+b+" got "+a);
  else
    postMsg(message + ", expected "+b+" got "+a);
}

function todo(a, message)
{
  if (a)
    postMsg("FAILURE: TODO works? " + message);
  else
    postMsg("TODO: " + message);
}

function todoSecurityState(expectedState, message)
{
  isSecurityState(expectedState, message, todo);
}

function isSecurityState(expectedState, message, test)
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  
  if (!test)
    test = ok;

  
  var ui = window
    .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
    .getInterface(Components.interfaces.nsIWebNavigation)
    .QueryInterface(Components.interfaces.nsIDocShell)
    .securityUI;

  var isInsecure = !ui ||
    (ui.state & Components.interfaces.nsIWebProgressListener.STATE_IS_INSECURE);
  var isBroken = ui &&
    (ui.state & Components.interfaces.nsIWebProgressListener.STATE_IS_BROKEN);
  var isEV = ui &&
    (ui.state & Components.interfaces.nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL);

  var gotState;
  if (isInsecure)
    gotState = "insecure";
  else if (isBroken)
    gotState = "broken";
  else if (isEV)
    gotState = "EV";
  else
    gotState = "secure";

  test(gotState == expectedState, (message || "") + ", " + "expected " + expectedState + " got " + gotState);

  switch (expectedState)
  {
    case "insecure":
      test(isInsecure && !isBroken && !isEV, "for 'insecure' excpected flags [1,0,0], " + (message || ""));
      break;
    case "broken":
      test(ui && !isInsecure && isBroken && !isEV, "for 'broken' expected  flags [0,1,0], " + (message || ""));
      break;
    case "secure":
      test(ui && !isInsecure && !isBroken && !isEV, "for 'secure' expected flags [0,0,0], " + (message || ""));
      break;
    case "EV":
      test(ui && !isInsecure && !isBroken && isEV, "for 'EV' expected flags [0,0,1], " + (message || ""));
      break;
    default:
      throw "Invalid isSecurityState state";
  }
}

function waitForSecurityState(expectedState, callback)
{
  var roundsLeft = 200; 
  var interval =
  window.setInterval(function() {
    isSecurityState(expectedState, "", function(isok) {if (isok) {roundsLeft = 0;}});
    if (!roundsLeft--) {
      window.clearInterval(interval);
      callback();
    }
  }, 100);
}
