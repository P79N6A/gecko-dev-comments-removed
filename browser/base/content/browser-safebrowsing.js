# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

#ifdef MOZ_SAFE_BROWSING
var gSafeBrowsing = {

  setReportPhishingMenu: function() {
    
    var uri = getBrowser().currentURI;
    var isPhishingPage = uri && uri.spec.startsWith("about:blocked?e=phishingBlocked");

    
    document.getElementById("menu_HelpPopup_reportPhishingtoolmenu")
            .hidden = isPhishingPage;
    document.getElementById("menu_HelpPopup_reportPhishingErrortoolmenu")
            .hidden = !isPhishingPage;

    var broadcasterId = isPhishingPage
                        ? "reportPhishingErrorBroadcaster"
                        : "reportPhishingBroadcaster";

    var broadcaster = document.getElementById(broadcasterId);
    if (!broadcaster)
      return;

    if (uri && (uri.schemeIs("http") || uri.schemeIs("https")))
      broadcaster.removeAttribute("disabled");
    else
      broadcaster.setAttribute("disabled", true);
  },

  




  getReportURL: function(name) {
    var reportUrl = SafeBrowsing.getReportURL(name);

    var pageUri = gBrowser.currentURI.clone();

    
    if (pageUri instanceof Ci.nsIURL)
      pageUri.query = '';

    reportUrl += "&url=" + encodeURIComponent(pageUri.asciiSpec);

    return reportUrl;
  }
}
#endif
