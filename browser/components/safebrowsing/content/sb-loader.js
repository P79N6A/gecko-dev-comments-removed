









































var safebrowsing = {
  controller: null,
  phishWarden: null,

  
  
  
  progressListener: null,
  progressListenerCallback: {
    requests: [],
    onDocNavStart: function(request, url) {
      this.requests.push({
        'request': request,
        'url': url
      });
    }
  },

  startup: function() {
    setTimeout(safebrowsing.deferredStartup, 2000);

    
    window.removeEventListener("load", safebrowsing.startup, false);
  },
  
  deferredStartup: function() {
    var appContext = Cc["@mozilla.org/safebrowsing/application;1"]
                     .getService().wrappedJSObject;

    

    var contentArea = document.getElementById("content");

    safebrowsing.progressListener.QueryInterface(Ci.nsIWebProgressListener);
    var phishWarden = new appContext.PROT_PhishingWarden(
          safebrowsing.progressListener, document.getElementById("content"));
    safebrowsing.phishWarden = phishWarden;

    
    
    
    phishWarden.registerWhiteTable("goog-white-domain");
    phishWarden.registerWhiteTable("goog-white-url");
    phishWarden.registerBlackTable("goog-black-url");
    phishWarden.registerBlackTable("goog-black-enchash");

    
    phishWarden.maybeToggleUpdateChecking();
    var tabWatcher = new appContext.G_TabbedBrowserWatcher(
        contentArea,
        "safebrowsing-watcher",
        true );
    safebrowsing.controller = new appContext.PROT_Controller(
        window,
        tabWatcher,
        phishWarden);

    
    
    safebrowsing.progressListener.globalProgressListenerEnabled = false;
    
    
    
    
    if (!phishWarden.phishWardenEnabled_) {
      safebrowsing.progressListenerCallback.requests = null;
      safebrowsing.progressListenerCallback.onDocNavStart = null;
      safebrowsing.progressListenerCallback = null;
      safebrowsing.progressListener = null;
      return;
    }

    var pendingRequests = safebrowsing.progressListenerCallback.requests;
    for (var i = 0; i < pendingRequests.length; ++i) {
      var request = pendingRequests[i].request;
      var url = pendingRequests[i].url;

      phishWarden.onDocNavStart(request, url);
    }
    
    safebrowsing.progressListenerCallback.requests = null;
    safebrowsing.progressListenerCallback.onDocNavStart = null;
    safebrowsing.progressListenerCallback = null;
    safebrowsing.progressListener = null;
  },

  


  shutdown: function() {
    if (safebrowsing.controller) {
      
      safebrowsing.controller.shutdown();
    }
    if (safebrowsing.phishWarden) {
      safebrowsing.phishWarden.shutdown();
    }
    
    window.removeEventListener("unload", safebrowsing.shutdown, false);
  },

  setReportPhishingMenu: function() {
    var uri = getBrowser().currentURI;
    if (!uri)
      return;

    var sbIconElt = document.getElementById("safebrowsing-urlbar-icon");
    var helpMenuElt = document.getElementById("helpMenu");
    var phishLevel = sbIconElt.getAttribute("level");

    
    document.getElementById("menu_HelpPopup_reportPhishingtoolmenu")
            .hidden = ("safe" != phishLevel);
    document.getElementById("menu_HelpPopup_reportPhishingErrortoolmenu")
            .hidden = ("safe" == phishLevel);

    var broadcasterId;
    if ("safe" == phishLevel) {
      broadcasterId = "reportPhishingBroadcaster";
    } else {
      broadcasterId = "reportPhishingErrorBroadcaster";
    }

    var broadcaster = document.getElementById(broadcasterId);
    if (!broadcaster)
      return;

    var progressListener =
      Cc["@mozilla.org/browser/safebrowsing/navstartlistener;1"]
      .createInstance(Ci.nsIDocNavStartProgressListener);
    broadcaster.setAttribute("disabled", progressListener.isSpurious(uri));
  },
  
  




  getReportURL: function(name) {
    var appContext = Cc["@mozilla.org/safebrowsing/application;1"]
                     .getService().wrappedJSObject;
    var reportUrl = appContext.getReportURL(name);

    var pageUrl = getBrowser().currentURI.asciiSpec;
    reportUrl += "&url=" + encodeURIComponent(pageUrl);

    return reportUrl;
  }
}




safebrowsing.progressListener =
  Components.classes["@mozilla.org/browser/safebrowsing/navstartlistener;1"]
            .createInstance(Components.interfaces.nsIDocNavStartProgressListener);
safebrowsing.progressListener.callback =
  safebrowsing.progressListenerCallback;
safebrowsing.progressListener.globalProgressListenerEnabled = true;
safebrowsing.progressListener.delay = 0;

window.addEventListener("load", safebrowsing.startup, false);
window.addEventListener("unload", safebrowsing.shutdown, false);
