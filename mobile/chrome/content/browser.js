







































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

var HUDBar = null;

function getBrowser() {
  return Browser.content.browser;
}

var Browser = {
  _content : null,

  _titleChanged : function(aEvent) {
    if (aEvent.target != this.content.browser.contentDocument)
      return;

      document.title = "Firefox - " + aEvent.target.title;
  },

  _tabOpen : function(aEvent) {
    aEvent.originalTarget.zoomController = new ZoomController(this._content);
    aEvent.originalTarget.mouseController = new MouseController(this._content);
    aEvent.originalTarget.progressController = new ProgressController(aEvent.originalTarget);
  },

  _tabClose : function(aEvent) {
  },

  _tabSelect : function(aEvent) {
    
  },

  _popupShowing : function(aEvent) {
    var target = document.popupNode;
    var isContentSelected = !document.commandDispatcher.focusedWindow.getSelection().isCollapsed;
    var isTextField = target instanceof HTMLTextAreaElement;
    if (target instanceof HTMLInputElement && (target.type == "text" || target.type == "password"))
      isTextField = true;
    var isTextSelected= (isTextField && target.selectionStart != target.selectionEnd);

    





















    InlineSpellCheckerUI.clearSuggestionsFromMenu();
    InlineSpellCheckerUI.uninit();

    var separator = document.getElementById("menusep_spellcheck");
    separator.hidden = true;
    var addToDictionary = document.getElementById("menuitem_addToDictionary");
    addToDictionary.hidden = true;
    var noSuggestions = document.getElementById("menuitem_noSuggestions");
    noSuggestions.hidden = true;

    
    var win = target.ownerDocument.defaultView;
    if (win) {
      var isEditable = false;
      try {
        var editingSession = win.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIWebNavigation)
                                .QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIEditingSession);
        isEditable = editingSession.windowIsEditable(win);
      }
      catch(ex) {
        
      }
    }

    var editor = null;
    if (isTextField && !target.readOnly)
      editor = target.QueryInterface(Ci.nsIDOMNSEditableElement).editor;

    if (isEditable)
      editor = editingSession.getEditorForWindow(win);
