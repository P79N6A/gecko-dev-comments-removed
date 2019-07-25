






const pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

const PREF = "browser.newtab.url";

function test() {
  let newTabUrl = Services.prefs.getCharPref(PREF) || "about:blank";

  waitForExplicitFinish();
  
  ok(!pb.privateBrowsingEnabled, "private browsing is disabled");

  
  openNewTab(function () {
    
    is(gBrowser.selectedBrowser.currentURI.spec, newTabUrl,
       "URL of NewTab should be browser.newtab.url in Normal mode");

    
    togglePrivateBrowsing(function () {
      ok(pb.privateBrowsingEnabled, "private browsing is enabled");
    
      
      openNewTab(function () {
        
        is(gBrowser.selectedBrowser.currentURI.spec, "about:privatebrowsing",
           "URL of NewTab should be about:privatebrowsing in PB mode");

        
        togglePrivateBrowsing(function () {
          ok(!pb.privateBrowsingEnabled, "private browsing is disabled");
          
          
          
          openNewTab(function () {
            
            is(gBrowser.selectedBrowser.currentURI.spec, newTabUrl, 
               "URL of NewTab should be browser.newtab.url in Normal mode");
            gBrowser.removeTab(gBrowser.selectedTab);
            gBrowser.removeTab(gBrowser.selectedTab);
            finish();
          });
        });
      });
    });
  });
}

function togglePrivateBrowsing(aCallback) {
  let topic = "private-browsing-transition-complete";

  Services.obs.addObserver(function observe() {
    Services.obs.removeObserver(observe, topic);
    executeSoon(aCallback);
  }, topic, false);

  pb.privateBrowsingEnabled = !pb.privateBrowsingEnabled;
}

function openNewTab(aCallback) {
  
  BrowserOpenTab();

  let browser = gBrowser.selectedBrowser;
  if (browser.contentDocument.readyState == "complete") {
    executeSoon(aCallback);
    return;
  }

  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    executeSoon(aCallback);
  }, true);
}
