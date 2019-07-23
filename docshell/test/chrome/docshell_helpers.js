


var imports = [ "SimpleTest", "is", "isnot", "ok", "onerror", "todo", 
  "todo_is", "todo_isnot" ];
for each (var import in imports) {
  window[import] = window.opener.wrappedJSObject[import];
}




const NAV_NONE = 0;
const NAV_BACK = 1;
const NAV_FORWARD = 2;
const NAV_URI = 3;
const NAV_RELOAD = 4;

var gExpectedEvents;          
                              
var gFinalEvent;              
var gUrisNotInBFCache = [];   
                              
var gNavType = NAV_NONE;      
                              
var gOrigMaxTotalViewers =    
  undefined;                  
























































function doPageNavigation(params) {
  
  let back = params.back ? params.back : false;
  let forward = params.forward ? params.forward : false;
  let reload = params.reload ? params.reload : false;
  let uri = params.uri ? params.uri : false;
  let eventsToListenFor = typeof(params.eventsToListenFor) != "undefined" ?
    params.eventsToListenFor : ["pageshow"];
  gExpectedEvents = typeof(params.eventsToListenFor) == "undefined" || 
    eventsToListenFor.length == 0 ? undefined : params.expectedEvents; 
  let preventBFCache = (typeof[params.preventBFCache] == "undefined") ? 
    false : params.preventBFCache;
  let waitOnly = (typeof(params.waitForEventsOnly) == "boolean" 
    && params.waitForEventsOnly);
  
  
  if (back && forward)
    throw "Can't specify both back and forward";
  if (back && uri)
    throw "Can't specify both back and a uri";
  if (forward && uri)
    throw "Can't specify both forward and a uri";
  if (reload && (forward || back || uri))
    throw "Can't specify reload and another navigation type";
  if (!back && !forward && !uri && !reload && !waitOnly)
    throw "Must specify back or foward or reload or uri";
  if (params.onNavComplete && eventsToListenFor.length == 0)
    throw "Can't use onNavComplete when eventsToListenFor == []";
  if (params.preventBFCache && eventsToListenFor.length == 0)
    throw "Can't use preventBFCache when eventsToListenFor == []";
  if (params.preventBFCache && waitOnly)
    throw "Can't prevent bfcaching when only waiting for events";
  if (waitOnly && typeof(params.onNavComplete) == "undefined")
    throw "Must specify onNavComplete when specifying waitForEventsOnly";
  if (waitOnly && (back || forward || reload || uri))
    throw "Can't specify a navigation type when using waitForEventsOnly";
  for each (let anEventType in eventsToListenFor) {
    let eventFound = false;
    if ( (anEventType == "pageshow") && (!gExpectedEvents) )
      eventFound = true;
    for each (let anExpectedEvent in gExpectedEvents) {
      if (anExpectedEvent.type == anEventType)
        eventFound = true;
    }
    if (!eventFound)
      throw "Event type " + anEventType + " is specified in " +
        "eventsToListenFor, but not in expectedEvents";
  }
  
  
  
  gFinalEvent = eventsToListenFor.length == 0 ? true : false;
  
  
  
  for each (let eventType in eventsToListenFor) {
    dump("TEST: registering a listener for " + eventType + " events\n");
    TestWindow.getBrowser().addEventListener(eventType, pageEventListener, 
      true);
  }

  
  if (back) {
    gNavType = NAV_BACK;
    TestWindow.getBrowser().goBack();
  }
  else if (forward) {
    gNavType = NAV_FORWARD;
    TestWindow.getBrowser().goForward();
  }
  else if (uri) {
    gNavType = NAV_URI;
    TestWindow.getBrowser().loadURI(uri);
  }
  else if (reload) {
    gNavType = NAV_RELOAD;
    TestWindow.getBrowser().reload();
  }
  else if (waitOnly) {
    gNavType = NAV_NONE;
  }
  else {
    throw "No valid navigation type passed to doPageNavigation!";
  }
  
  
  
  if (eventsToListenFor.length > 0 && params.onNavComplete)
  {
    waitForTrue(
      function() { return gFinalEvent; },
      function() { 
        doPageNavigation_complete(eventsToListenFor, params.onNavComplete, 
          preventBFCache);
      } );
  }
}







function doPageNavigation_complete(eventsToListenFor, onNavComplete, 
  preventBFCache) {
  
  dump("TEST: removing event listeners\n");
  for each (let eventType in eventsToListenFor) {
    TestWindow.getBrowser().removeEventListener(eventType, pageEventListener, 
      true);
  }
  
  
  
  let uri = TestWindow.getBrowser().currentURI.spec;
  if (preventBFCache) {
    TestWindow.getWindow().addEventListener("unload", function() { 
        dump("TEST: Called dummy unload function to prevent page from " +
          "being bfcached.\n"); 
      }, true);
      
    
    
    if (!(uri in gUrisNotInBFCache)) {
      gUrisNotInBFCache.push(uri);
    }  
  } else if (gNavType == NAV_URI) {
    
    
    gUrisNotInBFCache.forEach(
      function(element, index, array) {
        if (element == uri) {
          array.splice(index, 1);
        }
      }, this);
  }
  
  
  onNavComplete.call();
}