dump("ready\n");
    if (editor) {
dump("editor\n");
dump("anchor="+editor.selection.anchorNode+"\n");
dump("offset="+editor.selection.anchorOffset+"\n");
dump(editor.selectionController.getSelection(Ci.nsISelectionController.SELECTION_SPELLCHECK).rangeCount);
      InlineSpellCheckerUI.init(editor);
dump(InlineSpellCheckerUI.canSpellCheck);

      InlineSpellCheckerUI.initFromEvent(editor.selection.anchorNode, editor.selection.anchorOffset);

      var onMisspelling = InlineSpellCheckerUI.overMisspelling;
      if (onMisspelling) {
dump("misspelling\n");
        separator.hidden = false;
        addToDictionary.hidden = false;
        var menu = document.getElementById("popup_content");
        var suggestions = InlineSpellCheckerUI.addSuggestionsToMenu(menu, addToDictionary, 5);
        noSuggestions.hidden = (suggestions > 0);
      }
    }
  },

  startup : function() {
    this.prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch2);

    window.controllers.appendController(this);
    if (LocationBar)
      window.controllers.appendController(LocationBar);
    if (HUDBar)
      window.controllers.appendController(HUDBar);

    var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    var styleSheets = Cc["@mozilla.org/content/style-sheet-service;1"].getService(Ci.nsIStyleSheetService);

    
    var hideCursor = this.prefs.getBoolPref("browser.ui.cursor") == false;
    if (hideCursor) {
      window.QueryInterface(Ci.nsIDOMChromeWindow).setCursor("none");

      var styleURI = ios.newURI("chrome://browser/content/content.css", null, null);
      styleSheets.loadAndRegisterSheet(styleURI, styleSheets.AGENT_SHEET);
    }

    
    var styleURI = ios.newURI("chrome://browser/content/scrollbars.css", null, null);
    styleSheets.loadAndRegisterSheet(styleURI, styleSheets.AGENT_SHEET);

    this._content = document.getElementById("content");
    this._content.addEventListener("DOMTitleChanged", this, true);
    this._content.addEventListener("TabOpen", this, true);
    this._content.addEventListener("TabClose", this, true);
    this._content.addEventListener("TabSelect", this, true);
    document.getElementById("popup_content").addEventListener("popupshowing", this, false);

    this._content.addBrowser("about:blank", null, null, false);

    if (LocationBar)
      LocationBar.init();
    if (HUDBar)
      HUDBar.init();
    DownloadMonitor.init();
    Cc["@mozilla.org/login-manager;1"].getService(Ci.nsILoginManager);

    
    var whereURI = null;
    try {
      
      whereURI = this.prefs.getCharPref("browser.startup.homepage");
    }
    catch (e) {
    }

    
    if (window.arguments && window.arguments[0]) {
      try {
        var cmdLine = window.arguments[0].QueryInterface(Ci.nsICommandLine);
        if (cmdLine.length == 1) {
          var uri = cmdLine.getArgument(0);
          if (uri != "" && uri[0] != '-') {
            whereURI = cmdLine.resolveURI(uri);
            if (whereURI)
              whereURI = whereURI.spec;
          }
        }
      }
      catch (e) {
      }
    }

    if (whereURI) {
      var self = this;
      setTimeout(function() { self.content.browser.loadURI(whereURI, null, null, false); }, 10);
    }
  },

  get content() {
    return this._content;
  },

  handleEvent: function (aEvent) {
    switch (aEvent.type) {
      case "DOMTitleChanged":
        this._titleChanged(aEvent);
        break;
      case "TabOpen":
        this._tabOpen(aEvent);
        break;
      case "TabClose":
        this._tabClose(aEvent);
        break;
      case "TabSelect":
        this._tabSelect(aEvent);
        break;
      case "popupshowing":
        this._popupShowing(aEvent);
        break;
    }
  },

  supportsCommand : function(cmd) {
    var isSupported = false;
    switch (cmd) {
      case "cmd_newTab":
      case "cmd_closeTab":
      case "cmd_switchTab":
      case "cmd_menu":
      case "cmd_fullscreen":
      case "cmd_addons":
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
      case "cmd_newTab":
        this.content.addBrowser("about:blank", null, null, false);
        break;
      case "cmd_closeTab":
        this.content.removeBrowser();
        break;
      case "cmd_switchTab":
        this.content.select();
        break;
      case "cmd_menu":
      {





        var menu = document.getElementById("mainmenu");
        menu.openPopup(window.screenX, window.screenY, true);
        break;
      }
      case "cmd_fullscreen":
        window.fullScreen = window.fullScreen ? false : true;
        break;
      case "cmd_addons":
      {
        const EMTYPE = "Extension:Manager";

        var aOpenMode = "extensions";
        var wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
        var needToOpen = true;
        var windowType = EMTYPE + "-" + aOpenMode;
        var windows = wm.getEnumerator(windowType);
        while (windows.hasMoreElements()) {
          var theEM = windows.getNext().QueryInterface(Ci.nsIDOMWindowInternal);
          if (theEM.document.documentElement.getAttribute("windowtype") == windowType) {
            theEM.focus();
            needToOpen = false;
            break;
          }
        }

        if (needToOpen) {
          const EMURL = "chrome://mozapps/content/extensions/extensions.xul?type=" + aOpenMode;
          const EMFEATURES = "chrome,dialog=no,resizable=yes";
          window.openDialog(EMURL, "", EMFEATURES);
        }
        break;
      }
      case "cmd_downloads":
        Cc["@mozilla.org/download-manager-ui;1"].getService(Ci.nsIDownloadManagerUI).show(window);
    }
  }
};


function ProgressController(aBrowser) {
  this.init(aBrowser);
}

ProgressController.prototype = {
  _browser : null,

  init : function(aBrowser) {
    this._browser = aBrowser;
    this._browser.addProgressListener(this, Components.interfaces.nsIWebProgress.NOTIFY_ALL);
  },

  onStateChange : function(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
      if (aRequest && aWebProgress.DOMWindow == this._browser.contentWindow) {
        if (aStateFlags & Ci.nsIWebProgressListener.STATE_START) {
          if (LocationBar)
            LocationBar.update(TOOLBARSTATE_LOADING);
          if (HUDBar)
            HUDBar.update(TOOLBARSTATE_LOADING);
        }
        else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
          this._browser.zoomController.scale = 1;
          if (LocationBar)
            LocationBar.update(TOOLBARSTATE_LOADED);
          if (HUDBar)
            HUDBar.update(TOOLBARSTATE_LOADED);
        }
      }
    }

    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
        aWebProgress.DOMWindow.focus();
        
      }
    }
  },

  
  
  onProgressChange : function(aWebProgress, aRequest, aCurSelf, aMaxSelf, aCurTotal, aMaxTotal) {
  },

  
  onLocationChange : function(aWebProgress, aRequest, aLocation) {
    if (aWebProgress.DOMWindow == this._browser.contentWindow) {
      if (LocationBar)
        LocationBar.setURI();
      if (HUDBar)
        HUDBar.setURI(aLocation.spec);
    }
  },

  
  
  onStatusChange : function(aWebProgress, aRequest, aStatus, aMessage) {
  },

  
  onSecurityChange : function(aWebProgress, aRequest, aState) {
  },

  QueryInterface : function(aIID) {
    if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};









