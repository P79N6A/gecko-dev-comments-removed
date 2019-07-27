















 

'use strict';

var EXPORTED_SYMBOLS = ['PdfjsChromeUtils'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const PREF_PREFIX = 'pdfjs';
const PDF_CONTENT_TYPE = 'application/pdf';

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

let Svc = {};
XPCOMUtils.defineLazyServiceGetter(Svc, 'mime',
                                   '@mozilla.org/mime;1',
                                   'nsIMIMEService');


var DEFAULT_PREFERENCES = {
  showPreviousViewOnLoad: true,
  defaultZoomValue: '',
  sidebarViewOnLoad: 0,
  enableHandToolOnLoad: false,
  enableWebGL: false,
  disableRange: false,
  disableAutoFetch: false,
  disableFontFace: false,
  disableTextLayer: false,
  useOnlyCssZoom: false
};


let PdfjsChromeUtils = {
  
  
  _allowedPrefNames: Object.keys(DEFAULT_PREFERENCES),
  _ppmm: null,
  _mmg: null,

  



  init: function () {
    if (!this._ppmm) {
      
      this._ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"].getService(Ci.nsIMessageBroadcaster);
      this._ppmm.addMessageListener("PDFJS:Parent:clearUserPref", this);
      this._ppmm.addMessageListener("PDFJS:Parent:setIntPref", this);
      this._ppmm.addMessageListener("PDFJS:Parent:setBoolPref", this);
      this._ppmm.addMessageListener("PDFJS:Parent:setCharPref", this);
      this._ppmm.addMessageListener("PDFJS:Parent:setStringPref", this);
      this._ppmm.addMessageListener("PDFJS:Parent:isDefaultHandlerApp", this);

      
      this._mmg = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
      this._mmg.addMessageListener("PDFJS:Parent:getChromeWindow", this);
      this._mmg.addMessageListener("PDFJS:Parent:getFindBar", this);
      this._mmg.addMessageListener("PDFJS:Parent:displayWarning", this);

      
      Services.obs.addObserver(this, "quit-application", false);
    }
  },

  uninit: function () {
    if (this._ppmm) {
      this._ppmm.removeMessageListener("PDFJS:Parent:clearUserPref", this);
      this._ppmm.removeMessageListener("PDFJS:Parent:setIntPref", this);
      this._ppmm.removeMessageListener("PDFJS:Parent:setBoolPref", this);
      this._ppmm.removeMessageListener("PDFJS:Parent:setCharPref", this);
      this._ppmm.removeMessageListener("PDFJS:Parent:setStringPref", this);
      this._ppmm.removeMessageListener("PDFJS:Parent:isDefaultHandlerApp", this);

      this._mmg.removeMessageListener("PDFJS:Parent:getChromeWindow", this);
      this._mmg.removeMessageListener("PDFJS:Parent:getFindBar", this);
      this._mmg.removeMessageListener("PDFJS:Parent:displayWarning", this);

      Services.obs.removeObserver(this, "quit-application", false);

      this._mmg = null;
      this._ppmm = null;
    }
  },

  





  notifyChildOfSettingsChange: function () {
    if (Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_DEFAULT &&
        this._ppmm) {
      
      
      
      
      
      this._ppmm.broadcastAsyncMessage("PDFJS:Child:refreshSettings", {});
    }
  },

  



  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "quit-application") {
      this.uninit();
    }
  },

  receiveMessage: function (aMsg) {
    switch (aMsg.name) {
      case "PDFJS:Parent:clearUserPref":
        this._clearUserPref(aMsg.data.name);
        break;
      case "PDFJS:Parent:setIntPref":
        this._setIntPref(aMsg.data.name, aMsg.data.value);
        break;
      case "PDFJS:Parent:setBoolPref":
        this._setBoolPref(aMsg.data.name, aMsg.data.value);
        break;
      case "PDFJS:Parent:setCharPref":
        this._setCharPref(aMsg.data.name, aMsg.data.value);
        break;
      case "PDFJS:Parent:setStringPref":
        this._setStringPref(aMsg.data.name, aMsg.data.value);
        break;
      case "PDFJS:Parent:isDefaultHandlerApp":
        return this.isDefaultHandlerApp();
      case "PDFJS:Parent:displayWarning":
        this._displayWarning(aMsg);
        break;

      
      case "PDFJS:Parent:getChromeWindow":
        return this._getChromeWindow(aMsg);
      case "PDFJS:Parent:getFindBar":
        return this._getFindBar(aMsg);
    }
  },

  



  _getChromeWindow: function (aMsg) {
    
    
    let browser = aMsg.target;
    let wrapper = new PdfjsWindowWrapper(browser);
    let suitcase = aMsg.objects.suitcase;
    suitcase.setChromeWindow(wrapper);
    return true;
  },

  _getFindBar: function (aMsg) {
    
    
    let browser = aMsg.target;
    let wrapper = new PdfjsFindbarWrapper(browser);
    let suitcase = aMsg.objects.suitcase;
    suitcase.setFindBar(wrapper);
    return true;
  },

  _ensurePreferenceAllowed: function (aPrefName) {
    let unPrefixedName = aPrefName.split(PREF_PREFIX + '.');
    if (unPrefixedName[0] !== '' ||
        this._allowedPrefNames.indexOf(unPrefixedName[1]) === -1) {
      let msg = "'" + aPrefName + "' ";
      msg += "can't be accessed from content. See PdfjsChromeUtils." 
      throw new Error(msg);
    }
  },

  _clearUserPref: function (aPrefName) {
    this._ensurePreferenceAllowed(aPrefName);
    Services.prefs.clearUserPref(aPrefName);
  },

  _setIntPref: function (aPrefName, aPrefValue) {
    this._ensurePreferenceAllowed(aPrefName);
    Services.prefs.setIntPref(aPrefName, aPrefValue);
  },

  _setBoolPref: function (aPrefName, aPrefValue) {
    this._ensurePreferenceAllowed(aPrefName);
    Services.prefs.setBoolPref(aPrefName, aPrefValue);
  },

  _setCharPref: function (aPrefName, aPrefValue) {
    this._ensurePreferenceAllowed(aPrefName);
    Services.prefs.setCharPref(aPrefName, aPrefValue);
  },

  _setStringPref: function (aPrefName, aPrefValue) {
    this._ensurePreferenceAllowed(aPrefName);
    let str = Cc['@mozilla.org/supports-string;1']
                .createInstance(Ci.nsISupportsString);
    str.data = aPrefValue;
    Services.prefs.setComplexValue(aPrefName, Ci.nsISupportsString, str);
  },

  




  isDefaultHandlerApp: function () {
    var handlerInfo = Svc.mime.getFromTypeAndExtension(PDF_CONTENT_TYPE, 'pdf');
    return !handlerInfo.alwaysAskBeforeHandling &&
           handlerInfo.preferredAction == Ci.nsIHandlerInfo.handleInternally;
  },

  



  _displayWarning: function (aMsg) {
    let json = aMsg.data;
    let browser = aMsg.target;
    let cpowCallback = aMsg.objects.callback;
    let tabbrowser = browser.getTabBrowser();
    let notificationBox = tabbrowser.getNotificationBox(browser);
    
    
    
    let responseSent = false;
    let buttons = [{
      label: json.label,
      accessKey: json.accessKey,
      callback: function() {
        responseSent = true;
        cpowCallback(true);
      }
    }];
    notificationBox.appendNotification(json.message, 'pdfjs-fallback', null,
                                       notificationBox.PRIORITY_INFO_LOW,
                                       buttons,
                                       function eventsCallback(eventType) {
      
      
      if (eventType !== 'removed') {
        return;
      }
      
      
      if (responseSent) {
        return;
      }
      cpowCallback(false);
    });
  }
};







function PdfjsFindbarWrapper(aBrowser) {
  let tabbrowser = aBrowser.getTabBrowser();
  let tab = tabbrowser._getTabForBrowser(aBrowser);
  this._findbar = tabbrowser.getFindBar(tab);
};

PdfjsFindbarWrapper.prototype = {
  __exposedProps__: {
    addEventListener: "r",
    removeEventListener: "r",
    updateControlState: "r",
  },
  _findbar: null,

  updateControlState: function (aResult, aFindPrevious) {
    this._findbar.updateControlState(aResult, aFindPrevious);
  },

  addEventListener: function (aType, aListener, aUseCapture, aWantsUntrusted) {
    this._findbar.addEventListener(aType, aListener, aUseCapture, aWantsUntrusted);
  },

  removeEventListener: function (aType, aListener, aUseCapture) {
    this._findbar.removeEventListener(aType, aListener, aUseCapture);
  }
};

function PdfjsWindowWrapper(aBrowser) {
  this._window = aBrowser.ownerDocument.defaultView;
};

PdfjsWindowWrapper.prototype = {
  __exposedProps__: {
    valueOf: "r",
  },
  _window: null,

  valueOf: function () {
    return this._window.valueOf();
  }
};