function waitForPageEvents(params) {
  params.waitForEventsOnly = true;
  doPageNavigation(params);
}




function pageEventListener(event) {
  try {
    dump("TEST: eventListener received a " + event.type + " event for page " +
      event.originalTarget.title + ", persisted=" + event.persisted + "\n");
  }catch(e) {
    
  }
  
  
  
  
  
  if ( (event.type == "pageshow") && 
    (gNavType == NAV_BACK || gNavType == NAV_FORWARD) ) {
    let uri = TestWindow.getBrowser().currentURI.spec;
    if (uri in gUrisNotInBFCache) {
      ok(!event.persisted, "pageshow event has .persisted = false, even " +
       "though it was loaded with .preventBFCache previously\n");
    }
  }
  
  
  
  
  if ((typeof(gExpectedEvents) == "undefined") && event.type == "pageshow")
  {
    setTimeout(function() { gFinalEvent = true; }, 0);
    return;
  }
  
  
  
  if (gExpectedEvents.length == 0) {
    ok(false, "Unexpected event (" + event.type + ") occurred");
    return;
  }
  
  
  
  let expected = gExpectedEvents.shift();
  
  is(event.type, expected.type, 
    "A " + expected.type + " event was expected, but a " +
    event.type + " event occurred");
    
  if (typeof(expected.title) != "undefined") {
    ok(event.originalTarget instanceof HTMLDocument,
       "originalTarget for last " + event.type + 
       " event not an HTMLDocument");
    is(event.originalTarget.title, expected.title, 
      "A " + event.type + " event was expected for page " +
      expected.title + ", but was fired for page " + 
      event.originalTarget.title);
  }  
  
  if (typeof(expected.persisted) != "undefined") {
    is(event.persisted, expected.persisted, 
      "The persisted property of the " + event.type + "event on page " +
      event.originalTarget.title + " had an unexpected value"); 
  }

  
  if (gExpectedEvents.length == 0)
    setTimeout(function() { gFinalEvent = true; }, 0);
}




function finish() {
  
  var history = TestWindow.getBrowser().webNavigation.sessionHistory;
  history.PurgeHistory(history.count);
  
  
  
  if (typeof(gOrigMaxTotalViewers) != "undefined") {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                .getService(Components.interfaces.nsIPrefBranch);
    prefs.setIntPref("browser.sessionhistory.max_total_viewers",
      gOrigMaxTotalViewers);
  }

  
  window.close();
  window.opener.wrappedJSObject.SimpleTest.finish();
}


















function waitForTrue(fn, onWaitComplete, timeout) {
  var start = new Date().valueOf();
  if (typeof(timeout) != "undefined") {
    
    
    if (timeout < 500)
      timeout *= 1000;
  }
  
  
  
  var intervalid;
  intervalid =
    setInterval(
      function() {  
        var timeoutHit = false;
        if (typeof(timeout) != "undefined") {
          timeoutHit = new Date().valueOf() - start >= 
            timeout ? true : false;
          if (timeoutHit) {
            ok(false, "Timed out waiting for condition");
          }
        }
        if (timeoutHit || fn.call()) {
          
          clearInterval(intervalid);
          onWaitComplete.call();          
        } 
      }, 20);
}









function enableBFCache(enable) {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
              .getService(Components.interfaces.nsIPrefBranch);
  
  
  
  
  if (typeof(gOrigMaxTotalViewers) == "undefined") {
    gOrigMaxTotalViewers =
      prefs.getIntPref("browser.sessionhistory.max_total_viewers");
  }
  
  if (typeof(enable) == "boolean") {
    if (enable)
      prefs.setIntPref("browser.sessionhistory.max_total_viewers", -1);
    else
      prefs.setIntPref("browser.sessionhistory.max_total_viewers", 0);    
  }
  else if (typeof(enable) == "number") {
    prefs.setIntPref("browser.sessionhistory.max_total_viewers", enable);    
  }
}





function getHttpUrl(filename) {
  return "http://localhost:8888/chrome/docshell/test/chrome/" + filename;
}





var TestWindow = {};
TestWindow.getWindow = function () {
  return document.getElementById("content").contentWindow;
}
TestWindow.getBrowser = function () {
  return document.getElementById("content");
}
TestWindow.getDocument = function () {
  return document.getElementById("content").contentDocument;
}
