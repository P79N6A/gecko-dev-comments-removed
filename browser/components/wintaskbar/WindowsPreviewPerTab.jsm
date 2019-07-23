











































































var EXPORTED_SYMBOLS = ["AeroPeek"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");


const TOGGLE_PREF_NAME = "browser.taskbar.previews.enable";

const DISABLE_THRESHOLD_PREF_NAME = "browser.taskbar.previews.max";

const CACHE_EXPIRATION_TIME_PREF_NAME = "browser.taskbar.previews.cachetime";

const WINTASKBAR_CONTRACTID = "@mozilla.org/windows-taskbar;1";



XPCOMUtils.defineLazyServiceGetter(this, "ioSvc",
                                   "@mozilla.org/network/io-service;1",
                                   "nsIIOService");
XPCOMUtils.defineLazyServiceGetter(this, "imgTools",
                                   "@mozilla.org/image/tools;1",
                                   "imgITools");
XPCOMUtils.defineLazyServiceGetter(this, "faviconSvc",
                                   "@mozilla.org/browser/favicon-service;1",
                                   "nsIFaviconService");


function _imageFromURI(uri, callback) {
  let channel = ioSvc.newChannelFromURI(uri);
  NetUtil.asyncFetch(channel, function(inputStream, resultCode) {
    if (!Components.isSuccessCode(resultCode))
      return;
    try {
      let out_img = { value: null };
      imgTools.decodeImageData(inputStream, channel.contentType, out_img);
      callback(out_img.value);
    } catch (e) {
      
      
      let defaultURI = faviconSvc.defaultFavicon;
      if (!defaultURI.equals(uri))
        _imageFromURI(defaultURI, callback);
    }
  });
}


function getFaviconAsImage(iconurl, callback) {
  if (iconurl)
    _imageFromURI(NetUtil.newURI(iconurl), callback);
  else
    _imageFromURI(faviconSvc.defaultFavicon, callback);
}

















function PreviewController(win, tab) {
  this.win = win;
  this.tab = tab;
  this.linkedBrowser = tab.linkedBrowser;

  this.linkedBrowser.addEventListener("MozAfterPaint", this, false);
  this.linkedBrowser.addEventListener("DOMTitleChanged", this, false);
  
  this.linkedBrowser.addEventListener("pageshow", this, false);

  
  XPCOMUtils.defineLazyGetter(this, "preview", function () this.win.previewFromTab(this.tab));

  XPCOMUtils.defineLazyGetter(this, "canvasPreview", function ()
    this.win.win.document.createElementNS("http://www.w3.org/1999/xhtml", "canvas"));

  XPCOMUtils.defineLazyGetter(this, "dirtyRegion",
    function () {
      let dirtyRegion = Cc["@mozilla.org/gfx/region;1"]
                       .createInstance(Ci.nsIScriptableRegion);
      dirtyRegion.init();
      return dirtyRegion;
    });
}

PreviewController.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITaskbarPreviewController,
                                         Ci.nsIDOMEventListener]),
  destroy: function () {
    this.linkedBrowser.removeEventListener("pageshow", this, false);
    this.linkedBrowser.removeEventListener("DOMTitleChanged", this, false);
    this.linkedBrowser.removeEventListener("MozAfterPaint", this, false);
  },
  get wrappedJSObject() {
    return this;
  },

  get dirtyRects() {
    let rectstream = this.dirtyRegion.getRects();
    if (!rectstream)
      return [];
    let rects = [];
    for (let i = 0; i < rectstream.length; i+= 4) {
      let r = {x:      rectstream[i],
               y:      rectstream[i+1],
               width:  rectstream[i+2],
               height: rectstream[i+3]};
      rects.push(r);
    }
    return rects;
  },

  
  
  
  resetCanvasPreview: function () {
    this.canvasPreview.width = 0;
    this.canvasPreview.height = 0;
  },

  
  
  updateCanvasPreview: function () {
    let win = this.linkedBrowser.contentWindow;
    let bx = this.linkedBrowser.boxObject;
    
    if (bx.width != this.canvasPreview.width ||
        bx.height != this.canvasPreview.height) {
      
      this.onTabPaint({left:0, top:0, width:bx.width, height:bx.height});
      this.canvasPreview.width = bx.width;
      this.canvasPreview.height = bx.height;
    }

    
    let ctx = this.canvasPreview.getContext("2d");
    let flags = this.canvasPreviewFlags;
    
    
    this.dirtyRegion.intersectRect(0, 0, bx.width, bx.height);
    this.dirtyRects.forEach(function (r) {
      let x = r.x;
      let y = r.y;
      let width = r.width;
      let height = r.height;
      ctx.save();
      ctx.translate(x, y);
      ctx.drawWindow(win, x, y, width, height, "white", flags);
      ctx.restore();
    });
    this.dirtyRegion.setToRect(0,0,0,0);

    
    
    AeroPeek.resetCacheTimer();
  },

  onTabPaint: function (rect) {
    
    if (!rect.width || !rect.height)
      return;

    let r = { x: Math.floor(rect.left),
              y: Math.floor(rect.top),
              width: Math.ceil(rect.width),
              height: Math.ceil(rect.height)
            };
    this.dirtyRegion.unionRect(r.x, r.y, r.width, r.height);
  },

  updateTitleAndTooltip: function () {
    let title = this.win.tabbrowser.getWindowTitleForBrowser(this.linkedBrowser);
    this.preview.title = title;
    this.preview.tooltip = title;
  },

  
  

  get width() {
    return this.win.width;
  },

  get height() {
    return this.win.height;
  },

  get thumbnailAspectRatio() {
    let boxObject = this.tab.linkedBrowser.boxObject;
    
    let tabWidth = boxObject.width || 1;
    
    let tabHeight = boxObject.height || 1;
    return tabWidth / tabHeight;
  },

  drawPreview: function (ctx) {
    let self = this;
    this.win.tabbrowser.previewTab(this.tab, function () self.previewTabCallback(ctx));

    
    return false;
  },

  previewTabCallback: function (ctx) {
    let width = this.win.width;
    let height = this.win.height;
    
    ctx.drawWindow(this.win.win, 0, 0, width, height, "transparent");

    
    
    this.updateCanvasPreview();

    let boxObject = this.linkedBrowser.boxObject;
    ctx.translate(boxObject.x, boxObject.y);
    ctx.drawImage(this.canvasPreview, 0, 0);
  },

  drawThumbnail: function (ctx, width, height) {
    this.updateCanvasPreview();

    let scale = width/this.linkedBrowser.boxObject.width;
    ctx.scale(scale, scale);
    ctx.drawImage(this.canvasPreview, 0, 0);

    
    return false;
  },

  onClose: function () {
    this.win.tabbrowser.removeTab(this.tab);
  },

  onActivate: function () {
    this.win.tabbrowser.selectedTab = this.tab;

    
    
    return true;
  },

  
  handleEvent: function (evt) {
    switch (evt.type) {
      case "MozAfterPaint":
        if (evt.originalTarget === this.linkedBrowser.contentWindow) {
          let clientRects = evt.clientRects;
          let length = clientRects.length;
          for (let i = 0; i < length; i++) {
            let r = clientRects.item(i);
            this.onTabPaint(r);
          }
        }
        let preview = this.preview;
        if (preview.visible)
          preview.invalidate();
        break;
      case "pageshow":
      case "DOMTitleChanged":
        
        
        this.win.tabbrowser.setTabTitle(this.tab);
        this.updateTitleAndTooltip();
        break;
    }
  }
};

