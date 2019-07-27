






















const gSSService = Cc["@mozilla.org/ssservice;1"]
                     .getService(Ci.nsISiteSecurityService);
const gIOService = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);

const kPinningDomain = "include-subdomains.pinning-dynamic.example.com";
const khpkpPinninEnablePref = "security.cert_pinning.process_headers_from_non_builtin_roots";
const kpkpEnforcementPref = "security.cert_pinning.enforcement_level";
const kBadPinningDomain = "bad.include-subdomains.pinning-dynamic.example.com";
const kURLPath = "/browser/browser/base/content/test/general/pinning_headers.sjs?";

function test() {
  waitForExplicitFinish();
  
  
  Services.prefs.setIntPref(kpkpEnforcementPref, 2);
  Services.prefs.setBoolPref(khpkpPinninEnablePref, true);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(kpkpEnforcementPref);
    Services.prefs.clearUserPref(khpkpPinninEnablePref);
    let uri = gIOService.newURI("https://" + kPinningDomain, null, null);
    gSSService.removeState(Ci.nsISiteSecurityService.HEADER_HPKP, uri, 0);
  });
  whenNewTabLoaded(window, loadPinningPage);
}


function loadPinningPage() {
  gBrowser.selectedBrowser.addEventListener("load",
                                             successfulPinningPageListener,
                                             true);

  gBrowser.selectedBrowser.loadURI("https://" + kPinningDomain + kURLPath + "valid");
}



let successfulPinningPageListener = {
  handleEvent: function() {
    gBrowser.selectedBrowser.removeEventListener("load", this, true);
    gBrowser.addProgressListener(certErrorProgressListener);
    gBrowser.selectedBrowser.loadURI("https://" + kBadPinningDomain);
  }
};



let certErrorProgressListener = {
  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
      let textElement = content.document.getElementById("errorShortDescText");
      let text = textElement.innerHTML;
      ok(text.indexOf("mozilla_pkix_error_key_pinning_failure") > 0,
         "Got a pinning error page");
      gBrowser.removeProgressListener(this);
      gBrowser.selectedBrowser.addEventListener("load",
                                                successfulPinningRemovalPageListener,
                                                true);
      gBrowser.selectedBrowser.loadURI("https://" + kPinningDomain + kURLPath + "zeromaxagevalid");
    }
  }
};



let successfulPinningRemovalPageListener = {
  handleEvent: function() {
    gBrowser.selectedBrowser.removeEventListener("load", this, true);
    gBrowser.selectedBrowser.addEventListener("load",
                                              successfulLoadListener,
                                              true);

    gBrowser.selectedBrowser.loadURI("https://" + kBadPinningDomain);
  }
};



let successfulLoadListener = {
  handleEvent: function() {
    gBrowser.selectedBrowser.removeEventListener("load", this, true);
    gBrowser.removeTab(gBrowser.selectedTab);
    ok(true, "load complete");
    finish();
  }
};
