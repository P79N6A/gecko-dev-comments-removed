# -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
# The Original Code is Firefox Anti-Phishing Support.
#
# The Initial Developer of the Original Code is
# Mozilla Corporation.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Jeff Walden <jwalden+code@mit.edu>   (original author)
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












 
var gPhishDialog = {
  


  _webProgress: null,

  


  init: function ()
  {
    const Cc = Components.classes, Ci = Components.interfaces;

    var providerNum = window.arguments[0].providerNum;

    var phishBefore = document.getElementById("phishBefore");

    var prefb = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefService).
                getBranch("browser.safebrowsing.provider.");

    
    
    
    var providerName = prefb.getComplexValue(providerNum + ".name", Ci.nsISupportsString).data
    var strings = document.getElementById("bundle_phish");
    phishBefore.textContent = strings.getFormattedString("phishBeforeText", [providerName]);

    
    
    var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].
                    getService(Ci.nsIURLFormatter);
    var privacyURL = formatter.formatURLPref("browser.safebrowsing.provider." +
                                             providerNum +
                                             ".privacy.url",
                                             null);
    var fallbackURL = formatter.formatURLPref("browser.safebrowsing.provider." +
                                              providerNum +
                                              ".privacy.fallbackurl",
                                              null);
    this._progressListener._providerFallbackURL = fallbackURL;

    
    var frame = document.getElementById("phishPolicyFrame");
    var webProgress = frame.docShell
                           .QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebProgress);
    webProgress.addProgressListener(this._progressListener,
                                    Ci.nsIWebProgress.NOTIFY_STATE_WINDOW);

    this._webProgress = webProgress; 

    
    const loadFlags = Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    frame.webNavigation.loadURI(privacyURL, loadFlags, null, null, null);
  },

  



  _progressListener:
  {
    




    _providerLoadFailed: false,
    _providerFallbackLoadFailed: false,
    
    _tryLoad: function(url) {
      const Ci = Components.interfaces;
      const loadFlags = Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
      var frame = document.getElementById("phishPolicyFrame");
      frame.webNavigation.loadURI(url, loadFlags, null, null, null);
    },

    onStateChange: function (aWebProgress, aRequest, aStateFlags, aStatus)
    {
      
      const Ci = Components.interfaces, Cr = Components.results;
      if ((aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) &&
          (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)) {
        
        if (Components.isSuccessCode(aRequest.status)) {
          try {
            aRequest.QueryInterface(Ci.nsIHttpChannel);
          } catch (e) {
            
            
            return;
          }

          
          if (200 == aRequest.responseStatus)
            return;
        }

        
        if (!this._providerLoadFailed) {
          this._provderLoadFailed = true;
          
          this._tryLoad(this._providerFallbackURL);
        } else if (!this._providerFallbackLoadFailed) {
          
          this._providerFallbackLoadFailed = true;

          
          const fallbackURL = "chrome://browser/content/preferences/fallbackEULA.xhtml";
          this._tryLoad(fallbackURL);

          
          document.getElementById("acceptOrDecline").disabled = true;
        } else {
          throw "Fallback policy failed to load -- what the hay!?!";
        }
      }
    },
    
    onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress,
                               aMaxSelfProgress, aCurTotalProgress,
                               aMaxTotalProgress)
    {
    },

    onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage)
    {
    },

    QueryInterface : function(aIID)
    {
      const Ci = Components.interfaces;
      if (aIID.equals(Ci.nsIWebProgressListener) ||
          aIID.equals(Ci.nsISupportsWeakReference) ||
          aIID.equals(Ci.nsISupports))
        return this;
      throw Components.results.NS_NOINTERFACE;
    }
  },

  




  accept: function ()
  {
    window.arguments[0].userAgreed = true;
  },

  


  uninit: function ()
  {
    
    this._webProgress.removeProgressListener(this._progressListener);
    this._progressListener = this._webProgress = null;
  },

  


  onchangeRadio: function ()
  {
    var radio = document.getElementById("acceptOrDecline");
    document.documentElement.getButton("accept").disabled = (radio.value == "false");
  }
};
