


















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
  pdfBugEnabled: false,
  disableRange: false,
  disableStream: false,
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
    this._browsers = new Set();
    if (!this._ppmm) {
      
      this._ppmm = Cc['@mozilla.org/parentprocessmessagemanager;1'].
        getService(Ci.nsIMessageBroadcaster);
      this._ppmm.addMessageListener('PDFJS:Parent:clearUserPref', this);
      this._ppmm.addMessageListener('PDFJS:Parent:setIntPref', this);
      this._ppmm.addMessageListener('PDFJS:Parent:setBoolPref', this);
      this._ppmm.addMessageListener('PDFJS:Parent:setCharPref', this);
      this._ppmm.addMessageListener('PDFJS:Parent:setStringPref', this);
      this._ppmm.addMessageListener('PDFJS:Parent:isDefaultHandlerApp', this);

      
      this._mmg = Cc['@mozilla.org/globalmessagemanager;1'].
        getService(Ci.nsIMessageListenerManager);
      this._mmg.addMessageListener('PDFJS:Parent:displayWarning', this);

      this._mmg.addMessageListener('PDFJS:Parent:addEventListener', this);
      this._mmg.addMessageListener('PDFJS:Parent:removeEventListener', this);
      this._mmg.addMessageListener('PDFJS:Parent:updateControlState', this);

      
      Services.obs.addObserver(this, 'quit-application', false);
    }
  },

  uninit: function () {
    if (this._ppmm) {
      this._ppmm.removeMessageListener('PDFJS:Parent:clearUserPref', this);
      this._ppmm.removeMessageListener('PDFJS:Parent:setIntPref', this);
      this._ppmm.removeMessageListener('PDFJS:Parent:setBoolPref', this);
      this._ppmm.removeMessageListener('PDFJS:Parent:setCharPref', this);
      this._ppmm.removeMessageListener('PDFJS:Parent:setStringPref', this);
      this._ppmm.removeMessageListener('PDFJS:Parent:isDefaultHandlerApp',
                                       this);

      this._mmg.removeMessageListener('PDFJS:Parent:displayWarning', this);

      this._mmg.removeMessageListener('PDFJS:Parent:addEventListener', this);
      this._mmg.removeMessageListener('PDFJS:Parent:removeEventListener', this);
      this._mmg.removeMessageListener('PDFJS:Parent:updateControlState', this);

      Services.obs.removeObserver(this, 'quit-application', false);

      this._mmg = null;
      this._ppmm = null;
    }
  },

  





  notifyChildOfSettingsChange: function () {
    if (Services.appinfo.processType ===
        Services.appinfo.PROCESS_TYPE_DEFAULT && this._ppmm) {
      
      
      
      
      
      this._ppmm.broadcastAsyncMessage('PDFJS:Child:refreshSettings', {});
    }
  },

  



  observe: function(aSubject, aTopic, aData) {
    if (aTopic === 'quit-application') {
      this.uninit();
    }
  },

  receiveMessage: function (aMsg) {
    switch (aMsg.name) {
      case 'PDFJS:Parent:clearUserPref':
        this._clearUserPref(aMsg.data.name);
        break;
      case 'PDFJS:Parent:setIntPref':
        this._setIntPref(aMsg.data.name, aMsg.data.value);
        break;
      case 'PDFJS:Parent:setBoolPref':
        this._setBoolPref(aMsg.data.name, aMsg.data.value);
        break;
      case 'PDFJS:Parent:setCharPref':
        this._setCharPref(aMsg.data.name, aMsg.data.value);
        break;
      case 'PDFJS:Parent:setStringPref':
        this._setStringPref(aMsg.data.name, aMsg.data.value);
        break;
      case 'PDFJS:Parent:isDefaultHandlerApp':
        return this.isDefaultHandlerApp();
      case 'PDFJS:Parent:displayWarning':
        this._displayWarning(aMsg);
        break;


      case 'PDFJS:Parent:updateControlState':
        return this._updateControlState(aMsg);
      case 'PDFJS:Parent:addEventListener':
        return this._addEventListener(aMsg);
      case 'PDFJS:Parent:removeEventListener':
        return this._removeEventListener(aMsg);
    }
  },

  



  _findbarFromMessage: function(aMsg) {
    let browser = aMsg.target;
    let tabbrowser = browser.getTabBrowser();
    let tab = tabbrowser.getTabForBrowser(browser);
    return tabbrowser.getFindBar(tab);
  },

  _updateControlState: function (aMsg) {
    let data = aMsg.data;
    this._findbarFromMessage(aMsg)
        .updateControlState(data.result, data.findPrevious);
  },

  handleEvent: function(aEvent) {
    
    
    let type = aEvent.type;
    let detail = {
      query: aEvent.detail.query,
      caseSensitive: aEvent.detail.caseSensitive,
      highlightAll: aEvent.detail.highlightAll,
      findPrevious: aEvent.detail.findPrevious
    };

    let browser = aEvent.currentTarget.browser;
    if (!this._browsers.has(browser)) {
      throw new Error('FindEventManager was not bound ' +
                      'for the current browser.');
    }
    
    let mm = browser.messageManager;
    mm.sendAsyncMessage('PDFJS:Child:handleEvent',
                        { type: type, detail: detail });
    aEvent.preventDefault();
  },

  _types: ['find',
           'findagain',
           'findhighlightallchange',
           'findcasesensitivitychange'],

  _addEventListener: function (aMsg) {
    let browser = aMsg.target;
    if (this._browsers.has(browser)) {
      throw new Error('FindEventManager was bound 2nd time ' +
                      'without unbinding it first.');
    }

    
    
    this._browsers.add(browser);

    
    for (var i = 0; i < this._types.length; i++) {
      var type = this._types[i];
      this._findbarFromMessage(aMsg)
          .addEventListener(type, this, true);
    }
  },

  _removeEventListener: function (aMsg) {
    let browser = aMsg.target;
    if (!this._browsers.has(browser)) {
      throw new Error('FindEventManager was unbound without binding it first.');
    }

    this._browsers.delete(browser);

    
    for (var i = 0; i < this._types.length; i++) {
      var type = this._types[i];
      this._findbarFromMessage(aMsg)
          .removeEventListener(type, this, true);
    }
  },

  _ensurePreferenceAllowed: function (aPrefName) {
    let unPrefixedName = aPrefName.split(PREF_PREFIX + '.');
    if (unPrefixedName[0] !== '' ||
        this._allowedPrefNames.indexOf(unPrefixedName[1]) === -1) {
      let msg = '"' + aPrefName + '" ' +
                'can\'t be accessed from content. See PdfjsChromeUtils.';
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
    return (!handlerInfo.alwaysAskBeforeHandling &&
            handlerInfo.preferredAction === Ci.nsIHandlerInfo.handleInternally);
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


