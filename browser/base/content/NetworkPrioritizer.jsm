














































let EXPORTED_SYMBOLS = ["trackBrowserWindow"];

const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");



XPCOMUtils.defineLazyServiceGetter(this, "_focusManager",
                                   "@mozilla.org/focus-manager;1",
                                   "nsIFocusManager");



const TAB_EVENTS = ["TabOpen", "TabSelect"];
const WINDOW_EVENTS = ["activate", "unload"];

const PRIORITY_DELTA = -10;



let _lastFocusedWindow = null;
let _windows = [];



function trackBrowserWindow(aWindow) {
  WindowHelper.addWindow(aWindow);
}



function _handleEvent(aEvent) {
  switch (aEvent.type) {
    case "TabOpen":
      BrowserHelper.onOpen(aEvent.target.linkedBrowser);
      break;
    case "TabSelect":
      BrowserHelper.onSelect(aEvent.target.linkedBrowser);
      break;
    case "activate":
      WindowHelper.onActivate(aEvent.target);
      break;
    case "unload":
      WindowHelper.removeWindow(aEvent.currentTarget);
      break;
  }
}



let BrowserHelper = {
  onOpen: function(aBrowser) {
    
    if (aBrowser.ownerDocument.defaultView != _lastFocusedWindow)
      this.decreasePriority(aBrowser);
  },

  onSelect: function(aBrowser) {
    let windowEntry = WindowHelper.getEntry(aBrowser.ownerDocument.defaultView);
    if (windowEntry.lastSelectedBrowser)
      this.decreasePriority(windowEntry.lastSelectedBrowser);
    this.increasePriority(aBrowser);

    windowEntry.lastSelectedBrowser = aBrowser;
  },

  
  getLoadgroup: function(aBrowser) {
    return aBrowser.webNavigation.QueryInterface(Ci.nsIDocumentLoader)
                   .loadGroup.QueryInterface(Ci.nsISupportsPriority);
  },

  increasePriority: function(aBrowser) {
    this.getLoadgroup(aBrowser).adjustPriority(PRIORITY_DELTA);
  },

  decreasePriority: function(aBrowser) {
    this.getLoadgroup(aBrowser).adjustPriority(PRIORITY_DELTA * -1);
  }
};



let WindowHelper = {
  addWindow: function(aWindow) {
    
    _windows.push({ window: aWindow, lastSelectedBrowser: null });

    
    TAB_EVENTS.forEach(function(event) {
      aWindow.gBrowser.tabContainer.addEventListener(event, _handleEvent, false);
    });
    WINDOW_EVENTS.forEach(function(event) {
      aWindow.addEventListener(event, _handleEvent, false);
    });

    
    
    if (aWindow == _focusManager.activeWindow)
      this.handleFocusedWindow(aWindow);
    else
      this.decreasePriority(aWindow);

    
    BrowserHelper.onSelect(aWindow.gBrowser.selectedBrowser);
  },

  removeWindow: function(aWindow) {
    if (aWindow == _lastFocusedWindow)
      _lastFocusedWindow = null;

    
    _windows.splice(this.getEntryIndex(aWindow), 1);

    
    TAB_EVENTS.forEach(function(event) {
      aWindow.gBrowser.tabContainer.removeEventListener(event, _handleEvent, false);
    });
    WINDOW_EVENTS.forEach(function(event) {
      aWindow.removeEventListener(event, _handleEvent, false);
    });
  },

  onActivate: function(aWindow, aHasFocus) {
    
    if (aWindow == _lastFocusedWindow)
      return;

    
    this.handleFocusedWindow(aWindow);

    
    this.increasePriority(aWindow);
  },

  handleFocusedWindow: function(aWindow) {
    
    if (_lastFocusedWindow)
      this.decreasePriority(_lastFocusedWindow);

    
    _lastFocusedWindow = aWindow;
  },

  
  increasePriority: function(aWindow) {
    aWindow.gBrowser.browsers.forEach(function(aBrowser) {
      BrowserHelper.increasePriority(aBrowser);
    });
  },

  decreasePriority: function(aWindow) {
    aWindow.gBrowser.browsers.forEach(function(aBrowser) {
      BrowserHelper.decreasePriority(aBrowser);
    });
  },

  getEntry: function(aWindow) {
    return _windows[this.getEntryIndex(aWindow)];
  },

  getEntryIndex: function(aWindow) {
    
    for (let i = 0; i < _windows.length; i++)
      if (_windows[i].window == aWindow)
        return i;
  }
};

