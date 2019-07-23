


















































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const CID = Components.ID("{5280606b-2510-4fe0-97ef-9b5a22eafe6b}");
const CONTRACT_ID = "@mozilla.org/browser/sessionstore;1";
const CLASS_NAME = "Browser Session Store Service";

const STATE_STOPPED = 0;
const STATE_RUNNING = 1;
const STATE_QUITTING = -1;

const STATE_STOPPED_STR = "stopped";
const STATE_RUNNING_STR = "running";

const PRIVACY_NONE = 0;
const PRIVACY_ENCRYPTED = 1;
const PRIVACY_FULL = 2;


const OBSERVING = [
  "domwindowopened", "domwindowclosed",
  "quit-application-requested", "quit-application-granted",
  "quit-application", "browser:purge-session-history"
];





const WINDOW_ATTRIBUTES = ["width", "height", "screenX", "screenY", "sizemode"];





const WINDOW_HIDEABLE_FEATURES = [
  "menubar", "toolbar", "locationbar", 
  "personalbar", "statusbar", "scrollbars"
];






const CAPABILITIES = [
  "Subframes", "Plugins", "Javascript", "MetaRedirects", "Images"
];


var EVAL_SANDBOX = new Components.utils.Sandbox("about:blank");

function debug(aMsg) {
  aMsg = ("SessionStore: " + aMsg).replace(/\S{80}/g, "$&\n");
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService)
                                     .logStringMessage(aMsg);
}



function SessionStoreService() {
}

