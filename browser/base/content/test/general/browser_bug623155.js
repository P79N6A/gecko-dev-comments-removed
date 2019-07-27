


const REDIRECT_FROM = "https://example.com/browser/browser/base/content/test/general/" +
                      "redirect_bug623155.sjs";

const REDIRECT_TO = "https://www.bank1.com/"; 

function isRedirectedURISpec(aURISpec) {
  return isRedirectedURI(Services.io.newURI(aURISpec, null, null));
}

function isRedirectedURI(aURI) {
  
  return Services.io.newURI(REDIRECT_TO, null, null)
                 .equalsExceptRef(aURI);
}




























var gNewTab;

function test() {
  waitForExplicitFinish();

  
  gNewTab = gBrowser.addTab(REDIRECT_FROM + "#BG");
  gBrowser.getBrowserForTab(gNewTab)
          .webProgress
          .addProgressListener(gWebProgressListener,
                               Components.interfaces.nsIWebProgress
                                                    .NOTIFY_LOCATION);
}

var gWebProgressListener = {
  QueryInterface: function(aIID) {
    if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },

  
  
  
  
  
  
  
  

  onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags) {
    if (!aRequest) {
      
      return;
    }

    ok(gNewTab, "There is a new tab.");
    ok(isRedirectedURI(aLocation),
       "onLocationChange catches only redirected URI.");

    if (aLocation.ref == "BG") {
      
      isnot(gNewTab, gBrowser.selectedTab, "This is a background tab.");
    } else if (aLocation.ref == "FG") {
      
      is(gNewTab, gBrowser.selectedTab, "This is a foreground tab.");
    }
    else {
      
      ok(false, "This URI hash is not expected:" + aLocation.ref);
    }

    let isSelectedTab = gNewTab.selected;
    setTimeout(delayed, 0, isSelectedTab);
  }
};

function delayed(aIsSelectedTab) {
  
  if (!aIsSelectedTab) {
    gBrowser.selectedTab = gNewTab;
  }

  ok(isRedirectedURISpec(content.location.href),
     "The content area is redirected. aIsSelectedTab:" + aIsSelectedTab);
  is(gURLBar.value, content.location.href,
     "The URL bar shows the content URI. aIsSelectedTab:" + aIsSelectedTab);

  if (!aIsSelectedTab) {
    
    gBrowser.selectedBrowser.loadURI(REDIRECT_FROM + "#FG");
  }
  else {
    
    finish();
  }
}


registerCleanupFunction(function() {
  if (gNewTab) {
    gBrowser.getBrowserForTab(gNewTab)
            .webProgress
            .removeProgressListener(gWebProgressListener);

    gBrowser.removeTab(gNewTab);
  }
  gNewTab = null;
});
