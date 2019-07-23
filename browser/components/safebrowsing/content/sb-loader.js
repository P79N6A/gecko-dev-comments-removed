# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Google Safe Browsing.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Fritz Schneider <fritz@google.com> (original author)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****







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

    var malwareWarden = new appContext.PROT_MalwareWarden();
    safebrowsing.malwareWarden = malwareWarden;

    
    malwareWarden.registerBlackTable("goog-malware-sha128");

    malwareWarden.maybeToggleUpdateChecking();

    

    safebrowsing.progressListener.QueryInterface(Ci.nsIWebProgressListener);
    var phishWarden = new appContext.PROT_PhishingWarden(
        safebrowsing.progressListener, getBrowser());
    safebrowsing.phishWarden = phishWarden;

    
    
    
    phishWarden.registerWhiteTable("goog-white-exp");
    phishWarden.registerBlackTable("goog-phish-sha128");

    
    phishWarden.maybeToggleUpdateChecking();
    safebrowsing.controller = new appContext.PROT_Controller(
        window, getBrowser(), phishWarden);

    
    
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
    if (safebrowsing.malwareWarden) {
      safebrowsing.malwareWarden.shutdown();
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