SessionStoreService.prototype = {

  
  xulAttributes: [],

  
  _loadState: STATE_STOPPED,

  
  _interval: 10000,

  
  _resume_from_crash: true,

  
  _lastSaveTime: 0, 

  
  _windows: {},

  
  _lastWindowClosed: null,

  
  _dirtyWindows: {},

  
  _dirty: false,



  


  init: function sss_init(aWindow) {
    if (!aWindow || this._loadState == STATE_RUNNING) {
      
      
      if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
        this.onLoad(aWindow);
      return;
    }

    this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefService).getBranch("browser.");
    this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);

    
    if (!this._prefBranch.getBoolPref("sessionstore.enabled"))
      return;

    var observerService = Cc["@mozilla.org/observer-service;1"].
                          getService(Ci.nsIObserverService);

    OBSERVING.forEach(function(aTopic) {
      observerService.addObserver(this, aTopic, true);
    }, this);
    
    
    this._interval = this._prefBranch.getIntPref("sessionstore.interval");
    this._prefBranch.addObserver("sessionstore.interval", this, true);
    
    
    this._resume_from_crash = this._prefBranch.getBoolPref("sessionstore.resume_from_crash");
    this._prefBranch.addObserver("sessionstore.resume_from_crash", this, true);
    
    
    this._prefBranch.addObserver("sessionstore.max_tabs_undo", this, true);

    
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    this._sessionFile = dirService.get("ProfD", Ci.nsILocalFile);
    this._sessionFileBackup = this._sessionFile.clone();
    this._sessionFile.append("sessionstore.js");
    this._sessionFileBackup.append("sessionstore.bak");
   
    
    var iniString;
    try {
      var ss = Cc["@mozilla.org/browser/sessionstartup;1"].
               getService(Ci.nsISessionStartup);
      if (ss.doRestore())
        iniString = ss.state;
    }
    catch(ex) { dump(ex + "\n"); } 

    if (iniString) {
      try {
        
        this._initialState = this._safeEval(iniString);
        
        this._lastSessionCrashed =
          this._initialState.session && this._initialState.session.state &&
          this._initialState.session.state == STATE_RUNNING_STR;
        
        
        WINDOW_ATTRIBUTES.forEach(function(aAttr) {
          delete this._initialState.windows[0][aAttr];
        }, this);
        delete this._initialState.windows[0].hidden;
      }
      catch (ex) { debug("The session file is invalid: " + ex); }
    }
    
    
    if (this._lastSessionCrashed) {
      try {
        this._writeFile(this._sessionFileBackup, iniString);
      }
      catch (ex) { } 
    }

    
    if (!this._resume_from_crash)
      this._clearDisk();
    
    
    this.onLoad(aWindow);
  },

  



  _uninit: function sss_uninit() {
    if (this._doResumeSession()) { 
      this.saveState(true);
    }
    else { 
      this._clearDisk();
    }
    
    if (this._saveTimer) {
      this._saveTimer.cancel();
      this._saveTimer = null;
    }
  },

  


  observe: function sss_observe(aSubject, aTopic, aData) {
    
    var _this = this;

    switch (aTopic) {
    case "domwindowopened": 
      aSubject.addEventListener("load", function(aEvent) {
        aEvent.currentTarget.removeEventListener("load", arguments.callee, false);
        _this.onLoad(aEvent.currentTarget);
        }, false);
      break;
    case "domwindowclosed": 
      this.onClose(aSubject);
      break;
    case "quit-application-requested":
      
      this._forEachBrowserWindow(function(aWindow) {
        this._collectWindowData(aWindow);
      });
      this._dirtyWindows = [];
      this._dirty = false;
      break;
    case "quit-application-granted":
      
      this._loadState = STATE_QUITTING;
      break;
    case "quit-application":
      if (aData == "restart")
        this._prefBranch.setBoolPref("sessionstore.resume_session_once", true);
      this._loadState = STATE_QUITTING; 
      this._uninit();
      break;
    case "browser:purge-session-history": 
      this._forEachBrowserWindow(function(aWindow) {
        Array.forEach(aWindow.getBrowser().browsers, function(aBrowser) {
          delete aBrowser.parentNode.__SS_data;
        });
      });
      this._lastWindowClosed = null;
      this._clearDisk();
      
      for (ix in this._windows) {
        this._windows[ix]._closedTabs = [];
      }
      
      var win = this._getMostRecentBrowserWindow();
      if (win)
        win.setTimeout(function() { _this.saveState(true); }, 0);
      else
        this.saveState(true);
      break;
    case "nsPref:changed": 
      switch (aData) {
      
      
      case "sessionstore.max_tabs_undo":
        var ix;
        for (ix in this._windows) {
          this._windows[ix]._closedTabs.splice(this._prefBranch.getIntPref("sessionstore.max_tabs_undo"));
        }
        break;
      case "sessionstore.interval":
        this._interval = this._prefBranch.getIntPref("sessionstore.interval");
        
        if (this._saveTimer) {
          this._saveTimer.cancel();
          this._saveTimer = null;
        }
        this.saveStateDelayed(null, -1);
        break;
      case "sessionstore.resume_from_crash":
        this._resume_from_crash = this._prefBranch.getBoolPref("sessionstore.resume_from_crash");
        
        
        if (this._resume_from_crash)
          this.saveState(true);
        else if (this._loadState == STATE_RUNNING)
          this._clearDisk();
        break;
      }
      break;
    case "timer-callback": 
      this._saveTimer = null;
      this.saveState();
      break;
    }
  },



  


  handleEvent: function sss_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "load":
      case "pageshow":
        this.onTabLoad(aEvent.currentTarget.ownerDocument.defaultView, aEvent.currentTarget, aEvent);
        break;
      case "input":
      case "DOMAutoComplete":
        this.onTabInput(aEvent.currentTarget.ownerDocument.defaultView, aEvent.currentTarget, aEvent);
        break;
      case "TabOpen":
      case "TabClose":
        var panelID = aEvent.originalTarget.linkedPanel;
        var tabpanel = aEvent.originalTarget.ownerDocument.getElementById(panelID);
        if (aEvent.type == "TabOpen") {
          this.onTabAdd(aEvent.currentTarget.ownerDocument.defaultView, tabpanel);
        }
        else {
          this.onTabClose(aEvent.currentTarget.ownerDocument.defaultView, aEvent.originalTarget);
          this.onTabRemove(aEvent.currentTarget.ownerDocument.defaultView, tabpanel);
        }
        break;
      case "TabSelect":
        var tabpanels = aEvent.currentTarget.mPanelContainer;
        this.onTabSelect(aEvent.currentTarget.ownerDocument.defaultView, tabpanels);
        break;
    }
  },

  








  onLoad: function sss_onLoad(aWindow) {
    
    if (aWindow && aWindow.__SSi && this._windows[aWindow.__SSi])
      return;

    var _this = this;

    
    if (aWindow.document.documentElement.getAttribute("windowtype") != "navigator:browser" ||
      this._loadState == STATE_QUITTING)
      return;

    
    aWindow.__SSi = "window" + Date.now();

    
    this._windows[aWindow.__SSi] = { tabs: [], selected: 0, _closedTabs: [] };
    
    
    if (this._loadState == STATE_STOPPED) {
      this._loadState = STATE_RUNNING;
      this._lastSaveTime = Date.now();
      
      
      
      this.saveStateDelayed(aWindow, 10000);

      
      if (this._initialState) {
        
        this._initialState._firstTabs = true;
        this.restoreWindow(aWindow, this._initialState, this._isCmdLineEmpty(aWindow));
        delete this._initialState;
      }
      
      if (this._lastSessionCrashed) {
        
        aWindow.setTimeout(function(){ _this.retryDownloads(aWindow); }, 0);
      }
    }
    
    var tabbrowser = aWindow.getBrowser();
    var tabpanels = tabbrowser.mPanelContainer;
    
    
    for (var i = 0; i < tabpanels.childNodes.length; i++) {
      this.onTabAdd(aWindow, tabpanels.childNodes[i], true);
    }
    
    tabbrowser.addEventListener("TabOpen", this, true);
    tabbrowser.addEventListener("TabClose", this, true);
    tabbrowser.addEventListener("TabSelect", this, true);
  },

  






  onClose: function sss_onClose(aWindow) {
    
    if (!aWindow.__SSi || !this._windows[aWindow.__SSi]) {
      return;
    }
    
    var tabbrowser = aWindow.getBrowser();
    var tabpanels = tabbrowser.mPanelContainer;

    tabbrowser.removeEventListener("TabOpen", this, true);
    tabbrowser.removeEventListener("TabClose", this, true);
    tabbrowser.removeEventListener("TabSelect", this, true);
    
    for (var i = 0; i < tabpanels.childNodes.length; i++) {
      this.onTabRemove(aWindow, tabpanels.childNodes[i], true);
    }
    
    if (this._loadState == STATE_RUNNING) { 
      
      this._collectWindowData(aWindow);
      
      
      this._lastWindowClosed = this._windows[aWindow.__SSi];
      this._lastWindowClosed.title = aWindow.content.document.title;
      this._updateCookies([this._lastWindowClosed]);
      
      
      delete this._windows[aWindow.__SSi];
      
      
      this.saveStateDelayed();
    }
    
    delete aWindow.__SSi;
  },

  








  onTabAdd: function sss_onTabAdd(aWindow, aPanel, aNoNotification) {
    aPanel.addEventListener("load", this, true);
    aPanel.addEventListener("pageshow", this, true);
    aPanel.addEventListener("input", this, true);
    aPanel.addEventListener("DOMAutoComplete", this, true);
    
    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }
  },

  








  onTabRemove: function sss_onTabRemove(aWindow, aPanel, aNoNotification) {
    aPanel.removeEventListener("load", this, true);
    aPanel.removeEventListener("pageshow", this, true);
    aPanel.removeEventListener("input", this, true);
    aPanel.removeEventListener("DOMAutoComplete", this, true);
    
    delete aPanel.__SS_data;
    delete aPanel.__SS_text;
    
    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }
  },

  






  onTabClose: function sss_onTabClose(aWindow, aTab) {
    var maxTabsUndo = this._prefBranch.getIntPref("sessionstore.max_tabs_undo");
    
    if (maxTabsUndo == 0) {
      return;
    }
    
    
    this._saveWindowHistory(aWindow);
    this._updateTextAndScrollData(aWindow);
    
    
    var tabState = this._windows[aWindow.__SSi].tabs[aTab._tPos];
    if (tabState && (tabState.entries.length > 1 ||
        tabState.entries[0].url != "about:blank")) {
      this._windows[aWindow.__SSi]._closedTabs.unshift({
        state: tabState,
        title: aTab.getAttribute("label"),
        pos: aTab._tPos
      });
      var length = this._windows[aWindow.__SSi]._closedTabs.length;
      if (length > maxTabsUndo)
        this._windows[aWindow.__SSi]._closedTabs.splice(maxTabsUndo, length - maxTabsUndo);
    }
  },

  








  onTabLoad: function sss_onTabLoad(aWindow, aPanel, aEvent) { 
    
    
    if (aEvent.type != "load" && !aEvent.persisted) {
      return;
    }
    
    delete aPanel.__SS_data;
    delete aPanel.__SS_text;
    this.saveStateDelayed(aWindow);
    
    
    this._updateCrashReportURL(aWindow);
  },

  









  onTabInput: function sss_onTabInput(aWindow, aPanel, aEvent) {
    if (this._saveTextData(aPanel, aEvent.originalTarget)) {
      this.saveStateDelayed(aWindow, 3000);
    }
  },

  






  onTabSelect: function sss_onTabSelect(aWindow, aPanels) {
    if (this._loadState == STATE_RUNNING) {
      this._windows[aWindow.__SSi].selected = aPanels.selectedIndex;
      this.saveStateDelayed(aWindow);

      
      this._updateCrashReportURL(aWindow);
    }
  },



  getBrowserState: function sss_getBrowserState() {
    return this._toJSONString(this._getCurrentState());
  },

  setBrowserState: function sss_setBrowserState(aState) {
    var window = this._getMostRecentBrowserWindow();
    if (!window) {
      this._openWindowWithState("(" + aState + ")");
      return;
    }

    
    this._forEachBrowserWindow(function(aWindow) {
      if (aWindow != window) {
        aWindow.close();
      }
    });

    
    this.restoreWindow(window, "(" + aState + ")", true);
  },

  getWindowState: function sss_getWindowState(aWindow) {
    return this._toJSONString(this._getWindowState(aWindow));
  },

  setWindowState: function sss_setWindowState(aWindow, aState, aOverwrite) {
    this.restoreWindow(aWindow, "(" + aState + ")", aOverwrite);
  },

  getClosedTabCount: function sss_getClosedTabCount(aWindow) {
    return this._windows[aWindow.__SSi]._closedTabs.length;
  },

  closedTabNameAt: function sss_closedTabNameAt(aWindow, aIx) {
    var tabs = this._windows[aWindow.__SSi]._closedTabs;
    
    return aIx in tabs ? tabs[aIx].title : null;
  },

  getClosedTabData: function sss_getClosedTabDataAt(aWindow) {
    return this._toJSONString(this._windows[aWindow.__SSi]._closedTabs);
  },

  undoCloseTab: function sss_undoCloseTab(aWindow, aIndex) {
    var closedTabs = this._windows[aWindow.__SSi]._closedTabs;

    
    aIndex = aIndex || 0;

    if (aIndex in closedTabs) {
      var browser = aWindow.getBrowser();

      
      var closedTab = closedTabs.splice(aIndex, 1).shift();
      var closedTabState = closedTab.state;

      
      closedTabState._tab = browser.addTab();
        
      
      browser.moveTabTo(closedTabState._tab, closedTab.pos);
  
      
      this.restoreHistoryPrecursor(aWindow, [closedTabState], 1, 0, 0);

      
      var content = browser.getBrowserForTab(closedTabState._tab).contentWindow;
      aWindow.setTimeout(function() { content.focus(); }, 0);
    }
    else {
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
    }
  },

  getWindowValue: function sss_getWindowValue(aWindow, aKey) {
    if (aWindow.__SSi) {
      var data = this._windows[aWindow.__SSi].extData || {};
      return data[aKey] || "";
    }
    else {
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
    }
  },

  setWindowValue: function sss_setWindowValue(aWindow, aKey, aStringValue) {
    if (aWindow.__SSi) {
      if (!this._windows[aWindow.__SSi].extData) {
        this._windows[aWindow.__SSi].extData = {};
      }
      this._windows[aWindow.__SSi].extData[aKey] = aStringValue;
      this.saveStateDelayed(aWindow);
    }
    else {
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
    }
  },

  deleteWindowValue: function sss_deleteWindowValue(aWindow, aKey) {
    if (this._windows[aWindow.__SSi].extData[aKey])
      delete this._windows[aWindow.__SSi].extData[aKey];
    else
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
  },

  getTabValue: function sss_getTabValue(aTab, aKey) {
    var data = aTab.__SS_extdata || {};
    return data[aKey] || "";
  },

  setTabValue: function sss_setTabValue(aTab, aKey, aStringValue) {
    if (!aTab.__SS_extdata) {
      aTab.__SS_extdata = {};
    }
    aTab.__SS_extdata[aKey] = aStringValue;
    this.saveStateDelayed(aTab.ownerDocument.defaultView);
  },

  deleteTabValue: function sss_deleteTabValue(aTab, aKey) {
    if (aTab.__SS_extdata[aKey])
      delete aTab.__SS_extdata[aKey];
    else
      Components.returnCode = Cr.NS_ERROR_INVALID_ARG;
  },


  persistTabAttribute: function sss_persistTabAttribute(aName) {
    this.xulAttributes.push(aName);
    this.saveStateDelayed();
  },



  




  _saveWindowHistory: function sss_saveWindowHistory(aWindow) {
    var tabbrowser = aWindow.getBrowser();
    var browsers = tabbrowser.browsers;
    var tabs = this._windows[aWindow.__SSi].tabs = [];
    this._windows[aWindow.__SSi].selected = 0;
    
    for (var i = 0; i < browsers.length; i++) {
      var tabData = { entries: [], index: 0 };
      
      var browser = browsers[i];
      if (!browser || !browser.currentURI) {
        
        tabs.push(tabData);
        continue;
      }
      else if (browser.parentNode.__SS_data && browser.parentNode.__SS_data._tab) {
        
        tabs.push(browser.parentNode.__SS_data);
        continue;
      }
      var history = null;
      
      try {
        history = browser.sessionHistory;
      }
      catch (ex) { } 
      
      if (history && browser.parentNode.__SS_data && browser.parentNode.__SS_data.entries[history.index]) {
        tabData = browser.parentNode.__SS_data;
        tabData.index = history.index + 1;
      }
      else if (history && history.count > 0) {
        for (var j = 0; j < history.count; j++) {
          tabData.entries.push(this._serializeHistoryEntry(history.getEntryAtIndex(j, false)));
        }
        tabData.index = history.index + 1;
        
        browser.parentNode.__SS_data = tabData;
      }
      else {
        tabData.entries[0] = { url: browser.currentURI.spec };
        tabData.index = 1;
      }
      tabData.zoom = browser.markupDocumentViewer.textZoom;
      
      var disallow = CAPABILITIES.filter(function(aCapability) {
        return !browser.docShell["allow" + aCapability];
      });
      tabData.disallow = disallow.join(",");
      
      var _this = this;
      var xulattr = Array.filter(tabbrowser.mTabs[i].attributes, function(aAttr) {
        return (_this.xulAttributes.indexOf(aAttr.name) > -1);
      }).map(function(aAttr) {
        return aAttr.name + "=" + encodeURI(aAttr.value);
      });
      tabData.xultab = xulattr.join(" ");
      
      tabData.extData = tabbrowser.mTabs[i].__SS_extdata || null;
      
      tabs.push(tabData);
      
      if (browser == tabbrowser.selectedBrowser) {
        this._windows[aWindow.__SSi].selected = i + 1;
      }
    }
  },

  






  _serializeHistoryEntry: function sss_serializeHistoryEntry(aEntry) {
    var entry = { url: aEntry.URI.spec, children: [] };
    
    if (aEntry.title && aEntry.title != entry.url) {
      entry.title = aEntry.title;
    }
    if (aEntry.isSubFrame) {
      entry.subframe = true;
    }
    if (!(aEntry instanceof Ci.nsISHEntry)) {
      return entry;
    }
    
    var cacheKey = aEntry.cacheKey;
    if (cacheKey && cacheKey instanceof Ci.nsISupportsPRUint32) {
      entry.cacheKey = cacheKey.data;
    }
    entry.ID = aEntry.ID;
    
    var x = {}, y = {};
    aEntry.getScrollPosition(x, y);
    entry.scroll = x.value + "," + y.value;
    
    try {
      var prefPostdata = this._prefBranch.getIntPref("sessionstore.postdata");
      if (prefPostdata && aEntry.postData && this._checkPrivacyLevel(aEntry.URI.schemeIs("https"))) {
        aEntry.postData.QueryInterface(Ci.nsISeekableStream).
                        seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
        var stream = Cc["@mozilla.org/scriptableinputstream;1"].
                     createInstance(Ci.nsIScriptableInputStream);
        stream.init(aEntry.postData);
        var postdata = stream.read(stream.available());
        if (prefPostdata == -1 || postdata.replace(/^(Content-.*\r\n)+(\r\n)*/, "").length <= prefPostdata) {
          entry.postdata = postdata;
        }
      }
    }
    catch (ex) { debug(ex); } 
    
    if (!(aEntry instanceof Ci.nsISHContainer)) {
      return entry;
    }
    
    for (var i = 0; i < aEntry.childCount; i++) {
      var child = aEntry.GetChildAt(i);
      if (child) {
        entry.children.push(this._serializeHistoryEntry(child));
      }
      else { 
        entry.children.push({ url: "about:blank" });
      }
    }
    
    return entry;
  },

  







  _saveTextData: function sss_saveTextData(aPanel, aTextarea) {
    var id = aTextarea.id ? "#" + aTextarea.id :
                            aTextarea.name;
    if (!id
      || !(aTextarea instanceof Ci.nsIDOMHTMLTextAreaElement 
      || aTextarea instanceof Ci.nsIDOMHTMLInputElement)) {
      return false; 
    }
    
    if (!aPanel.__SS_text) {
      aPanel.__SS_text = [];
      aPanel.__SS_text._refs = [];
    }
    
    
    var ix = aPanel.__SS_text._refs.indexOf(aTextarea);
    if (ix == -1) {
      
      aPanel.__SS_text._refs.push(aTextarea);
      ix = aPanel.__SS_text.length;
    }
    else if (!aPanel.__SS_text[ix].cache) {
      
      
      return false;
    }
    
    
    var content = aTextarea.ownerDocument.defaultView;
    while (content != content.top) {
      var frames = content.parent.frames;
      for (var i = 0; i < frames.length && frames[i] != content; i++);
      id = i + "|" + id;
      content = content.parent;
    }
    
    
    aPanel.__SS_text[ix] = { id: id, element: aTextarea };
    
    return true;
  },

  





  _updateTextAndScrollData: function sss_updateTextAndScrollData(aWindow) {
    var _this = this;
    function updateRecursively(aContent, aData) {
      for (var i = 0; i < aContent.frames.length; i++) {
        if (aData.children && aData.children[i]) {
          updateRecursively(aContent.frames[i], aData.children[i]);
        }
      }
      
      var isHTTPS = _this._getURIFromString((aContent.parent || aContent).
                                        document.location.href).schemeIs("https");
      if ((aContent.document.designMode || "") == "on" && _this._checkPrivacyLevel(isHTTPS)) {
        if (aData.innerHTML == undefined) {
          
          aContent.addEventListener("keypress", function(aEvent) { _this.saveStateDelayed(aWindow, 3000); }, true);
        }
        aData.innerHTML = aContent.document.body.innerHTML;
      }
      aData.scroll = aContent.scrollX + "," + aContent.scrollY;
    }
    
    Array.forEach(aWindow.getBrowser().browsers, function(aBrowser, aIx) {
      try {
        var tabData = this._windows[aWindow.__SSi].tabs[aIx];
        if (tabData.entries.length == 0)
          return; 
        
        var text = [];
        if (aBrowser.parentNode.__SS_text && this._checkPrivacyLevel(aBrowser.currentURI.schemeIs("https"))) {
          for (var ix = aBrowser.parentNode.__SS_text.length - 1; ix >= 0; ix--) {
            var data = aBrowser.parentNode.__SS_text[ix];
            if (!data.cache) {
              
              data.cache = encodeURI(data.element.value);
            }
            text.push(data.id + "=" + data.cache);
          }
        }
        if (aBrowser.currentURI.spec == "about:config") {
          text = ["#textbox=" + encodeURI(aBrowser.contentDocument.getElementById("textbox").wrappedJSObject.value)];
        }
        tabData.text = text.join(" ");
        
        updateRecursively(aBrowser.contentWindow, tabData.entries[tabData.index - 1]);
      }
      catch (ex) { debug(ex); } 
    }, this);
  },

  




  _updateCookieHosts: function sss_updateCookieHosts(aWindow) {
    var hosts = this._windows[aWindow.__SSi]._hosts = {};
    
    
    var _this = this;
    function extractHosts(aEntry) {
      if (/^https?:\/\/(?:[^@\/\s]+@)?([\w.-]+)/.test(aEntry.url) &&
        !hosts[RegExp.$1] && _this._checkPrivacyLevel(_this._getURIFromString(aEntry.url).schemeIs("https"))) {
        var host = RegExp.$1;
        var ix;
        for (ix = host.indexOf(".") + 1; ix; ix = host.indexOf(".", ix) + 1) {
          hosts[host.substr(ix)] = true;
        }
        hosts[host] = true;
      }
      else if (/^file:\/\/([^\/]*)/.test(aEntry.url)) {
        hosts[RegExp.$1] = true;
      }
      if (aEntry.children) {
        aEntry.children.forEach(extractHosts);
      }
    }
    
    this._windows[aWindow.__SSi].tabs.forEach(function(aTabData) { aTabData.entries.forEach(extractHosts); });
  },

  




  _updateCookies: function sss_updateCookies(aWindows) {
    var cookiesEnum = Cc["@mozilla.org/cookiemanager;1"].
                      getService(Ci.nsICookieManager).enumerator;
    
    for (var i = 0; i < aWindows.length; i++)
      aWindows[i].cookies = [];
    
    
    var MAX_EXPIRY = Math.pow(2, 62);
    while (cookiesEnum.hasMoreElements()) {
      var cookie = cookiesEnum.getNext().QueryInterface(Ci.nsICookie2);
      if (cookie.isSession && this._checkPrivacyLevel(cookie.isSecure)) {
        var jscookie = null;
        aWindows.forEach(function(aWindow) {
          if (aWindow._hosts && aWindow._hosts[cookie.rawHost]) {
            
            if (!jscookie) {
              jscookie = { host: cookie.host, value: cookie.value };
              
              if (cookie.path) jscookie.path = cookie.path;
              if (cookie.name) jscookie.name = cookie.name;
              if (cookie.isSecure) jscookie.secure = true;
              if (cookie.isHttpOnly) jscookie.httponly = true;
              if (cookie.expiry < MAX_EXPIRY) jscookie.expiry = cookie.expiry;
            }
            aWindow.cookies.push(jscookie);
          }
        });
      }
    }
    
    
    for (i = 0; i < aWindows.length; i++)
      if (aWindows[i].cookies.length == 0)
        delete aWindows[i].cookies;
  },

  




  _updateWindowFeatures: function sss_updateWindowFeatures(aWindow) {
    var winData = this._windows[aWindow.__SSi];
    
    WINDOW_ATTRIBUTES.forEach(function(aAttr) {
      winData[aAttr] = this._getWindowDimension(aWindow, aAttr);
    }, this);
    
    winData.hidden = WINDOW_HIDEABLE_FEATURES.filter(function(aItem) {
      return aWindow[aItem] && !aWindow[aItem].visible;
    }).join(",");
    
    winData.sidebar = aWindow.document.getElementById("sidebar-box").getAttribute("sidebarcommand");
  },

  



  _getCurrentState: function sss_getCurrentState() {
    var activeWindow = this._getMostRecentBrowserWindow();
    
    if (this._loadState == STATE_RUNNING) {
      
      this._forEachBrowserWindow(function(aWindow) {
        if (this._dirty || this._dirtyWindows[aWindow.__SSi] || aWindow == activeWindow) {
          this._collectWindowData(aWindow);
        }
        else { 
          this._updateWindowFeatures(aWindow);
        }
      }, this);
      this._dirtyWindows = [];
      this._dirty = false;
    }
    
    
    var total = [], windows = [];
    var ix;
    for (ix in this._windows) {
      total.push(this._windows[ix]);
      windows.push(ix);
    }
    this._updateCookies(total);
    
    
    var ix = activeWindow ? windows.indexOf(activeWindow.__SSi || "") : -1;
    if (ix > 0) {
      total.unshift(total.splice(ix, 1)[0]);
    }

    
    if (total.length == 0 && this._lastWindowClosed) {
      total.push(this._lastWindowClosed);
    }
    
    return { windows: total };
  },

  





  _getWindowState: function sss_getWindowState(aWindow) {
    if (this._loadState == STATE_RUNNING) {
      this._collectWindowData(aWindow);
    }
    
    var total = [this._windows[aWindow.__SSi]];
    this._updateCookies(total);
    
    return { windows: total };
  },

  _collectWindowData: function sss_collectWindowData(aWindow) {
    
    this._saveWindowHistory(aWindow);
    this._updateTextAndScrollData(aWindow);
    this._updateCookieHosts(aWindow);
    this._updateWindowFeatures(aWindow);
    
    this._dirtyWindows[aWindow.__SSi] = false;
  },



  








  restoreWindow: function sss_restoreWindow(aWindow, aState, aOverwriteTabs) {
    
    if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
      this.onLoad(aWindow);

    try {
      var root = typeof aState == "string" ? this._safeEval(aState) : aState;
      if (!root.windows[0]) {
        return; 
      }
    }
    catch (ex) { 
      debug(ex);
      return;
    }
    
    var winData;
    
    
    for (var w = 1; w < root.windows.length; w++) {
      winData = root.windows[w];
      if (winData && winData.tabs && winData.tabs[0]) {
        this._openWindowWithState({ windows: [winData], opener: aWindow });
      }
    }
    
    winData = root.windows[0];
    if (!winData.tabs) {
      winData.tabs = [];
    }
    
    var tabbrowser = aWindow.getBrowser();
    var openTabCount = aOverwriteTabs ? tabbrowser.browsers.length : -1;
    var newTabCount = winData.tabs.length;
    
    for (var t = 0; t < newTabCount; t++) {
      winData.tabs[t]._tab = t < openTabCount ? tabbrowser.mTabs[t] : tabbrowser.addTab();
      
      if (!aOverwriteTabs && root._firstTabs) {
        tabbrowser.moveTabTo(winData.tabs[t]._tab, t);
      }
    }

    
    for (t = openTabCount - 1; t >= newTabCount; t--) {
      tabbrowser.removeTab(tabbrowser.mTabs[t]);
    }
    
    if (aOverwriteTabs) {
      this.restoreWindowFeatures(aWindow, winData, root.opener || null);
    }
    if (winData.cookies) {
      this.restoreCookies(winData.cookies);
    }
    if (winData.extData) {
      if (!this._windows[aWindow.__SSi].extData) {
        this._windows[aWindow.__SSi].extData = {}
      }
      for (var key in winData.extData) {
        this._windows[aWindow.__SSi].extData[key] = winData.extData[key];
      }
    }
    if (winData._closedTabs && (root._firstTabs || aOverwriteTabs)) {
      this._windows[aWindow.__SSi]._closedTabs = winData._closedTabs;
    }
    
    this.restoreHistoryPrecursor(aWindow, winData.tabs, (aOverwriteTabs ?
      (parseInt(winData.selected) || 1) : 0), 0, 0);
  },

  










  restoreHistoryPrecursor: function sss_restoreHistoryPrecursor(aWindow, aTabs, aSelectTab, aIx, aCount) {
    var tabbrowser = aWindow.getBrowser();
    
    
    
    for (var t = aIx; t < aTabs.length; t++) {
      try {
        if (!tabbrowser.getBrowserForTab(aTabs[t]._tab).webNavigation.sessionHistory) {
          throw new Error();
        }
      }
      catch (ex) { 
        if (aCount < 10) {
          var restoreHistoryFunc = function(self) {
            self.restoreHistoryPrecursor(aWindow, aTabs, aSelectTab, aIx, aCount + 1);
          }
          aWindow.setTimeout(restoreHistoryFunc, 100, this);
          return;
        }
      }
    }
    
    
    for (t = 0; t < aTabs.length; t++) {
      if (!aTabs[t].entries || !aTabs[t].entries[0])
        continue; 
      
      var tab = aTabs[t]._tab;
      var browser = tabbrowser.getBrowserForTab(tab);
      browser.stop(); 
      
      tab.setAttribute("busy", "true");
      tabbrowser.updateIcon(tab);
      tabbrowser.setTabTitleLoading(tab);
      
      
      
      browser.parentNode.__SS_data = aTabs[t];
    }
    
    
    if (aSelectTab-- && aTabs[aSelectTab]) {
        aTabs.unshift(aTabs.splice(aSelectTab, 1)[0]);
        tabbrowser.selectedTab = aTabs[0]._tab;
    }

    
    var idMap = { used: {} };
    this.restoreHistory(aWindow, aTabs, idMap);
  },

  








  restoreHistory: function sss_restoreHistory(aWindow, aTabs, aIdMap) {
    var _this = this;
    while (aTabs.length > 0 && (!aTabs[0]._tab || !aTabs[0]._tab.parentNode)) {
      aTabs.shift(); 
    }
    if (aTabs.length == 0) {
      return; 
    }
    
    var tabData = aTabs.shift();

    var tab = tabData._tab;
    var browser = aWindow.getBrowser().getBrowserForTab(tab);
    var history = browser.webNavigation.sessionHistory;
    
    if (history.count > 0) {
      history.PurgeHistory(history.count);
    }
    history.QueryInterface(Ci.nsISHistoryInternal);
    
    if (!tabData.entries) {
      tabData.entries = [];
    }
    if (tabData.extData) {
      tab.__SS_extdata = tabData.extData;
    }
    
    browser.markupDocumentViewer.textZoom = parseFloat(tabData.zoom || 1);
    
    for (var i = 0; i < tabData.entries.length; i++) {
      history.addEntry(this._deserializeHistoryEntry(tabData.entries[i], aIdMap), true);
    }
    
    
    var disallow = (tabData.disallow)?tabData.disallow.split(","):[];
    CAPABILITIES.forEach(function(aCapability) {
      browser.docShell["allow" + aCapability] = disallow.indexOf(aCapability) == -1;
    });
    Array.filter(tab.attributes, function(aAttr) {
      return (_this.xulAttributes.indexOf(aAttr.name) > -1);
    }).forEach(tab.removeAttribute, tab);
    if (tabData.xultab) {
      tabData.xultab.split(" ").forEach(function(aAttr) {
        if (/^([^\s=]+)=(.*)/.test(aAttr)) {
          tab.setAttribute(RegExp.$1, decodeURI(RegExp.$2));
        }
      });
    }
    
    
    var event = aWindow.document.createEvent("Events");
    event.initEvent("SSTabRestoring", true, false);
    tab.dispatchEvent(event);
    
    var activeIndex = (tabData.index || tabData.entries.length) - 1;
    try {
      browser.webNavigation.gotoIndex(activeIndex);
    }
    catch (ex) { } 
    
    
    
    
    browser.__SS_restore_data = tabData.entries[activeIndex] || {};
    browser.__SS_restore_text = tabData.text || "";
    browser.__SS_restore_tab = tab;
    browser.__SS_restore = this.restoreDocument_proxy;
    browser.addEventListener("load", browser.__SS_restore, true);
    
    aWindow.setTimeout(function(){ _this.restoreHistory(aWindow, aTabs, aIdMap); }, 0);
  },

  







  _deserializeHistoryEntry: function sss_deserializeHistoryEntry(aEntry, aIdMap) {
    var shEntry = Cc["@mozilla.org/browser/session-history-entry;1"].
                  createInstance(Ci.nsISHEntry);
    
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    shEntry.setURI(ioService.newURI(aEntry.url, null, null));
    shEntry.setTitle(aEntry.title || aEntry.url);
    shEntry.setIsSubFrame(aEntry.subframe || false);
    shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
    
    if (aEntry.cacheKey) {
      var cacheKey = Cc["@mozilla.org/supports-PRUint32;1"].
                     createInstance(Ci.nsISupportsPRUint32);
      cacheKey.data = aEntry.cacheKey;
      shEntry.cacheKey = cacheKey;
    }
    if (aEntry.ID) {
      
      
      var id = aIdMap[aEntry.ID] || 0;
      if (!id) {
        for (id = Date.now(); aIdMap.used[id]; id++);
        aIdMap[aEntry.ID] = id;
        aIdMap.used[id] = true;
      }
      shEntry.ID = id;
    }
    
    var scrollPos = (aEntry.scroll || "0,0").split(",");
    scrollPos = [parseInt(scrollPos[0]) || 0, parseInt(scrollPos[1]) || 0];
    shEntry.setScrollPosition(scrollPos[0], scrollPos[1]);
    
    if (aEntry.postdata) {
      var stream = Cc["@mozilla.org/io/string-input-stream;1"].
                   createInstance(Ci.nsIStringInputStream);
      stream.setData(aEntry.postdata, -1);
      shEntry.postData = stream;
    }
    
    if (aEntry.children && shEntry instanceof Ci.nsISHContainer) {
      for (var i = 0; i < aEntry.children.length; i++) {
        shEntry.AddChild(this._deserializeHistoryEntry(aEntry.children[i], aIdMap), i);
      }
    }
    
    return shEntry;
  },

  


  restoreDocument_proxy: function sss_restoreDocument_proxy(aEvent) {
    
    if (!aEvent || !aEvent.originalTarget || !aEvent.originalTarget.defaultView || aEvent.originalTarget.defaultView != aEvent.originalTarget.defaultView.top) {
      return;
    }
    
    var textArray = this.__SS_restore_text ? this.__SS_restore_text.split(" ") : [];
    function restoreTextData(aContent, aPrefix) {
      textArray.forEach(function(aEntry) {
        if (/^((?:\d+\|)*)(#?)([^\s=]+)=(.*)$/.test(aEntry) && (!RegExp.$1 || RegExp.$1 == aPrefix)) {
          var document = aContent.document;
          var node = RegExp.$2 ? document.getElementById(RegExp.$3) : document.getElementsByName(RegExp.$3)[0] || null;
          if (node && "value" in node) {
            node.value = decodeURI(RegExp.$4);
            
            var event = document.createEvent("UIEvents");
            event.initUIEvent("input", true, true, aContent, 0);
            node.dispatchEvent(event);
          }
        }
      });
    }
    
    function restoreTextDataAndScrolling(aContent, aData, aPrefix) {
      restoreTextData(aContent, aPrefix);
      if (aData.innerHTML) {
        aContent.setTimeout(function(aHTML) { if (this.document.designMode == "on") { this.document.body.innerHTML = aHTML; } }, 0, aData.innerHTML);
      }
      if (aData.scroll && /(\d+),(\d+)/.test(aData.scroll)) {
        aContent.scrollTo(RegExp.$1, RegExp.$2);
      }
      for (var i = 0; i < aContent.frames.length; i++) {
        if (aData.children && aData.children[i]) {
          restoreTextDataAndScrolling(aContent.frames[i], aData.children[i], i + "|" + aPrefix);
        }
      }
    }
    
    var content = aEvent.originalTarget.defaultView;
    if (this.currentURI.spec == "about:config") {
      
      
      content = content.wrappedJSObject;
    }
    restoreTextDataAndScrolling(content, this.__SS_restore_data, "");
    
    
    var event = this.ownerDocument.createEvent("Events");
    event.initEvent("SSTabRestored", true, false);
    this.__SS_restore_tab.dispatchEvent(event);
    
    this.removeEventListener("load", this.__SS_restore, true);
    delete this.__SS_restore_data;
    delete this.__SS_restore_text;
    delete this.__SS_restore_tab;
    delete this.__SS_restore;
  },

  








  restoreWindowFeatures: function sss_restoreWindowFeatures(aWindow, aWinData, aOpener) {
    var hidden = (aWinData.hidden)?aWinData.hidden.split(","):[];
    WINDOW_HIDEABLE_FEATURES.forEach(function(aItem) {
      aWindow[aItem].visible = hidden.indexOf(aItem) == -1;
    });
    
    var _this = this;
    aWindow.setTimeout(function() {
      _this.restoreDimensions_proxy.apply(_this, [aWindow, aOpener, aWinData.width || 0, 
        aWinData.height || 0, "screenX" in aWinData ? aWinData.screenX : NaN,
        "screenY" in aWinData ? aWinData.screenY : NaN,
        aWinData.sizemode || "", aWinData.sidebar || ""]);
    }, 0);
  },

  
















  restoreDimensions_proxy: function sss_restoreDimensions_proxy(aWindow, aOpener, aWidth, aHeight, aLeft, aTop, aSizeMode, aSidebar) {
    var win = aWindow;
    var _this = this;
    function win_(aName) { return _this._getWindowDimension(win, aName); }
    
    
    if (aWidth && aHeight && (aWidth != win_("width") || aHeight != win_("height"))) {
      aWindow.resizeTo(aWidth, aHeight);
    }
    if (!isNaN(aLeft) && !isNaN(aTop) && (aLeft != win_("screenX") || aTop != win_("screenY"))) {
      aWindow.moveTo(aLeft, aTop);
    }
    if (aSizeMode == "maximized" && win_("sizemode") != "maximized") {
      aWindow.maximize();
    }
    else if (aSizeMode && aSizeMode != "maximized" && win_("sizemode") != "normal") {
      aWindow.restore();
    }
    var sidebar = aWindow.document.getElementById("sidebar-box");
    if (sidebar.getAttribute("sidebarcommand") != aSidebar) {
      aWindow.toggleSidebar(aSidebar);
    }
    
    
    if (aOpener) {
      aOpener.focus();
    }
  },

  




  restoreCookies: function sss_restoreCookies(aCookies) {
    if (aCookies.count && aCookies.domain1) {
      
      var converted = [];
      for (var i = 1; i <= aCookies.count; i++) {
        
        var parsed = aCookies["value" + i].match(/^([^=;]+)=([^;]*);(?:domain=[^;]+;)?(?:path=([^;]*);)?(secure;)?(httponly;)?/);
        if (parsed && /^https?:\/\/([^\/]+)/.test(aCookies["domain" + i]))
          converted.push({
            host: RegExp.$1, path: parsed[3], name: parsed[1], value: parsed[2],
            secure: parsed[4], httponly: parsed[5]
          });
      }
      aCookies = converted;
    }
    
    var cookieManager = Cc["@mozilla.org/cookiemanager;1"].
                        getService(Ci.nsICookieManager2);
    
    var MAX_EXPIRY = Math.pow(2, 62);
    for (i = 0; i < aCookies.length; i++) {
      var cookie = aCookies[i];
      try {
        cookieManager.add(cookie.host, cookie.path || "", cookie.name || "", cookie.value, !!cookie.secure, !!cookie.httponly, true, "expiry" in cookie ? cookie.expiry : MAX_EXPIRY);
      }
      catch (ex) { Components.utils.reportError(ex); } 
    }
  },

  




  retryDownloads: function sss_retryDownloads(aWindow) {
    var downloadManager = Cc["@mozilla.org/download-manager;1"].
                          getService(Ci.nsIDownloadManager);
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    
    var database = downloadManager.DBConnection;
    
    var stmt = database.createStatement("SELECT source, target, id " +
                                        "FROM moz_downloads " +
                                        "WHERE state = ?1");
    stmt.bindInt32Parameter(0, Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING);

    var dls = [];
    
    while (stmt.executeStep()) {
      
      var url = stmt.getUTF8String(0);
      
      var savedTo = stmt.getUTF8String(1);
      var savedToURI = ioService.newURI(savedTo, null, null);
      savedTo = savedToURI.path;
   
      var dl = { id: stmt.getInt64(2), url: url, savedTo: savedTo };
      dls.push(dl);
    }
    stmt.reset();

    
    stmt = database.createStatement("DELETE FROM moz_downloads " +
                                    "WHERE state = ?1");
    stmt.bindInt32Parameter(0, Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING);
    stmt.execute();

    for (var i = dls.length - 1; i >= 0; --i) {
      var linkChecker = Cc["@mozilla.org/network/urichecker;1"].
                        createInstance(Ci.nsIURIChecker);
      linkChecker.init(ioService.newURI(dls[i].url, null, null));
      linkChecker.loadFlags = Ci.nsIRequest.LOAD_BACKGROUND;
      linkChecker.asyncCheck(new AutoDownloader(dls[i].url, dls[i].savedTo, aWindow),
                             null);
    }
  },



  







  saveStateDelayed: function sss_saveStateDelayed(aWindow, aDelay) {
    if (aWindow) {
      this._dirtyWindows[aWindow.__SSi] = true;
    }

    if (!this._saveTimer && this._resume_from_crash) {
      
      var minimalDelay = this._lastSaveTime + this._interval - Date.now();
      
      
      aDelay = Math.max(minimalDelay, aDelay || 2000);
      if (aDelay > 0) {
        this._saveTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this._saveTimer.init(this, aDelay, Ci.nsITimer.TYPE_ONE_SHOT);
      }
      else {
        this.saveState();
      }
    }
  },

  




  saveState: function sss_saveState(aUpdateAll) {
    
    if (!this._resume_from_crash && this._loadState == STATE_RUNNING)
      return;
    
    this._dirty = aUpdateAll;
    var oState = this._getCurrentState();
    oState.session = { state: ((this._loadState == STATE_RUNNING) ? STATE_RUNNING_STR : STATE_STOPPED_STR) };
    this._writeFile(this._sessionFile, oState.toSource());
    this._lastSaveTime = Date.now();
  },

  


  _clearDisk: function sss_clearDisk() {
    if (this._sessionFile.exists()) {
      try {
        this._sessionFile.remove(false);
      }
      catch (ex) { dump(ex + '\n'); } 
    }
    if (this._sessionFileBackup.exists()) {
      try {
        this._sessionFileBackup.remove(false);
      }
      catch (ex) { dump(ex + '\n'); } 
    }
  },



  





  _forEachBrowserWindow: function sss_forEachBrowserWindow(aFunc) {
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                         getService(Ci.nsIWindowMediator);
    var windowsEnum = windowMediator.getEnumerator("navigator:browser");
    
    while (windowsEnum.hasMoreElements()) {
      var window = windowsEnum.getNext();
      if (window.__SSi) {
        aFunc.call(this, window);
      }
    }
  },

  



  _getMostRecentBrowserWindow: function sss_getMostRecentBrowserWindow() {
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                         getService(Ci.nsIWindowMediator);
    return windowMediator.getMostRecentWindow("navigator:browser");
  },

  





  _openWindowWithState: function sss_openWindowWithState(aState) {
    var argString = Cc["@mozilla.org/supports-string;1"].
                    createInstance(Ci.nsISupportsString);
    argString.data = "";

    
    var window = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                 getService(Ci.nsIWindowWatcher).
                 openWindow(null, this._prefBranch.getCharPref("chromeURL"), "_blank",
                            "chrome,dialog=no,all", argString);
    
    window.__SS_state = aState;
    var _this = this;
    window.addEventListener("load", function(aEvent) {
      aEvent.currentTarget.removeEventListener("load", arguments.callee, true);
      _this.restoreWindow(aEvent.currentTarget, aEvent.currentTarget.__SS_state, true, true);
      delete aEvent.currentTarget.__SS_state;
    }, true);
  },

  



  _doResumeSession: function sss_doResumeSession() {
    return this._prefBranch.getIntPref("startup.page") == 3 ||
      this._prefBranch.getBoolPref("sessionstore.resume_session_once");
  },

  





  _isCmdLineEmpty: function sss_isCmdLineEmpty(aWindow) {
    var defaultArgs = Cc["@mozilla.org/browser/clh;1"].
                      getService(Ci.nsIBrowserHandler).defaultArgs;
    if (aWindow.arguments && aWindow.arguments[0] &&
        aWindow.arguments[0] == defaultArgs)
      aWindow.arguments[0] = null;

    return !aWindow.arguments || !aWindow.arguments[0];
  },

  






  _checkPrivacyLevel: function sss_checkPrivacyLevel(aIsHTTPS) {
    return this._prefBranch.getIntPref("sessionstore.privacy_level") < (aIsHTTPS ? PRIVACY_ENCRYPTED : PRIVACY_FULL);
  },

  










  _getWindowDimension: function sss_getWindowDimension(aWindow, aAttribute) {
    if (aAttribute == "sizemode") {
      switch (aWindow.windowState) {
      case aWindow.STATE_MAXIMIZED:
        return "maximized";
      case aWindow.STATE_MINIMIZED:
        return "minimized";
      default:
        return "normal";
      }
    }
    
    var dimension;
    switch (aAttribute) {
    case "width":
      dimension = aWindow.outerWidth;
      break;
    case "height":
      dimension = aWindow.outerHeight;
      break;
    default:
      dimension = aAttribute in aWindow ? aWindow[aAttribute] : "";
      break;
    }
    
    if (aWindow.windowState == aWindow.STATE_NORMAL) {
      return dimension;
    }
    return aWindow.document.documentElement.getAttribute(aAttribute) || dimension;
  },

  




  _getStringBundle: function sss_getStringBundle(aURI) {
     var bundleService = Cc["@mozilla.org/intl/stringbundle;1"].
                         getService(Ci.nsIStringBundleService);
     var appLocale = Cc["@mozilla.org/intl/nslocaleservice;1"].
                     getService(Ci.nsILocaleService).getApplicationLocale();
     return bundleService.createBundle(aURI, appLocale);
  },

  




  _getURIFromString: function sss_getURIFromString(aString) {
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    return ioService.newURI(aString, null, null);
  },

  


  _updateCrashReportURL: function sss_updateCrashReportURL(aWindow) {
    if (!Ci.nsICrashReporter) {
      
      this._updateCrashReportURL = function(aWindow) {};
      return;
    }
    try {
      var currentUrl = aWindow.getBrowser().currentURI.spec;
      var cr = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsICrashReporter);
      cr.annotateCrashReport("URL", currentUrl);
    }
    catch (ex) { debug(ex); }
  },

  


  _safeEval: function sss_safeEval(aStr) {
    return Components.utils.evalInSandbox(aStr, EVAL_SANDBOX);
  },

  









  _toJSONString: function sss_toJSONString(aJSObject) {
    
    const charMap = { "\b": "\\b", "\t": "\\t", "\n": "\\n", "\f": "\\f",
                      "\r": "\\r", '"': '\\"', "\\": "\\\\" };
    
    var parts = [];
    
    
    
    function jsonIfy(aObj) {
      if (typeof aObj == "boolean") {
        parts.push(aObj ? "true" : "false");
      }
      else if (typeof aObj == "number" && isFinite(aObj)) {
        
        parts.push(aObj.toString());
      }
      else if (typeof aObj == "string") {
        aObj = aObj.replace(/[\\"\x00-\x1F\u0080-\uFFFF]/g, function($0) {
          
          
          return charMap[$0] ||
            "\\u" + ("0000" + $0.charCodeAt(0).toString(16)).slice(-4);
        });
        parts.push('"' + aObj + '"')
      }
      else if (aObj == null) {
        parts.push("null");
      }
      else if (aObj instanceof Array || aObj instanceof EVAL_SANDBOX.Array) {
        parts.push("[");
        for (var i = 0; i < aObj.length; i++) {
          jsonIfy(aObj[i]);
          parts.push(",");
        }
        if (parts[parts.length - 1] == ",")
          parts.pop(); 
        parts.push("]");
      }
      else if (typeof aObj == "object") {
        parts.push("{");
        for (var key in aObj) {
          if (key == "_tab")
            continue; 
          
          jsonIfy(key.toString());
          parts.push(":");
          jsonIfy(aObj[key]);
          parts.push(",");
        }
        if (parts[parts.length - 1] == ",")
          parts.pop(); 
        parts.push("}");
      }
      else {
        throw new Error("No JSON representation for this object!");
      }
    }
    jsonIfy(aJSObject);
    
    var newJSONString = parts.join(" ");
    
    if (/[^,:{}\[\]0-9.\-+Eaeflnr-u \n\r\t]/.test(
      newJSONString.replace(/"(\\.|[^"\\])*"/g, "")
    ))
      throw new Error("JSON conversion failed unexpectedly!");
    
    return newJSONString;
  },



  






  _writeFile: function sss_writeFile(aFile, aData) {
    
    var stream = Cc["@mozilla.org/network/safe-file-output-stream;1"].
                 createInstance(Ci.nsIFileOutputStream);
    stream.init(aFile, 0x02 | 0x08 | 0x20, 0600, 0);

    
    var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    var convertedData = converter.ConvertFromUnicode(aData);
    convertedData += converter.Finish();

    
    stream.write(convertedData, convertedData.length);
    if (stream instanceof Ci.nsISafeOutputStream) {
      stream.finish();
    } else {
      stream.close();
    }
  },



  QueryInterface: function(aIID) {
    if (!aIID.equals(Ci.nsISupports) && 
      !aIID.equals(Ci.nsIObserver) && 
      !aIID.equals(Ci.nsISupportsWeakReference) && 
      !aIID.equals(Ci.nsIDOMEventListener) &&
      !aIID.equals(Ci.nsISessionStore)) {
      Components.returnCode = Cr.NS_ERROR_NO_INTERFACE;
      return null;
    }
    
    return this;
  }
};



