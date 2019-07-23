





var gState = 0;




var MAX_STATE;

function trace(text) {
  var t = text.replace(/&/g, "&" + "amp;").replace(/</g, "&" + "lt;") + "<br>";
  
}




todo(typeof window.ononline == "undefined",
     "window.ononline should be undefined at this point");
todo(typeof window.onoffline == "undefined",
     "window.onoffline should be undefined at this point");

if (0) {
  window.ononline = function() {
    ok(false, "window.ononline shouldn't be called");
  }
  window.onoffline = function() {
    ok(false, "window.onclick shouldn't be called");
  }
}













function makeHandler(nameTemplate, eventName, expectedStates) {
  return function(e) {
    var name = nameTemplate.replace(/%1/, eventName);
    ++gState;
    trace(name + ": gState=" + gState);
    ok(expectedStates.indexOf(gState) != -1,
       "handlers called in the right order: " + name + " is called, " + 
       "gState=" + gState + ", expectedStates=" + expectedStates);
    ok(e.constructor == Event, "event should be an Event");
    ok(e.type == eventName, "event type should be " + eventName);
    ok(e.bubbles, "event should bubble");
    ok(!e.cancelable, "event should not be cancelable");
    ok(e.target == (document instanceof HTMLDocument
                    ? document.body : document.documentElement),
       "the event target should be the body element");
  }
}

function doTest() {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var iosvc = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService2);
  iosvc.manageOfflineStatus = false;
  iosvc.offline = false;
  ok(navigator.onLine, "navigator.onLine should be true, since we've just " +
                       "set nsIIOService.offline to false");

  gState = 0;

  trace("setting iosvc.offline = true");
  iosvc.offline = true;
  trace("done setting iosvc.offline = true");
  ok(!navigator.onLine,
     "navigator.onLine should be false when iosvc.offline == true");
  ok(gState == window.MAX_STATE,
     "offline event: all registered handlers should have been invoked, " +
     "actual: " + gState);

  gState = 0;
  trace("setting iosvc.offline = false");
  iosvc.offline = false;
  trace("done setting iosvc.offline = false");
  ok(navigator.onLine,
     "navigator.onLine should be true when iosvc.offline == false");
  ok(gState == window.MAX_STATE,
     "online event: all registered handlers should have been invoked, " +
     "actual: " + gState);
}
