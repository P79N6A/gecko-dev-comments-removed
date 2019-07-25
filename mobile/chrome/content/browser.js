








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const FINDSTATE_FIND = 0;
const FINDSTATE_FIND_AGAIN = 1;
const FINDSTATE_FIND_PREVIOUS = 2;

Cu.import("resource://gre/modules/SpatialNavigation.js");



__defineGetter__("gPrefService", function () {
  delete gPrefService;
  var gPrefService;
  return gPrefService = Components.classes["@mozilla.org/preferences-service;1"]
                                  .getService(Components.interfaces.nsIPrefBranch2);
});

function getBrowser() {
  return Browser.content.browser;
}

var Browser = {
  _content : null,

  startup : function() {
    window.controllers.appendController(this);
    window.controllers.appendController(BrowserUI);

    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    var styleSheets = Cc["@mozilla.org/content/style-sheet-service;1"].getService(Ci.nsIStyleSheetService);

    
    var hideCursor = gPrefService.getBoolPref("browser.ui.cursor") == false;
    if (hideCursor) {
      window.QueryInterface(Ci.nsIDOMChromeWindow).setCursor("none");

      var styleURI = ios.newURI("chrome://browser/content/content.css", null, null);
      styleSheets.loadAndRegisterSheet(styleURI, styleSheets.AGENT_SHEET);
    }

    
    var styleURI = ios.newURI("chrome://browser/content/scrollbars.css", null, null);
    styleSheets.loadAndRegisterSheet(styleURI, styleSheets.AGENT_SHEET);

    this._content = document.getElementById("content");
    this._content.progressListenerCreator = function (content, browser) {
      return new ProgressController(content, browser);
    };

    var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
    os.addObserver(gXPInstallObserver, "xpinstall-install-blocked", false);
    os.addObserver(gXPInstallObserver, "xpinstall-download-started", false);

    BrowserUI.init();

    this._content.addEventListener("command", this._handleContentCommand, false);
    this._content.addEventListener("DOMUpdatePageReport", gPopupBlockerObserver.onUpdatePageReport, false);
    this._content.tabList = document.getElementById("tab-list");
    this._content.newTab(true);

    SpatialNavigation.init(this.content);

    this.setupGeolocationPrompt();

    Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);

    
    
    if (window.arguments && window.arguments[0]) {
      var whereURI = null;

      try {
        
        var cmdLine = window.arguments[0].QueryInterface(Ci.nsICommandLine);

        try {
          
          whereURI = gPrefService.getCharPref("browser.startup.homepage");
        } catch (e) {}

        
        if (cmdLine.length == 1) {
          
          var uri = cmdLine.getArgument(0);
          if (uri != "" && uri[0] != '-') {
            whereURI = cmdLine.resolveURI(uri);
            if (whereURI)
              whereURI = whereURI.spec;
          }
        }

        
        var uriFlag = cmdLine.handleFlagWithParam("url", false);
        if (uriFlag) {
          whereURI = cmdLine.resolveURI(uriFlag);
          if (whereURI)
            whereURI = whereURI.spec;
        }
      } catch (e) {}

      if (whereURI) {
        var self = this;
        setTimeout(function() { self.currentBrowser.loadURI(whereURI, null, null, false); }, 0);
      }

      
      var phs = Cc["@mozilla.org/plugin/host;1"].
                getService(Ci.nsIPluginHost);
      var plugins = phs.getPluginTags({ });
      for (i = 0; i < plugins.length; ++i)
        plugins[i].disabled = true;
    }
  },

  setupGeolocationPrompt: function() {
    try {
      var geolocationService = Cc["@mozilla.org/geolocation/service;1"].getService(Ci.nsIGeolocationService);
    }
    catch (ex) {
      return;
    }

    geolocationService.prompt = function(request) {
      var notificationBox = Browser.getNotificationBox();
      var notification = notificationBox.getNotificationWithValue("geolocation");

      if (!notification) {
        var bundle_browser = document.getElementById("bundle_browser");
        var buttons = [{
            label: bundle_browser.getString("gelocation.exactLocation"),
            subLabel: bundle_browser.getString("gelocation.exactLocation.subLabel"),
            accessKey: bundle_browser.getString("gelocation.exactLocationKey"),
            callback: function(){request.allow()}
          },
          {
            label: bundle_browser.getString("gelocation.neighborhoodLocation"),
            subLabel: bundle_browser.getString("gelocation.neighborhoodLocation.subLabel"),
            accessKey: bundle_browser.getString("gelocation.neighborhoodLocationKey"),
            callback: function(){request.allowButFuzz()}
          },
          {
            label: bundle_browser.getString("gelocation.nothingLocation"),
            subLabel: "",
            accessKey: bundle_browser.getString("gelocation.nothingLocationKey"),
            callback: function(){request.cancel()}
          }];

        var message = bundle_browser.getFormattedString("geolocation.requestMessage", [request.requestingURI.spec]);
        var notification = notificationBox.appendNotification(message,
                             "geolocation", null, 
                             notificationBox.PRIORITY_INFO_HIGH, buttons);
        var children = notification.childNodes;
        for (var b = 0; b < children.length; b++)
          children[b].setAttribute("sublabel", children[b].buttonInfo.subLabel);
        return 1;
      }
    }
  },

  get content() {
    return this._content;
  },

  



  get currentBrowser() {
    return this._content.browser;
  },

  supportsCommand : function(cmd) {
    var isSupported = false;
    switch (cmd) {
      case "cmd_fullscreen":
      case "cmd_downloads":
        isSupported = true;
        break;
      default:
        isSupported = false;
        break;
    }
    return isSupported;
  },

  isCommandEnabled : function(cmd) {
    return true;
  },

  doCommand : function(cmd) {
    var browser = this.content.browser;

    switch (cmd) {
      case "cmd_fullscreen":
        window.fullScreen = !window.fullScreen;
        break;
    }
  },

  getNotificationBox : function() {
    return document.getElementById("notifications");
  },

  findState: FINDSTATE_FIND,
  openFind: function(aState) {
    this.findState = aState;

    var findbar = document.getElementById("findbar");
    var browser = findbar.browser;
    if (!browser) {
      browser = this.content.browser;
      findbar.browser = browser;
    }

    var panel = document.getElementById("findpanel");
    if (panel.state == "open")
      this.doFind(null);
    else
      panel.openPopup(document.getElementById("findpanel-placeholder"), "before_start");
  },

  doFind: function (aEvent) {
    var findbar = document.getElementById("findbar");
    if (Browser.findState == FINDSTATE_FIND)
      findbar.onFindCommand();
    else
      findbar.onFindAgainCommand(Browser.findState == FINDSTATE_FIND_PREVIOUS);
  },

  translatePhoneNumbers: function() {
    let doc = getBrowser().contentDocument;
    let textnodes = doc.evaluate("//text()",
                                 doc,
                                 null,
                                 XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE,
                                 null);
    let s, node, lastLastIndex;
    let re = /(\+?1? ?-?\(?\d{3}\)?[ +-\.]\d{3}[ +-\.]\d{4})/
    for (var i = 0; i < textnodes.snapshotLength; i++) {
      node = textnodes.snapshotItem(i);
      s = node.data;
      if (s.match(re)) {
        s = s.replace(re, "<a href='tel:$1'> $1 </a>");
        try {
          let replacement = doc.createElement("span");
          replacement.innerHTML = s;
          node.parentNode.insertBefore(replacement, node);
          node.parentNode.removeChild(node);
        } catch(e) {
          
        }
      }
    }
  },

  





  _handleContentCommand: function (aEvent) {
    
    if (!aEvent.isTrusted)
      return;

    var ot = aEvent.originalTarget;
    var errorDoc = ot.ownerDocument;

    
    
    if (/^about:neterror\?e=nssBadCert/.test(errorDoc.documentURI)) {
      if (ot == errorDoc.getElementById('exceptionDialogButton')) {
        var params = { exceptionAdded : false };

        try {
          switch (gPrefService.getIntPref("browser.ssl_override_behavior")) {
            case 2 : 
              params.prefetchCert = true;
            case 1 : 
              params.location = errorDoc.location.href;
          }
        } catch (e) {
          Components.utils.reportError("Couldn't get ssl_override pref: " + e);
        }

        window.openDialog('chrome://pippki/content/exceptionDialog.xul',
                          '','chrome,centerscreen,modal', params);

        
        if (params.exceptionAdded)
          errorDoc.location.reload();
      }
      else if (ot == errorDoc.getElementById('getMeOutOfHereButton')) {
        
        var defaultPrefs = Cc["@mozilla.org/preferences-service;1"]
                          .getService(Ci.nsIPrefService).getDefaultBranch(null);
        var url = "about:blank";
        try {
          url = defaultPrefs.getCharPref("browser.startup.homepage");
          
          if (url.indexOf("|") != -1)
            url = url.split("|")[0];
        } catch (e) {  }

        Browser.currentBrowser.loadURI(url, null, null, false);
      }
    }
  }
};

