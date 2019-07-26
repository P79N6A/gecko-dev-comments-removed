







const kUniqueURI = Services.io.newURI("http://mochi.test:8888/#bug_680727",
                                      null, null);
var gAsyncHistory = 
  Cc["@mozilla.org/browser/history;1"].getService(Ci.mozIAsyncHistory);

let proxyPrefValue;

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();

  
  
  proxyPrefValue = Services.prefs.getIntPref("network.proxy.type");
  Services.prefs.setIntPref("network.proxy.type", 0);

  
  Components.classes["@mozilla.org/netwerk/cache-storage-service;1"]
            .getService(Components.interfaces.nsICacheStorageService)
            .clear();

  
  Services.io.offline = true;
  window.addEventListener("DOMContentLoaded", errorListener, false);
  content.location = kUniqueURI.spec;
}



function errorListener() {
  if(content.location == "about:blank") {
    info("got about:blank, which is expected once, so return");
    return;
  }

  window.removeEventListener("DOMContentLoaded", errorListener, false);
  ok(Services.io.offline, "Services.io.offline is true.");

  
  is(gBrowser.contentDocument.documentURI.substring(0, 27),
     "about:neterror?e=netOffline",
     "Document URI is the error page.");

  
  is(content.location.href, kUniqueURI.spec,
     "Docshell URI is the original URI.");

  
  promiseAsyncUpdates().then(function() {
    gAsyncHistory.isURIVisited(kUniqueURI, errorAsyncListener);
  });
}

function errorAsyncListener(aURI, aIsVisited) {
  ok(kUniqueURI.equals(aURI) && !aIsVisited,
     "The neterror page is not listed in global history.");

  Services.prefs.setIntPref("network.proxy.type", proxyPrefValue);

  
  Services.io.offline = false;

  window.addEventListener("DOMContentLoaded", reloadListener, false);

  ok(gBrowser.contentDocument.getElementById("errorTryAgain"),
     "The error page has got a #errorTryAgain element");
  gBrowser.contentDocument.getElementById("errorTryAgain").click();
}



function reloadListener() {
  window.removeEventListener("DOMContentLoaded", reloadListener, false);

  
  
  
  ok(!Services.io.offline, "Services.io.offline is false.");

  
  is(gBrowser.contentDocument.documentURI, kUniqueURI.spec, 
     "Document URI is not the offline-error page, but the original URI.");

  
  promiseAsyncUpdates().then(function() {
    gAsyncHistory.isURIVisited(kUniqueURI, reloadAsyncListener);
  });
}

function reloadAsyncListener(aURI, aIsVisited) {
  ok(kUniqueURI.equals(aURI) && aIsVisited, "We have visited the URI.");
  promiseClearHistory().then(finish);
}

registerCleanupFunction(function() {
  Services.prefs.setIntPref("network.proxy.type", proxyPrefValue);
  Services.io.offline = false;
  window.removeEventListener("DOMContentLoaded", errorListener, false);
  window.removeEventListener("DOMContentLoaded", reloadListener, false);
  gBrowser.removeCurrentTab();
});