XPCOMUtils.defineLazyGetter(PreviewController.prototype, "canvasPreviewFlags",
  function () { let canvasInterface = Ci.nsIDOMCanvasRenderingContext2D;
                return canvasInterface.DRAWWINDOW_DRAW_VIEW
                     | canvasInterface.DRAWWINDOW_DRAW_CARET
                     | canvasInterface.DRAWWINDOW_DO_NOT_FLUSH;
});










function TabWindow(win) {
  this.win = win;
  this.tabbrowser = win.gBrowser;

  this.previews = [];

  for (let i = 0; i < this.events.length; i++)
    this.tabbrowser.tabContainer.addEventListener(this.events[i], this, false);
  this.tabbrowser.addTabsProgressListener(this);


  AeroPeek.windows.push(this);
  let tabs = this.tabbrowser.mTabs;
  for (let i = 0; i < tabs.length; i++)
    this.newTab(tabs[i]);

  this.updateTabOrdering();
  AeroPeek.checkPreviewCount();
}

TabWindow.prototype = {
  _enabled: false,
  events: ["TabOpen", "TabClose", "TabSelect", "TabMove"],

  destroy: function () {
    this._destroying = true;

    let tabs = this.tabbrowser.mTabs;

    for (let i = 0; i < this.events.length; i++)
      this.tabbrowser.tabContainer.removeEventListener(this.events[i], this, false);

    for (let i = 0; i < tabs.length; i++)
      this.removeTab(tabs[i]);

    let idx = AeroPeek.windows.indexOf(this.win.gTaskbarTabGroup);
    AeroPeek.windows.splice(idx, 1);
    AeroPeek.checkPreviewCount();
  },

  get width () {
    return this.win.innerWidth;
  },
  get height () {
    return this.win.innerHeight;
  },

  
  newTab: function (tab) {
    let controller = new PreviewController(this, tab);
    let docShell = this.win
                  .QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsIDocShell);
    let preview = AeroPeek.taskbar.createTaskbarTabPreview(docShell, controller);
    preview.visible = AeroPeek.enabled;
    preview.active = this.tabbrowser.selectedTab == tab;
    
    getFaviconAsImage(null, function (img) {
      
      
      if (!preview.icon)
        preview.icon = img;
    });

    
    this.previews.splice(tab._tPos, 0, preview);
    AeroPeek.addPreview(preview);
    
    
    controller.updateTitleAndTooltip();
  },

  
  removeTab: function (tab) {
    let preview = this.previewFromTab(tab);
    preview.active = false;
    preview.visible = false;
    preview.move(null);
    preview.controller.wrappedJSObject.destroy();

    
    
    if (!this._destroying)
      this.previews.splice(tab._tPos, 1);
    AeroPeek.removePreview(preview);
  },

  get enabled () {
    return this._enabled;
  },

  set enabled (enable) {
    this._enabled = enable;
    
    
    
    this.previews.forEach(function (preview) {
      preview.move(null);
      preview.visible = enable;
    });
    this.updateTabOrdering();
  },

  previewFromTab: function (tab) {
    return this.previews[tab._tPos];
  },

  updateTabOrdering: function () {
    
    
    
    
    for (let i = this.previews.length - 1; i >= 0; i--) {
      let p = this.previews[i];
      let next = i == this.previews.length - 1 ? null : this.previews[i+1];
      p.move(next);
    }
  },

  
  handleEvent: function (evt) {
    let tab = evt.originalTarget;
    switch (evt.type) {
      case "TabOpen":
        this.newTab(tab);
        this.updateTabOrdering();
        break;
      case "TabClose":
        this.removeTab(tab);
        this.updateTabOrdering();
        break;
      case "TabSelect":
        this.previewFromTab(tab).active = true;
        break;
      case "TabMove":
        let oldPos = evt.detail;
        let newPos = tab._tPos;
        let preview = this.previews[oldPos];
        this.previews.splice(oldPos, 1);
        this.previews.splice(newPos, 0, preview);
        this.updateTabOrdering();
        break;
    }
  },

  
  onLocationChange: function () {
  },
  onProgressChange: function () {
  },
  onSecurityChange: function () {
  },
  onStateChange: function () {
  },
  onStatusChange: function () {
  },
  onLinkIconAvailable: function (aBrowser, aIconURL) {
    let self = this;
    getFaviconAsImage(aIconURL, function (img) {
      let index = self.tabbrowser.browsers.indexOf(aBrowser);
      
      if (index != -1)
        self.previews[index].icon = img;
    });
  }
}








