










































this.EXPORTED_SYMBOLS = ["AeroPeek"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");


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


function _imageFromURI(doc, uri, privateMode, callback) {
  let channel = ioSvc.newChannelFromURI2(uri,
                                         doc,
                                         null,  
                                         null,  
                                         Ci.nsILoadInfo.SEC_NORMAL,
                                         Ci.nsIContentPolicy.TYPE_IMAGE);
  try {
    channel.QueryInterface(Ci.nsIPrivateBrowsingChannel);
    channel.setPrivate(privateMode);
  } catch (e) {
    
  }
  NetUtil.asyncFetch2(channel, function(inputStream, resultCode) {
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


function getFaviconAsImage(doc, iconurl, privateMode, callback) {
  if (iconurl)
    _imageFromURI(doc, NetUtil.newURI(iconurl), privateMode, callback);
  else
    _imageFromURI(doc, faviconSvc.defaultFavicon, privateMode, callback);
}


function snapRectAtScale(r, scale) {
  let x = Math.floor(r.x * scale);
  let y = Math.floor(r.y * scale);
  let width = Math.ceil((r.x + r.width) * scale) - x;
  let height = Math.ceil((r.y + r.height) * scale) - y;

  r.x = x / scale;
  r.y = y / scale;
  r.width = width / scale;
  r.height = height / scale;
}

















function PreviewController(win, tab) {
  this.win = win;
  this.tab = tab;
  this.linkedBrowser = tab.linkedBrowser;
  this.preview = this.win.createTabPreview(this);

  this.linkedBrowser.addEventListener("MozAfterPaint", this, false);
  this.linkedBrowser.addEventListener("resize", this, false);
  this.tab.addEventListener("TabAttrModified", this, false);

  XPCOMUtils.defineLazyGetter(this, "canvasPreview", function () {
    let canvas = this.win.win.document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
    canvas.mozOpaque = true;
    return canvas;
  });

  XPCOMUtils.defineLazyGetter(this, "dirtyRegion",
    function () {
      let dirtyRegion = Cc["@mozilla.org/gfx/region;1"]
                       .createInstance(Ci.nsIScriptableRegion);
      dirtyRegion.init();
      return dirtyRegion;
    });

  XPCOMUtils.defineLazyGetter(this, "winutils",
    function () {
      let win = tab.linkedBrowser.contentWindow;
      return win.QueryInterface(Ci.nsIInterfaceRequestor)
                .getInterface(Ci.nsIDOMWindowUtils);
  });
}

PreviewController.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITaskbarPreviewController,
                                         Ci.nsIDOMEventListener]),
  destroy: function () {
    this.tab.removeEventListener("TabAttrModified", this, false);
    this.linkedBrowser.removeEventListener("resize", this, false);
    this.linkedBrowser.removeEventListener("MozAfterPaint", this, false);

    
    
    delete this.win;
    delete this.preview;
    delete this.dirtyRegion;
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
    this.resizeCanvasPreview(0, 0);
  },

  resizeCanvasPreview: function (width, height) {
    this.canvasPreview.width = width;
    this.canvasPreview.height = height;
  },

  get wasResizedSinceLastPreview () {
    let bx = this.linkedBrowser.boxObject;
    return bx.width != this.canvasPreview.width ||
           bx.height != this.canvasPreview.height;
  },

  get zoom() {
    
    
    
    
    return this.winutils.fullZoom;
  },

  
  
  updateCanvasPreview: function () {
    let win = this.linkedBrowser.contentWindow;
    let bx = this.linkedBrowser.boxObject;
    
    
    
    let flushLayout = this.wasResizedSinceLastPreview;
    
    if (flushLayout) {
      
      this.onTabPaint({left:0, top:0, right:win.innerWidth, bottom:win.innerHeight});
      this.resizeCanvasPreview(bx.width, bx.height);
    }

    
    let ctx = this.canvasPreview.getContext("2d");
    let scale = this.zoom;

    let flags = this.canvasPreviewFlags;
    if (flushLayout)
      flags &= ~Ci.nsIDOMCanvasRenderingContext2D.DRAWWINDOW_DO_NOT_FLUSH;

    
    
    this.dirtyRegion.intersectRect(0, 0, win.innerWidth, win.innerHeight);
    this.dirtyRects.forEach(function (r) {
      
      
      snapRectAtScale(r, scale);
      let x = r.x;
      let y = r.y;
      let width = r.width;
      let height = r.height;

      ctx.save();
      ctx.scale(scale, scale);
      ctx.translate(x, y);
      ctx.drawWindow(win, x, y, width, height, "white", flags);
      ctx.restore();
    });
    this.dirtyRegion.setToRect(0,0,0,0);

    
    
    AeroPeek.resetCacheTimer();
  },

  onTabPaint: function (rect) {
    let x = Math.floor(rect.left),
        y = Math.floor(rect.top),
        width = Math.ceil(rect.right) - x,
        height = Math.ceil(rect.bottom) - y;
    this.dirtyRegion.unionRect(x, y, width, height);
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
    
    
    
    let scale = this.winutils.screenPixelsPerCSSPixel / this.winutils.fullZoom;
    ctx.save();
    ctx.scale(scale, scale);
    let width = this.win.width;
    let height = this.win.height;
    
    ctx.drawWindow(this.win.win, 0, 0, width, height, "transparent");

    
    
    
    
    
    if (this.tab.hasAttribute("pending")) {
      
      
      this.updateCanvasPreview();

      let boxObject = this.linkedBrowser.boxObject;
      ctx.translate(boxObject.x, boxObject.y);
      ctx.drawImage(this.canvasPreview, 0, 0);
    }

    ctx.restore();
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
      case "TabAttrModified":
        this.updateTitleAndTooltip();
        break;
      case "resize":
        
        
        
        
        this.win.previews.forEach(function (p) {
          let controller = p.controller.wrappedJSObject;
          if (controller.wasResizedSinceLastPreview) {
            controller.resetCanvasPreview();
            p.invalidate();
          }
        });
        break;
    }
  }
};

