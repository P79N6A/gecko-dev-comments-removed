



"use strict";

this.EXPORTED_SYMBOLS = ["ContentRestore"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "DocShellCapabilities",
  "resource:///modules/sessionstore/DocShellCapabilities.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FormData",
  "resource://gre/modules/FormData.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PageStyle",
  "resource:///modules/sessionstore/PageStyle.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ScrollPosition",
  "resource://gre/modules/ScrollPosition.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionHistory",
  "resource:///modules/sessionstore/SessionHistory.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionStorage",
  "resource:///modules/sessionstore/SessionStorage.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Utils",
  "resource:///modules/sessionstore/Utils.jsm");



































function ContentRestore(chromeGlobal) {
  let internal = new ContentRestoreInternal(chromeGlobal);
  let external = {};

  let EXPORTED_METHODS = ["restoreHistory",
                          "restoreTabContent",
                          "restoreDocument",
                          "resetRestore",
                          "getRestoreEpoch",
                         ];

  for (let method of EXPORTED_METHODS) {
    external[method] = internal[method].bind(internal);
  }

  return Object.freeze(external);
}

function ContentRestoreInternal(chromeGlobal) {
  this.chromeGlobal = chromeGlobal;

  
  

  
  this._epoch = 0;

  
  
  this._tabData = null;

  
  
  
  this._restoringDocument = null;

  
  
  this._historyListener = null;

  
  
  this._progressListener = null;
}





ContentRestoreInternal.prototype = {

  get docShell() {
    return this.chromeGlobal.docShell;
  },

  





  restoreHistory: function (epoch, tabData, reloadCallback) {
    this._tabData = tabData;
    this._epoch = epoch;

    
    let webNavigation = this.docShell.QueryInterface(Ci.nsIWebNavigation);
    webNavigation.stop(Ci.nsIWebNavigation.STOP_ALL);

    
    
    
    let activeIndex = tabData.index - 1;
    let activePageData = tabData.entries[activeIndex] || {};
    let uri = activePageData.url || null;
    if (uri) {
      webNavigation.setCurrentURI(Utils.makeURI(uri));
    }

    SessionHistory.restore(this.docShell, tabData);

    
    let listener = new HistoryListener(this.docShell, reloadCallback);
    webNavigation.sessionHistory.addSHistoryListener(listener);
    this._historyListener = listener;

    
    
    let disallow = new Set(tabData.disallow && tabData.disallow.split(","));
    DocShellCapabilities.restore(this.docShell, disallow);

    if (tabData.storage && this.docShell instanceof Ci.nsIDocShell) {
      SessionStorage.restore(this.docShell, tabData.storage);
      delete tabData.storage;
    }
  },

  



  restoreTabContent: function (loadArguments, finishCallback) {
    let tabData = this._tabData;
    this._tabData = null;

    let webNavigation = this.docShell.QueryInterface(Ci.nsIWebNavigation);
    let history = webNavigation.sessionHistory;

    
    this._historyListener.uninstall();
    this._historyListener = null;

    
    
    let progressListener = new ProgressListener(this.docShell, () => {
      
      
      
      this.resetRestore();

      finishCallback();
    });
    this._progressListener = progressListener;

    
    
    
    webNavigation.setCurrentURI(Utils.makeURI("about:blank"));

    try {
      if (loadArguments) {
        
        
        let activeIndex = tabData.index - 1;
        if (activeIndex > 0) {
          
          history.getEntryAtIndex(activeIndex, true);
        }
        let referrer = loadArguments.referrer ?
                       Utils.makeURI(loadArguments.referrer) : null;
        webNavigation.loadURI(loadArguments.uri, loadArguments.flags,
                              referrer, null, null);
      } else if (tabData.userTypedValue && tabData.userTypedClear) {
        
        
        
        let activeIndex = tabData.index - 1;
        if (activeIndex > 0) {
          
          history.getEntryAtIndex(activeIndex, true);
        }

        
        webNavigation.loadURI(tabData.userTypedValue,
                              Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP,
                              null, null, null);
      } else if (tabData.entries.length) {
        
        let activeIndex = tabData.index - 1;
        this._restoringDocument = {entry: tabData.entries[activeIndex] || {},
                                   formdata: tabData.formdata || {},
                                   pageStyle: tabData.pageStyle || {},
                                   scrollPositions: tabData.scroll || {}};

        
        
        
        history.getEntryAtIndex(activeIndex, true);
        history.reloadCurrentEntry();
      } else {
        
        webNavigation.loadURI("about:blank",
                              Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_HISTORY,
                              null, null, null);
      }

      return true;
    } catch (ex if ex instanceof Ci.nsIException) {
      
      
      return false;
    }
  },

  








  getFramesToRestore: function (content, data) {
    function hasExpectedURL(aDocument, aURL) {
      return !aURL || aURL.replace(/#.*/, "") == aDocument.location.href.replace(/#.*/, "");
    }

    let frameList = [];

    function enumerateFrame(content, data) {
      
      if (!hasExpectedURL(content.document, data.url)) {
        return;
      }

      frameList.push([content, data]);

      for (let i = 0; i < content.frames.length; i++) {
        if (data.children && data.children[i]) {
          enumerateFrame(content.frames[i], data.children[i]);
        }
      }
    }

    enumerateFrame(content, data);

    return frameList;
  },

  




  restoreDocument: function () {
    this._epoch = 0;

    if (!this._restoringDocument) {
      return;
    }
    let {entry, pageStyle, formdata, scrollPositions} = this._restoringDocument;
    this._restoringDocument = null;

    let window = this.docShell.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);
    let frameList = this.getFramesToRestore(window, entry);

    
    if (typeof(pageStyle) === "string") {
      PageStyle.restore(this.docShell, frameList, pageStyle);
    } else {
      PageStyle.restoreTree(this.docShell, pageStyle);
    }

    FormData.restoreTree(window, formdata);
    ScrollPosition.restoreTree(window, scrollPositions);

    
    for (let [frame, data] of frameList) {
      if (data.hasOwnProperty("formdata") || data.hasOwnProperty("innerHTML")) {
        let formdata = data.formdata || {};
        formdata.url = data.url;

        if (data.hasOwnProperty("innerHTML")) {
          formdata.innerHTML = data.innerHTML;
        }

        FormData.restore(frame, formdata);
      }

      ScrollPosition.restore(frame, data.scroll || "");
    }
  },

  








  resetRestore: function () {
    this._tabData = null;

    if (this._historyListener) {
      this._historyListener.uninstall();
    }
    this._historyListener = null;

    if (this._progressListener) {
      this._progressListener.uninstall();
    }
    this._progressListener = null;
  },

  



  getRestoreEpoch: function () {
    return this._epoch;
  },
};






