# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var safebrowsing = {
  startup: function() {
    setTimeout(function() {
      safebrowsing.deferredStartup();
    }, 2000);
    window.removeEventListener("load", safebrowsing.startup, false);
  },

  deferredStartup: function() {
    this.appContext.initialize();
  },

  setReportPhishingMenu: function() {
      
    
    var isPhishingPage = /^about:blocked\?e=phishingBlocked/.test(content.document.documentURI);
    
    
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

    var uri = getBrowser().currentURI;
    if (uri && (uri.schemeIs("http") || uri.schemeIs("https")))
      broadcaster.removeAttribute("disabled");
    else
      broadcaster.setAttribute("disabled", true);
  },
  
  


  get appContext() {
    delete this.appContext;
    return this.appContext = Cc["@mozilla.org/safebrowsing/application;1"]
                            .getService().wrappedJSObject;
  },

  




  getReportURL: function(name) {
    var reportUrl = this.appContext.getReportURL(name);

    var pageUrl = getBrowser().currentURI.asciiSpec;
    reportUrl += "&url=" + encodeURIComponent(pageUrl);

    return reportUrl;
  }
}

window.addEventListener("load", safebrowsing.startup, false);