function ProgressController(aTabBrowser, aBrowser) {
  this._tabbrowser = aTabBrowser;
  this.init(aBrowser);
}

ProgressController.prototype = {
  _browser : null,

  init : function(aBrowser) {
    this._browser = aBrowser;
  },

  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
      if (aRequest && aWebProgress.DOMWindow == this._browser.contentWindow) {
        if (aStateFlags & Ci.nsIWebProgressListener.STATE_START) {
          BrowserUI.update(TOOLBARSTATE_LOADING, this._browser);
        }
        else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
          BrowserUI.update(TOOLBARSTATE_LOADED, this._browser);
        }
      }
    }

    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
        aWebProgress.DOMWindow.focus();
        Browser.translatePhoneNumbers();
        this._tabbrowser.updateBrowser(this._browser, true);
        this._tabbrowser.updateCanvasState();
        
      }
    }
  },

  
  
  onProgressChange : function(aWebProgress, aRequest, aCurSelf, aMaxSelf, aCurTotal, aMaxTotal) {
  },

  
  onLocationChange : function(aWebProgress, aRequest, aLocationURI) {

    var location = aLocationURI ? aLocationURI.spec : "";
    this._hostChanged = true;

    
    
    
    
    
    
    
    
    cBrowser = Browser.currentBrowser;
    if (cBrowser.lastURI) {
      var oldSpec = cBrowser.lastURI.spec;
      var oldIndexOfHash = oldSpec.indexOf("#");
      if (oldIndexOfHash != -1)
        oldSpec = oldSpec.substr(0, oldIndexOfHash);
      var newSpec = location;
      var newIndexOfHash = newSpec.indexOf("#");
      if (newIndexOfHash != -1)
        newSpec = newSpec.substr(0, newSpec.indexOf("#"));
      if (newSpec != oldSpec) {
        
        
        var nBox = Browser.getNotificationBox();
        nBox.removeTransientNotifications();
      }
    }
    cBrowser.lastURI = aLocationURI;


    if (aWebProgress.DOMWindow == this._browser.contentWindow) {
      BrowserUI.setURI();
      this._tabbrowser.updateBrowser(this._browser, false);
    }
  },

  
  
  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage) {
  },

 
  _state: null,
  _host: undefined,
  _hostChanged: false, 

  
  onSecurityChange : function(aWebProgress, aRequest, aState) {

    
    
    if (this._state == aState &&
        !this._hostChanged) {
      return;
    }
    this._state = aState;

    try {
      this._host = getBrowser().contentWindow.location.host;
    } catch(ex) {
      this._host = null;
    }

    this._hostChanged = false;

    
    
    
    var location = getBrowser().contentWindow.location;
    var locationObj = {};
    try {
      locationObj.host = location.host;
      locationObj.hostname = location.hostname;
      locationObj.port = location.port;
    } catch (ex) {
      
      
      
    }
    getIdentityHandler().checkIdentity(this._state, locationObj);

  },

  QueryInterface : function(aIID) {
    if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};




