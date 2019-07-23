




































 
var reporterListener = {

  QueryInterface: function(aIID) {
    if (aIID.equals(Components.interfaces.nsIWebProgressListener)   ||
        aIID.equals(Components.interfaces.nsIWebProgressListener2)  ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },

  onLocationChange: function(aProgress, aRequest, aURI) {
    var broadcaster = document.getElementById("reporterItemsBroadcaster");
    var isEnabled = false;

    if (aURI instanceof Components.interfaces.nsIURI) {
      switch (aURI.scheme) {
        case "http":
        case "https":
        case "ftp":
        case "gopher":
          isEnabled = true;
      }
    }

    broadcaster.setAttribute("disabled", !isEnabled);
  },

  onStateChange: function() {  },
  onProgressChange: function() {  },
  onStatusChange: function() {  },
  onSecurityChange: function() {  },
  onProgressChange64: function() { },
  onRefreshAttempted: function() { return true; }
}

function onBrowserLoad() {
  if ("undefined" != typeof(gBrowser))
    gBrowser.addProgressListener(reporterListener);
}

function loadReporterWizard() {
  var browser = getBrowser();
  var charSet = browser.contentDocument.characterSet;
  var url = browser.currentURI.spec;
  window.openDialog("chrome://reporter/content/reportWizard.xul", "",
                    "chrome,centerscreen,dialog",
                    url,
                    charSet);
  return true;
}

window.addEventListener("load", onBrowserLoad, false);