var SpeedCache = function(maxsize) {
    this.init(maxsize);
}

SpeedCache.prototype = {
    _items   : null,
    _maxsize : 1,

    init: function(maxsize) {
	
	if (maxsize <= 0) maxsize = 1;
	this._items = new Array(maxsize);
	this._count = 0;
	
	for (x = 0; x < maxsize; x++) {
	    this._items[x] = 0;
	}
    },

    addSpeed: function(speed){
	var index = this._count % this._items.length;
	this._items[index] = speed;
	this._count++;
    },
    
    getAverage: function() {
	var maxsize = this._items.length;
	var sum = 0;
	for (x = 0; x < maxsize; x++) {
	    sum += this._items[x];
	}
	return sum / maxsize;
    },
}

var MouseController = function(browser) {
  this.init(browser);
}

MouseController.prototype = {
  _browser: null,
  _contextID : null,
  _mousedown : false,
  _panning : false,
  
  _lastX   : new SpeedCache(5),
  _lastY   : new SpeedCache(5),

  init: function(aBrowser)
  {
    this._browser = aBrowser;
    this._browser.addEventListener("mousedown", this, false);
    this._browser.addEventListener("mouseup",this, false);
    this._browser.addEventListener("mousemove", this, false);
  },

  handleEvent: function(aEvent)
  {
    if (!aEvent.type in this)
      dump("MouseController called with unknown event type " + aEvent.type + "\n");
    this[aEvent.type](aEvent);
  },

  mousedown: function(aEvent)
  {
    
    




    if (aEvent.target instanceof HTMLInputElement ||
        aEvent.target instanceof HTMLTextAreaElement ||
        aEvent.target instanceof HTMLAnchorElement ||
        aEvent.target instanceof HTMLSelectElement)
      return;

    
    if (this.firstEvent &&
        (aEvent.timeStamp - this.firstEvent.timeStamp) < 400 &&
        Math.abs(aEvent.screenX - this.firstEvent.screenX) < 30 &&
        Math.abs(aEvent.screenY - this.firstEvent.screenY) < 30) {
      this.dblclick(aEvent);
      return;
    }

    this.lastEvent = this.firstEvent = aEvent;
    this._mousedown = true;
    this._panning = false;

    

    aEvent.stopPropagation();
    aEvent.preventDefault();
  },

  mouseup: function(aEvent)
  {
    this._mousedown = false;
    if (this._contextID) {
      clearTimeout(this._contextID);
      this._contextID = null;
    }

    

    
    var totalDistance = Math.sqrt(
        Math.pow(this.firstEvent.screenX - aEvent.screenX, 2) +
        Math.pow(this.firstEvent.screenY - aEvent.screenY, 2));

    if (totalDistance > 10) { 
      aEvent.preventDefault();
    }
    else if (this._panning) {
      
      
      this._browser.endPan();
      this._panning = false;
      return;
    }

    
    function _doKineticScroll(browser, speedX, speedY, step) {
      const decayFactor = 0.95;
      const cutoff = 2;

      
      const speedLimit = 55;
      if (Math.abs(speedY) > speedLimit)
        speedY = speedY > 0 ? speedLimit : -speedLimit;

      if (Math.abs(speedX) > speedLimit)
        speedX = speedX > 0 ? speedLimit : -speedLimit;

      
      if (speedX < 0)
        speedX = Math.ceil(speedX);
      else
        speedX = Math.floor(speedX);

      if (speedY < 0)
        speedY = Math.ceil(speedY);
      else
        speedY = Math.floor(speedY);

      
      browser.doPan(-speedX, -speedY);

      
      speedX *= (decayFactor - step/50);
      speedY *= (decayFactor - step/50);

      
      if (Math.abs(speedX) > cutoff || Math.abs(speedY) > cutoff)
        setTimeout( function() { _doKineticScroll(browser, speedX, speedY, ++step); }, 0);
      else
        browser.endPan();
    };

    var browser = this._browser;
    var speedX  = this._lastX.getAverage() * 100;
    var speedY  = this._lastY.getAverage() * 100;
    setTimeout(function() { _doKineticScroll(browser, speedX, speedY, 0); }, 0);
  },

  mousemove: function(aEvent)
  {
    if (!this._mousedown)
      return;

    var delta = aEvent.timeStamp - this.lastEvent.timeStamp;
    var x = aEvent.screenX - this.lastEvent.screenX;
    var y = aEvent.screenY - this.lastEvent.screenY;

    
    if (40 > delta || (2 > Math.abs(x) && 2 > Math.abs(y)))
      return;

    this._lastX.addSpeed(x / delta);
    this._lastY.addSpeed(y / delta);
    this.lastEvent = aEvent;

    
    if (this._contextID) {
      clearTimeout(this._contextID);
      this._contextID = null;
    }

    if (!this._panning) {
      this._panning = true;
      this._browser.startPan();
    }

    if (this._panning) {
      this._browser.doPan(-x, -y);
    }

    

    aEvent.stopPropagation();
    aEvent.preventDefault();
  },

  dblclick: function(aEvent)
  {
    
    var target = aEvent.target;
    aEvent.preventDefault();
    while (target && target.nodeName != "HTML") {
      var disp = window.getComputedStyle(target, "").getPropertyValue("display");
      if (!disp.match(/(inline)/g)) {
        this._browser.browser.zoomController.toggleZoom(target);
        break;
      }
      else {
        target = target.parentNode;
      }
    }
    aEvent.stopPropagation();
    aEvent.preventDefault();
  },

  contextMenu: function(aEvent)
  {
    if (HUDBar)
      HUDBar.show();
    if (this._contextID && this._browser.contextMenu) {
      document.popupNode = aEvent.target;
      var popup = document.getElementById(this._browser.contextMenu);
      popup.openPopup(this._browser, "", aEvent.clientX, aEvent.clientY, true, false);

      this._contextID = null;

      aEvent.stopPropagation();
      aEvent.preventDefault();
    }
  }
}