function IdentityHandler() {
  this._stringBundle = document.getElementById("bundle_browser");
  this._staticStrings = {};
  this._staticStrings[this.IDENTITY_MODE_DOMAIN_VERIFIED] = {
    encryption_label: this._stringBundle.getString("identity.encrypted")
  };
  this._staticStrings[this.IDENTITY_MODE_IDENTIFIED] = {
    encryption_label: this._stringBundle.getString("identity.encrypted")
  };
  this._staticStrings[this.IDENTITY_MODE_UNKNOWN] = {
    encryption_label: this._stringBundle.getString("identity.unencrypted")
  };

  this._cacheElements();
}

IdentityHandler.prototype = {

  
  IDENTITY_MODE_IDENTIFIED       : "verifiedIdentity", 
  IDENTITY_MODE_DOMAIN_VERIFIED  : "verifiedDomain",   
  IDENTITY_MODE_UNKNOWN          : "unknownIdentity",  

  
  _lastStatus : null,
  _lastLocation : null,

  


  _cacheElements : function() {
    this._identityPopup = document.getElementById("identity-popup");
    this._identityBox = document.getElementById("identity-box");
    this._identityPopupContentBox = document.getElementById("identity-popup-content-box");
    this._identityPopupContentHost = document.getElementById("identity-popup-content-host");
    this._identityPopupContentOwner = document.getElementById("identity-popup-content-owner");
    this._identityPopupContentSupp = document.getElementById("identity-popup-content-supplemental");
    this._identityPopupContentVerif = document.getElementById("identity-popup-content-verifier");
    this._identityPopupEncLabel = document.getElementById("identity-popup-encryption-label");
  },

  



  handleMoreInfoClick : function(event) {
    displaySecurityInfo();
    event.stopPropagation();
  },

  



  getIdentityData : function() {
    var result = {};
    var status = this._lastStatus.QueryInterface(Components.interfaces.nsISSLStatus);
    var cert = status.serverCert;

    
    result.subjectOrg = cert.organization;

    
    if (cert.subjectName) {
      result.subjectNameFields = {};
      cert.subjectName.split(",").forEach(function(v) {
        var field = v.split("=");
        this[field[0]] = field[1];
      }, result.subjectNameFields);

      
      result.city = result.subjectNameFields.L;
      result.state = result.subjectNameFields.ST;
      result.country = result.subjectNameFields.C;
    }

    
    result.caOrg =  cert.issuerOrganization || cert.issuerCommonName;
    result.cert = cert;

    return result;
  },

  








  checkIdentity : function(state, location) {
    var currentStatus = getBrowser().securityUI
                                .QueryInterface(Components.interfaces.nsISSLStatusProvider)
                                .SSLStatus;
    this._lastStatus = currentStatus;
    this._lastLocation = location;

    if (state & Components.interfaces.nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL)
      this.setMode(this.IDENTITY_MODE_IDENTIFIED);
    else if (state & Components.interfaces.nsIWebProgressListener.STATE_SECURE_HIGH)
      this.setMode(this.IDENTITY_MODE_DOMAIN_VERIFIED);
    else
      this.setMode(this.IDENTITY_MODE_UNKNOWN);
  },

  


  getEffectiveHost : function() {
    
    if (!this._eTLDService)
      this._eTLDService = Cc["@mozilla.org/network/effective-tld-service;1"]
                         .getService(Ci.nsIEffectiveTLDService);
    try {
      return this._eTLDService.getBaseDomainFromHost(this._lastLocation.hostname);
    } catch (e) {
      
      
      return this._lastLocation.hostname;
    }
  },

  



  setMode : function(newMode) {
    if (!this._identityBox) {
      
      
      return;
    }

    this._identityBox.className = newMode;
    this.setIdentityMessages(newMode);

    
    if (this._identityPopup.state == "open")
      this.setPopupMessages(newMode);
  },

  





  setIdentityMessages : function(newMode) {
    if (newMode == this.IDENTITY_MODE_DOMAIN_VERIFIED) {
      var iData = this.getIdentityData();

      
      
      var lookupHost = this._lastLocation.host;
      if (lookupHost.indexOf(':') < 0)
        lookupHost += ":443";

      
      if (!this._overrideService)
        this._overrideService = Components.classes["@mozilla.org/security/certoverride;1"]
                                          .getService(Components.interfaces.nsICertOverrideService);

      
      
      var tooltip = this._stringBundle.getFormattedString("identity.identified.verifier",
                                                          [iData.caOrg]);

      
      
      
      
      if (this._overrideService.hasMatchingOverride(this._lastLocation.hostname,
                                                    (this._lastLocation.port || 443),
                                                    iData.cert, {}, {}))
        tooltip = this._stringBundle.getString("identity.identified.verified_by_you");
    }
    else if (newMode == this.IDENTITY_MODE_IDENTIFIED) {
      
      iData = this.getIdentityData();
      tooltip = this._stringBundle.getFormattedString("identity.identified.verifier",
                                                      [iData.caOrg]);
    }
    else {
      tooltip = this._stringBundle.getString("identity.unknown.tooltip");
    }

    
    this._identityBox.tooltipText = tooltip;
  },

  






  setPopupMessages : function(newMode) {

    this._identityPopup.className = newMode;
    this._identityPopupContentBox.className = newMode;

    
    this._identityPopupEncLabel.textContent = this._staticStrings[newMode].encryption_label;

    
    var supplemental = "";
    var verifier = "";

    if (newMode == this.IDENTITY_MODE_DOMAIN_VERIFIED) {
      var iData = this.getIdentityData();
      var host = this.getEffectiveHost();
      var owner = this._stringBundle.getString("identity.ownerUnknown2");
      verifier = this._identityBox.tooltipText;
      supplemental = "";
    }
    else if (newMode == this.IDENTITY_MODE_IDENTIFIED) {
      
      iData = this.getIdentityData();
      host = this.getEffectiveHost();
      owner = iData.subjectOrg;
      verifier = this._identityBox.tooltipText;

      
      if (iData.city)
        supplemental += iData.city + "\n";
      if (iData.state && iData.country)
        supplemental += this._stringBundle.getFormattedString("identity.identified.state_and_country",
                                                              [iData.state, iData.country]);
      else if (iData.state) 
        supplemental += iData.state;
      else if (iData.country) 
        supplemental += iData.country;
    }
    else {
      
      host = "";
      owner = "";
    }

    
    this._identityPopupContentHost.textContent = host;
    this._identityPopupContentOwner.textContent = owner;
    this._identityPopupContentSupp.textContent = supplemental;
    this._identityPopupContentVerif.textContent = verifier;
  },

  hideIdentityPopup : function() {
    this._identityPopup.hidePopup();
  },

  


  handleIdentityButtonEvent : function(event) {

    event.stopPropagation();

    if ((event.type == "click" && event.button != 0) ||
        (event.type == "keypress" && event.charCode != KeyEvent.DOM_VK_SPACE &&
         event.keyCode != KeyEvent.DOM_VK_RETURN))
      return; 

    
    
    this._identityPopup.hidden = false;

    
    this._identityPopup.popupBoxObject
        .setConsumeRollupEvent(Ci.nsIPopupBoxObject.ROLLUP_CONSUME);

    
    this.setPopupMessages(this._identityBox.className);

    
    this._identityPopup.openPopup(this._identityBox, 'after_start');
  }
};