function AutoDownloader(aURL, aFilename, aWindow) {
   this._URL = aURL;
   this._filename = aFilename;
   this._window = aWindow;
}

AutoDownloader.prototype = {
  onStartRequest: function(aRequest, aContext) { },
  onStopRequest: function(aRequest, aContext, aStatus) {
    if (Components.isSuccessCode(aStatus)) {
      var file =
        Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.initWithPath(this._filename);
      if (file.exists()) {
        file.remove(false);
      }
      
      this._window.saveURL(this._URL, this._filename, null, true, true, null);
    }
  }
};





const SessionStoreModule = {

  getClassObject: function(aCompMgr, aCID, aIID) {
    if (aCID.equals(CID)) {
      return SessionStoreFactory;
    }
    
    Components.returnCode = Cr.NS_ERROR_NOT_REGISTERED;
    return null;
  },

  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType) {
    aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CID, CLASS_NAME, CONTRACT_ID, aFileSpec, aLocation, aType);

    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    catMan.addCategoryEntry("app-startup", CLASS_NAME, "service," + CONTRACT_ID, true, true);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType) {
    aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CID, aLocation);

    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    catMan.deleteCategoryEntry( "app-startup", "service," + CONTRACT_ID, true);
  },

  canUnload: function(aCompMgr) {
    return true;
  }
}



const SessionStoreFactory = {

  createInstance: function(aOuter, aIID) {
    if (aOuter != null) {
      Components.returnCode = Cr.NS_ERROR_NO_AGGREGATION;
      return null;
    }
    
    return (new SessionStoreService()).QueryInterface(aIID);
  },

  lockFactory: function(aLock) { },

  QueryInterface: function(aIID) {
    if (!aIID.equals(Ci.nsISupports) && !aIID.equals(Ci.nsIModule) &&
        !aIID.equals(Ci.nsIFactory) && !aIID.equals(Ci.nsISessionStore)) {
      Components.returnCode = Cr.NS_ERROR_NO_INTERFACE;
      return null;
    }
    
    return this;
  }
};

function NSGetModule(aComMgr, aFileSpec) {
  return SessionStoreModule;
}
