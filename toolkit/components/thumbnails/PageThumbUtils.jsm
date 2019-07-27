








this.EXPORTED_SYMBOLS = ["PageThumbUtils"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Services.jsm");

this.PageThumbUtils = {
  
  THUMBNAIL_BG_COLOR: "#fff",
  
  HTML_NAMESPACE: "http://www.w3.org/1999/xhtml",

  







  createCanvas: function (aWindow) {
    let doc = (aWindow || Services.appShell.hiddenDOMWindow).document;
    let canvas = doc.createElementNS(this.HTML_NAMESPACE, "canvas");
    canvas.mozOpaque = true;
    canvas.mozImageSmoothingEnabled = true;
    let [thumbnailWidth, thumbnailHeight] = this.getThumbnailSize();
    canvas.width = thumbnailWidth;
    canvas.height = thumbnailHeight;
    return canvas;
  },

  






  getThumbnailSize: function () {
    if (!this._thumbnailWidth || !this._thumbnailHeight) {
      let screenManager = Cc["@mozilla.org/gfx/screenmanager;1"]
                            .getService(Ci.nsIScreenManager);
      let left = {}, top = {}, width = {}, height = {};
      screenManager.primaryScreen.GetRectDisplayPix(left, top, width, height);
      this._thumbnailWidth = Math.round(width.value / 3);
      this._thumbnailHeight = Math.round(height.value / 3);
    }
    return [this._thumbnailWidth, this._thumbnailHeight];
  },

  







  determineCropSize: function (aWindow, aCanvas) {
    let utils = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindowUtils);
    
    let sbWidth = {__exposedProps__: {"value": "rw"}},
      sbHeight = {__exposedProps__: {"value": "rw"}};

    try {
      utils.getScrollbarSize(false, sbWidth, sbHeight);
    } catch (e) {
      
      Cu.reportError("Unable to get scrollbar size in determineCropSize.");
      sbWidth.value = sbHeight.value = 0;
    }

    
    
    let sw = aWindow.innerWidth - sbWidth.value;
    let sh = aWindow.innerHeight - sbHeight.value;

    let {width: thumbnailWidth, height: thumbnailHeight} = aCanvas;
    let scale = Math.min(Math.max(thumbnailWidth / sw, thumbnailHeight / sh), 1);
    let scaledWidth = sw * scale;
    let scaledHeight = sh * scale;

    if (scaledHeight > thumbnailHeight)
      sh -= Math.floor(Math.abs(scaledHeight - thumbnailHeight) * scale);

    if (scaledWidth > thumbnailWidth)
      sw -= Math.floor(Math.abs(scaledWidth - thumbnailWidth) * scale);

    return [sw, sh, scale];
  }
};