var gIdentityHandler;





function getIdentityHandler() {
  if (!gIdentityHandler)
    gIdentityHandler = new IdentityHandler();
  return gIdentityHandler;
}





const gPopupBlockerObserver = {
  _kIPM: Components.interfaces.nsIPermissionManager,

  onUpdatePageReport: function (aEvent)
  {
    var cBrowser = Browser.currentBrowser;
    if (aEvent.originalTarget != cBrowser)
      return;

    if (!cBrowser.pageReport)
      return;

    
    
    
    if (!cBrowser.pageReport.reported) {
      if(gPrefService.getBoolPref("privacy.popups.showBrowserMessage")) {
        var bundle_browser = document.getElementById("bundle_browser");
        var brandBundle = document.getElementById("bundle_brand");
        var brandShortName = brandBundle.getString("brandShortName");
        var message;
        var popupCount = cBrowser.pageReport.length;

        if (popupCount > 1)
          message = bundle_browser.getFormattedString("popupWarningMultiple", [brandShortName, popupCount]);
        else
          message = bundle_browser.getFormattedString("popupWarning", [brandShortName]);

        var notificationBox = Browser.getNotificationBox();
        var notification = notificationBox.getNotificationWithValue("popup-blocked");
        if (notification) {
          notification.label = message;
        }
        else {
          var buttons = [
            {
              label: bundle_browser.getString("popupButtonAlwaysAllow"),
              accessKey: bundle_browser.getString("popupButtonAlwaysAllow.accesskey"),
              callback: function() { gPopupBlockerObserver.toggleAllowPopupsForSite(); }
            },
            {
              label: bundle_browser.getString("popupButtonNeverWarn"),
              accessKey: bundle_browser.getString("popupButtonNeverWarn.accesskey"),
              callback: function() { gPopupBlockerObserver.dontShowMessage(); }
            }
          ];

          const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
          notificationBox.appendNotification(message, "popup-blocked",
                                             "",
                                             priority, buttons);
        }
      }
      
      
      cBrowser.pageReport.reported = true;
    }
  },

  toggleAllowPopupsForSite: function (aEvent)
  {
    var currentURI = Browser.currentBrowser.webNavigation.currentURI;
    var pm = Components.classes["@mozilla.org/permissionmanager;1"]
                       .getService(this._kIPM);
    pm.add(currentURI, "popup", this._kIPM.ALLOW_ACTION);

    Browser.getNotificationBox().removeCurrentNotification();
  },

  dontShowMessage: function ()
  {
    var showMessage = gPrefService.getBoolPref("privacy.popups.showBrowserMessage");
    gPrefService.setBoolPref("privacy.popups.showBrowserMessage", !showMessage);
    Browser.getNotificationBox().removeCurrentNotification();
  }
};