function HistoryListener(docShell, callback) {
  let webNavigation = docShell.QueryInterface(Ci.nsIWebNavigation);
  webNavigation.sessionHistory.addSHistoryListener(this);

  this.webNavigation = webNavigation;
  this.callback = callback;
}
HistoryListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsISHistoryListener,
    Ci.nsISupportsWeakReference
  ]),

  uninstall: function () {
    let shistory = this.webNavigation.sessionHistory;
    if (shistory) {
      shistory.removeSHistoryListener(this);
    }
  },

  OnHistoryNewEntry: function(newURI) {},
  OnHistoryGoBack: function(backURI) { return true; },
  OnHistoryGoForward: function(forwardURI) { return true; },
  OnHistoryGotoIndex: function(index, gotoURI) { return true; },
  OnHistoryPurge: function(numEntries) { return true; },
  OnHistoryReplaceEntry: function(index) {},

  OnHistoryReload: function(reloadURI, reloadFlags) {
    this.callback();

    
    return false;
  },
}







function ProgressListener(docShell, callback)
{
  let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIWebProgress);
  webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATE_WINDOW);

  this.webProgress = webProgress;
  this.callback = callback;
}
ProgressListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIWebProgressListener,
    Ci.nsISupportsWeakReference
  ]),

  uninstall: function() {
    this.webProgress.removeProgressListener(this);
  },

  onStateChange: function(webProgress, request, stateFlags, status) {
    if (webProgress.isTopLevel &&
        stateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        stateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) {
      this.callback();
    }
  },

  onLocationChange: function() {},
  onProgressChange: function() {},
  onStatusChange: function() {},
  onSecurityChange: function() {},
};
