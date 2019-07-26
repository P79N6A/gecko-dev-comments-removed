


XPCOMUtils.defineLazyGetter(this, "docShell", () => {
  return window.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIWebNavigation)
               .QueryInterface(Ci.nsIDocShell);
});

const EXPECTED_REFLOWS = [
  
  "stop@chrome://global/content/bindings/browser.xml|" +
    "addTab@chrome://browser/content/tabbrowser.xml|",

  
  "adjustTabstrip@chrome://browser/content/tabbrowser.xml|" +
    "_handleNewTab@chrome://browser/content/tabbrowser.xml|" +
    "onxbltransitionend@chrome://browser/content/tabbrowser.xml|",

  
  "updateCurrentBrowser@chrome://browser/content/tabbrowser.xml|" +
    "onselect@chrome://browser/content/browser.xul|",

  
  "openLinkIn@chrome://browser/content/utilityOverlay.js|" +
    "openUILinkIn@chrome://browser/content/utilityOverlay.js|" +
    "BrowserOpenTab@chrome://browser/content/browser.js|",

  
  "get_scrollPosition@chrome://global/content/bindings/scrollbox.xml|" +
    "_fillTrailingGap@chrome://browser/content/tabbrowser.xml|" +
    "_handleNewTab@chrome://browser/content/tabbrowser.xml|" +
    "onxbltransitionend@chrome://browser/content/tabbrowser.xml|"
];

const PREF_PRELOAD = "browser.newtab.preload";





function test() {
  waitForExplicitFinish();

  Services.prefs.setBoolPref(PREF_PRELOAD, false);
  registerCleanupFunction(() => Services.prefs.clearUserPref(PREF_PRELOAD));

  
  docShell.addWeakReflowObserver(observer);
  BrowserOpenTab();

  
  waitForTransitionEnd(function () {
    
    docShell.removeWeakReflowObserver(observer);
    gBrowser.removeCurrentTab();

    finish();
  });
}

let observer = {
  reflow: function (start, end) {
    
    let path = (new Error().stack).split("\n").slice(1).map(line => {
      return line.replace(/:\d+$/, "");
    }).join("|");

    
    if (path === "") {
      return;
    }

    
    for (let stack of EXPECTED_REFLOWS) {
      if (path.startsWith(stack)) {
        ok(true, "expected uninterruptible reflow '" + stack + "'");
        return;
      }
    }

    ok(false, "unexpected uninterruptible reflow '" + path + "'");
  },

  reflowInterruptible: function (start, end) {
    
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIReflowObserver,
                                         Ci.nsISupportsWeakReference])
};

function waitForTransitionEnd(callback) {
  let tab = gBrowser.selectedTab;
  tab.addEventListener("transitionend", function onEnd(event) {
    if (event.propertyName === "max-width") {
      tab.removeEventListener("transitionend", onEnd);
      executeSoon(callback);
    }
  });
}