var AeroPeek = {
  available: false,
  
  _prefenabled: true,

  _enabled: true,

  
  previews: [],

  
  windows: [],

  
  taskbar: null,

  
  maxpreviews: 20,

  
  cacheLifespan: 20,

  initialize: function () {
    if (!(WINTASKBAR_CONTRACTID in Cc))
      return;
    this.taskbar = Cc[WINTASKBAR_CONTRACTID].getService(Ci.nsIWinTaskbar);
    this.available = this.taskbar.available;
    if (!this.available)
      return;

    this.prefs.addObserver(TOGGLE_PREF_NAME, this, false);
    this.prefs.addObserver(DISABLE_THRESHOLD_PREF_NAME, this, false);
    this.prefs.addObserver(CACHE_EXPIRATION_TIME_PREF_NAME, this, false);

    this.cacheLifespan = this.prefs.getIntPref(CACHE_EXPIRATION_TIME_PREF_NAME);

    this.maxpreviews = this.prefs.getIntPref(DISABLE_THRESHOLD_PREF_NAME);

    this.enabled = this._prefenabled = this.prefs.getBoolPref(TOGGLE_PREF_NAME);
  },

  get enabled() {
    return this._enabled;
  },

  set enabled(enable) {
    if (this._enabled == enable)
      return;

    this._enabled = enable;

    this.windows.forEach(function (win) {
      win.enabled = enable;
    });
  },

  addPreview: function (preview) {
    this.previews.push(preview);
    this.checkPreviewCount();
  },

  removePreview: function (preview) {
    let idx = this.previews.indexOf(preview);
    this.previews.splice(idx, 1);
    this.checkPreviewCount();
  },

  checkPreviewCount: function () {
    if (this.previews.length > this.maxpreviews)
      this.enabled = false;
    else
      this.enabled = this._prefenabled;
  },

  onOpenWindow: function (win) {
    
    if (!this.available)
      return;

    win.gTaskbarTabGroup = new TabWindow(win);
  },

  onCloseWindow: function (win) {
    
    if (!this.available)
      return;

    win.gTaskbarTabGroup.destroy();
    win.gTaskbarTabGroup = null;
  },

  resetCacheTimer: function () {
    this.cacheTimer.cancel();
    this.cacheTimer.init(this, 1000*this.cacheLifespan, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  
  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "nsPref:changed":
        if (aData == CACHE_EXPIRATION_TIME_PREF_NAME)
          break;

        if (aData == TOGGLE_PREF_NAME)
          this._prefenabled = this.prefs.getBoolPref(TOGGLE_PREF_NAME);
        else if (aData == DISABLE_THRESHOLD_PREF_NAME)
          this.maxpreviews = this.prefs.getIntPref(DISABLE_THRESHOLD_PREF_NAME);
        
        this.checkPreviewCount();
        break;
      case "timer-callback":
        this.previews.forEach(function (preview) {
          let controller = preview.controller.wrappedJSObject;
          controller.resetCanvasPreview();
        });
        break;
    }
  }
};

XPCOMUtils.defineLazyGetter(AeroPeek, "cacheTimer", function ()
  Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer)
);

XPCOMUtils.defineLazyServiceGetter(AeroPeek, "prefs",
                                   "@mozilla.org/preferences-service;1",
                                   "nsIPrefBranch2");

AeroPeek.initialize();
