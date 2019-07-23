



































 



let cookieManager = Cc["@mozilla.org/cookiemanager;1"].
                    getService(Ci.nsICookieManager2);
let pb = Cc["@mozilla.org/privatebrowsing;1"].
         getService(Ci.nsIPrivateBrowsingService);
let _obs = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
let observerNotified = 0,  firstUnloadFired = 0, secondUnloadFired = 0;

let pbObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "private-browsing") {
      switch(aData) {
        case "enter":
          observerNotified++;
          is(observerNotified, 1, "This should be the first notification");
          is(firstUnloadFired, 1, "The first unload event should have been processed by now");
          break;
        case "exit":
          _obs.removeObserver(this, "private-browsing");
          observerNotified++;
          is(observerNotified, 2, "This should be the second notification");
          is(secondUnloadFired, 1, "The second unload event should have been processed by now");
          break;
      }
    }
  }
}
function test() {
  waitForExplicitFinish();
  _obs.addObserver(pbObserver, "private-browsing", false);
  is(gBrowser.tabContainer.childNodes.length, 1, "There should only be one tab");
  let testTab = gBrowser.addTab();
  gBrowser.selectedTab = testTab;
  testTab.linkedBrowser.addEventListener("unload", (function() {
    testTab.linkedBrowser.removeEventListener("unload", arguments.callee, true);
    firstUnloadFired++;
    is(observerNotified, 0, "The notification shouldn't have been sent yet");
  }), true);

  pb.privateBrowsingEnabled = true;
  let testTab = gBrowser.addTab();
  gBrowser.selectedTab = testTab;
  testTab.linkedBrowser.addEventListener("unload", (function() {
    testTab.linkedBrowser.removeEventListener("unload", arguments.callee, true);
    secondUnloadFired++;
    is(observerNotified, 1, "The notification shouldn't have been sent yet");
    cookieManager.add("example.com", "test/", "PB", "1", false, false, false, 1000000000000);
  }), true);

  pb.privateBrowsingEnabled = false;
  gBrowser.tabContainer.lastChild.linkedBrowser.addEventListener("unload", (function() {
    gBrowser.tabContainer.lastChild.linkedBrowser.removeEventListener("unload", arguments.callee, true);
    let count = cookieManager.countCookiesFromHost("example.com");
    is(count, 0, "There shouldn't be any cookies once pb mode has exited");
    cookieManager.QueryInterface(Ci.nsICookieManager);
    cookieManager.remove("example.com", "PB", "test/", false);
  }), true);
  gBrowser.removeCurrentTab();
  finish();
}