const gXPInstallObserver = {
  observe: function (aSubject, aTopic, aData)
  {
    var brandBundle = document.getElementById("bundle_brand");
    var browserBundle = document.getElementById("bundle_browser");
    switch (aTopic) {
      case "xpinstall-install-blocked":
        var installInfo = aSubject.QueryInterface(Components.interfaces.nsIXPIInstallInfo);
        var host = installInfo.originatingURI.host;
        var brandShortName = brandBundle.getString("brandShortName");
        var notificationName, messageString, buttons;
        if (!gPrefService.getBoolPref("xpinstall.enabled")) {
          notificationName = "xpinstall-disabled"
          if (gPrefService.prefIsLocked("xpinstall.enabled")) {
            messageString = browserBundle.getString("xpinstallDisabledMessageLocked");
            buttons = [];
          }
          else {
            messageString = browserBundle.getFormattedString("xpinstallDisabledMessage",
                                                             [brandShortName, host]);
            buttons = [{
              label: browserBundle.getString("xpinstallDisabledButton"),
              accessKey: browserBundle.getString("xpinstallDisabledButton.accesskey"),
              popup: null,
              callback: function editPrefs() {
                gPrefService.setBoolPref("xpinstall.enabled", true);
                return false;
              }
            }];
          }
        }
        else {
          notificationName = "xpinstall"
          messageString = browserBundle.getFormattedString("xpinstallPromptWarning",
                                                           [brandShortName, host]);

          buttons = [{
            label: browserBundle.getString("xpinstallPromptAllowButton"),
            accessKey: browserBundle.getString("xpinstallPromptAllowButton.accesskey"),
            popup: null,
            callback: function() {
              
              CommandUpdater.doCommand("cmd_addons");

              var mgr = Cc["@mozilla.org/xpinstall/install-manager;1"].createInstance(Ci.nsIXPInstallManager);
              mgr.initManagerWithInstallInfo(installInfo);
              return false;
            }
          }];
        }

        var nBox = Browser.getNotificationBox();
        if (!nBox.getNotificationWithValue(notificationName)) {
          const priority = nBox.PRIORITY_WARNING_MEDIUM;
          const iconURL = "chrome://mozapps/skin/update/update.png";
          nBox.appendNotification(messageString, notificationName, iconURL, priority, buttons);
        }
        break;
    }
  }
};

function getNotificationBox(aWindow) {
  return Browser.getNotificationBox();
}