function ZoomController(aBrowser) {
  this._browser = aBrowser;
};


ZoomController.prototype = {
  _minScale : 0.1,
  _maxScale : 3,
  _target : null,

  set scale(s)
  {
    var clamp = Math.min(this._maxScale, Math.max(this._minScale, s));
    clamp = Math.floor(clamp * 1000) / 1000;  
    if (clamp == this._browser.browser.markupDocumentViewer.fullZoom)
      return;

    this._browser.browser.markupDocumentViewer.fullZoom = clamp;

    
    var leftEdge = this._browser.browser.contentWindow.scrollX + this._browser.browser.contentWindow.document.documentElement.clientWidth;
    var scrollX = this._browser.browser.contentWindow.document.documentElement.scrollWidth - leftEdge;
    if (scrollX < 0)
      this._browser.browser.contentWindow.scrollBy(scrollX, 0);
  },

  get scale()
  {
    return this._browser.browser.markupDocumentViewer.fullZoom;
  },

  reset: function()
  {
    this._minScale = ZoomController.prototype._minScale;
    this._maxScale = ZoomController.prototype._maxScale;
  },

  fitContent: function()
  {
    this._target = null;
    try {
      var oldScale = this.scale;
      this.scale = 1;    
      var body = this._browser.contentWindow.document.body;
      var html = this._browser.contentWindow.document.documentElement;
      var newScale = this.scale;
      var finalWidth = html.clientWidth;
    }
    catch(e) {
      dump(e + "\n");
      return;
    }

    var prefScrollWidth = Math.max(html.scrollWidth, body.scrollWidth); 
    if (prefScrollWidth > (this._browser.boxObject.width - 10) )  {
      
      
      newScale = (this._browser.boxObject.width ) / prefScrollWidth;
      finalWidth = prefScrollWidth;
    }
    body.style.minWidth = body.style.maxWidth = (finalWidth -20) + "px";
    this._minScale = Math.max(this._minScale, newScale);
    this.scale = newScale;
  },

  getPagePosition: function (el)
  {
    var r = el.getBoundingClientRect();
    retVal = {
      width: r.right - r.left,
      height: r.bottom - r.top,
      x: r.left + this._browser.contentWindow.scrollX,
      y: r.top + this._browser.contentWindow.scrollY
    };
    return retVal;
  },

  getWindowRect: function()
  {
    return {
      x: this._browser.contentWindow.scrollX,
      y: this._browser.contentWindow.scrollY,
      width: this._browser.boxObject.width / this.scale,
      height: this._browser.boxObject.height / this.scale
    };
  },

  toggleZoom: function(el)
  {
    if (!el) return;

    if (this.scale == 1 || el != this._target) {
      
      this._target = el;
    }
    else {
      this.scale = 1;
      this._target = null;
    }
  },

  zoomToElement: function(el)
  {
    var margin = 8;

    
    var elRect = this.getPagePosition(el);
    this.scale = (this._browser.boxObject.width) / (elRect.width + 2 * margin);

    
    elRect = this.getPagePosition(el);
    winRect = this.getWindowRect();
    this._browser.contentWindow.scrollTo(Math.max(elRect.x - margin, 0), Math.max(0, elRect.y - margin));
  }
};
