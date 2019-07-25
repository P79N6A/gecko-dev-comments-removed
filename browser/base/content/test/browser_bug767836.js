




const pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
const PREF = "browser.newtab.url";
const NEWTABURL = Services.prefs.getCharPref(PREF) || "about:blank";
const TESTURL = "http://example.com/";
  
function test() {

  waitForExplicitFinish();
  
  ok(!pb.privateBrowsingEnabled, "private browsing is disabled");
  
  ok(!Services.prefs.prefHasUserValue(PREF), "No custom newtab url is set");
  
  openNewTab(function () {
    
    is(gBrowser.selectedBrowser.currentURI.spec, NEWTABURL,
       "URL of NewTab should be browser.newtab.url in Normal mode");
    
    Services.prefs.setCharPref(PREF, TESTURL);
    ok(Services.prefs.prefHasUserValue(PREF), "Custom newtab url is set");
    
    
    openNewTab(function () {
      is(gBrowser.selectedBrowser.currentURI.spec, TESTURL,
         "URL of NewTab should be the custom url");
      
      
      Services.prefs.clearUserPref(PREF);
      ok(!Services.prefs.prefHasUserValue(PREF), "No custom newtab url is set");
      
      
      togglePrivateBrowsing(function () {
        ok(pb.privateBrowsingEnabled, "private browsing is enabled");
        
        
        openNewTab(function () {
          
          is(gBrowser.selectedBrowser.currentURI.spec, "about:privatebrowsing",
             "URL of NewTab should be about:privatebrowsing in PB mode");
          
          Services.prefs.setCharPref(PREF, TESTURL);
          ok(Services.prefs.prefHasUserValue(PREF), "Custom newtab url is set");
          
          
          openNewTab(function () {
            is(gBrowser.selectedBrowser.currentURI.spec, TESTURL,
               "URL of NewTab should be the custom url");

            Services.prefs.clearUserPref(PREF);
            ok(!Services.prefs.prefHasUserValue(PREF), "No custom newtab url is set");
            
            
            togglePrivateBrowsing(function () {
              ok(!pb.privateBrowsingEnabled, "private browsing is disabled");
              
              gBrowser.removeTab(gBrowser.selectedTab);
              gBrowser.removeTab(gBrowser.selectedTab);
              finish();
            });
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