XPCOMUtils.defineLazyGetter(PreviewController.prototype, "canvasPreviewFlags",
  function () { let canvasInterface = Ci.nsIDOMCanvasRenderingContext2D;
                return canvasInterface.DRAWWINDOW_DRAW_VIEW
                     | canvasInterface.DRAWWINDOW_DRAW_CARET
                     | canvasInterface.DRAWWINDOW_ASYNC_DECODE_IMAGES
                     | canvasInterface.DRAWWINDOW_DO_NOT_FLUSH;
});










function TabWindow(win) {
  this.win = win;
  this.tabbrowser = win.gBrowser;

  this.previews = new Map();

  for (let i = 0; i < this.tabEvents.length; i++)
    this.tabbrowser.tabContainer.addEventListener(this.tabEvents[i], this, false);
  this.tabbrowser.addTabsProgressListener(this);

  for (let i = 0; i < this.winEvents.length; i++)
    this.win.addEventListener(this.winEvents[i], this, false);

  AeroPeek.windows.push(this);
  let tabs = this.tabbrowser.tabs;
  for (let i = 0; i < tabs.length; i++)
    this.newTab(tabs[i]);

  this.updateTabOrdering();
  AeroPeek.checkPreviewCount();
}

TabWindow.prototype = {
  _enabled: false,
  tabEvents: ["TabOpen", "TabClose", "TabSelect", "TabMove"],
  winEvents: ["tabviewshown", "tabviewhidden"],

  destroy: function () {
    this._destroying = true;

    let tabs = this.tabbrowser.tabs;

    this.tabbrowser.removeTabsProgressListener(this);
    for (let i = 0; i < this.tabEvents.length; i++)
      this.tabbrowser.tabContainer.removeEventListener(this.tabEvents[i], this, false);

    for (let i = 0; i < this.winEvents.length; i++)
      this.win.removeEventListener(this.winEvents[i], this, false);

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
    
    this.previews.set(tab, controller.preview);
    AeroPeek.addPreview(controller.preview);
    
    
    controller.updateTitleAndTooltip();
  },

  createTabPreview: function (controller) {
    let docShell = this.win
                  .QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsIDocShell);
    let preview = AeroPeek.taskbar.createTaskbarTabPreview(docShell, controller);
    preview.visible = AeroPeek.enabled;
    preview.active = this.tabbrowser.selectedTab == controller.tab;
    
    getFaviconAsImage(
      controller.linkedBrowser.contentWindow.document,
      null,
      PrivateBrowsingUtils.isWindowPrivate(this.win),
      function (img) {
        
        
        if (!preview.icon)
          preview.icon = img;
      });
    return preview;
  },

  
  removeTab: function (tab) {
    let preview = this.previewFromTab(tab);
    preview.active = false;
    preview.visible = false;
    preview.move(null);
    preview.controller.wrappedJSObject.destroy();

    this.previews.delete(tab);
    AeroPeek.removePreview(preview);
  },

  get enabled () {
    return this._enabled;
  },

  set enabled (enable) {
    this._enabled = enable;
    
    
    
    for (let [tab, preview] of this.previews) {
      preview.move(null);
      preview.visible = enable;
    }
    this.updateTabOrdering();
  },

  previewFromTab: function (tab) {
    return this.previews.get(tab);
  },

  updateTabOrdering: function () {
    let previews = this.previews;
    let tabs = this.tabbrowser.tabs;

    
    
    let inorder = [previews.get(t) for (t of tabs) if (previews.has(t))];

    
    
    
    
    for (let i = inorder.length - 1; i >= 0; i--) {
      inorder[i].move(inorder[i + 1] || null);
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
        this.updateTabOrdering();
        break;
      case "tabviewshown":
        this.enabled = false;
        break;
      case "tabviewhidden":
        if (!AeroPeek._prefenabled)
          return;
        this.enabled = true;
        break;
    }
  },

  
  onLinkIconAvailable: function (aBrowser, aIconURL) {
    let self = this;
    getFaviconAsImage(
      aBrowser.contentWindow.document,
      aIconURL,PrivateBrowsingUtils.isWindowPrivate(this.win),
      function (img) {
        let index = self.tabbrowser.browsers.indexOf(aBrowser);
        
        if (index != -1) {
          let tab = self.tabbrowser.tabs[index];
          self.previews.get(tab).icon = img;
        }
      });
  }
}








this.AeroPeek = {
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

  destroy: function destroy() {
    this._enabled = false;

    this.prefs.removeObserver(TOGGLE_PREF_NAME, this);
    this.prefs.removeObserver(DISABLE_THRESHOLD_PREF_NAME, this);
    this.prefs.removeObserver(CACHE_EXPIRATION_TIME_PREF_NAME, this);

    if (this.cacheTimer)
      this.cacheTimer.cancel();
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
    delete win.gTaskbarTabGroup;

    if (this.windows.length == 0)
      this.destroy();
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
                                   "nsIPrefBranch");

AeroPeek.initialize();
