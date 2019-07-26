



const {Cc, Ci, Cu} = require("chrome");
const {rgbToHsl} = require("devtools/css-color").colorUtils;
const {EventEmitter} = Cu.import("resource://gre/modules/devtools/event-emitter.js");

Cu.import("resource://gre/modules/Services.jsm");

loader.lazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"]
    .getService(Ci.nsIClipboardHelper);
});

loader.lazyGetter(this, "ssService", function() {
  return Cc["@mozilla.org/content/style-sheet-service;1"]
    .getService(Ci.nsIStyleSheetService);
});

loader.lazyGetter(this, "ioService", function() {
  return Cc["@mozilla.org/network/io-service;1"]
    .getService(Ci.nsIIOService);
});

loader.lazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});

loader.lazyGetter(this, "XULRuntime", function() {
  return Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime);
});

loader.lazyGetter(this, "l10n", () => Services.strings
  .createBundle("chrome://browser/locale/devtools/eyedropper.properties"));

const EYEDROPPER_URL = "chrome://browser/content/devtools/eyedropper.xul";
const CROSSHAIRS_URL = "chrome://browser/content/devtools/eyedropper/crosshairs.css";
const NOCURSOR_URL = "chrome://browser/content/devtools/eyedropper/nocursor.css";

const ZOOM_PREF = "devtools.eyedropper.zoom";
const FORMAT_PREF = "devtools.defaultColorUnit";

const CANVAS_WIDTH = 96;
const CANVAS_OFFSET = 3; 
const CLOSE_DELAY = 750;

const HEX_BOX_WIDTH = CANVAS_WIDTH + CANVAS_OFFSET * 2;
const HSL_BOX_WIDTH = 158;



















function Eyedropper(chromeWindow, opts = { copyOnSelect: true }) {
  this.copyOnSelect = opts.copyOnSelect;

  this._onFirstMouseMove = this._onFirstMouseMove.bind(this);
  this._onMouseMove = this._onMouseMove.bind(this);
  this._onMouseDown = this._onMouseDown.bind(this);
  this._onKeyDown = this._onKeyDown.bind(this);
  this._onFrameLoaded = this._onFrameLoaded.bind(this);

  this._chromeWindow = chromeWindow;
  this._chromeDocument = chromeWindow.document;

  this._dragging = true;
  this.loaded = false;

  this.format = Services.prefs.getCharPref(FORMAT_PREF); 
  this.zoom = Services.prefs.getIntPref(ZOOM_PREF);      

  this._zoomArea = {
    x: 0,          
    y: 0,          
    width: CANVAS_WIDTH,      
    height: CANVAS_WIDTH      
  };
  EventEmitter.decorate(this);
}

exports.Eyedropper = Eyedropper;

Eyedropper.prototype = {
  


  get cellsWide() {
    
    
    let cellsWide = Math.ceil(this._zoomArea.width / this.zoom);
    cellsWide += cellsWide % 2;

    return cellsWide;
  },

  


  get cellSize() {
    return this._zoomArea.width / this.cellsWide;
  },

  


  get centerCell() {
    return Math.floor(this.cellsWide / 2);
  },

  


  get centerColor() {
    let x = y = (this.centerCell * this.cellSize) + (this.cellSize / 2);
    let rgb = this._ctx.getImageData(x, y, 1, 1).data;
    return rgb;
  },

  



  open: function() {
    if (this.isOpen) {
      
      return;
    }
    this.isOpen = true;

    this._OS = XULRuntime.OS;

    this._chromeDocument.addEventListener("mousemove", this._onFirstMouseMove);

    this._showCrosshairs();
  },

  



  _onFirstMouseMove: function(event) {
    this._chromeDocument.removeEventListener("mousemove", this._onFirstMouseMove);

    this._panel = this._buildPanel();

    let popupSet = this._chromeDocument.querySelector("#mainPopupSet");
    popupSet.appendChild(this._panel);

    let { panelX, panelY } = this._getPanelCoordinates(event);
    this._panel.openPopupAtScreen(panelX, panelY);

    this._setCoordinates(event);

    this._addListeners();

    
    this._hideCrosshairs();
    this._hideCursor();
  },

  





  _setCoordinates: function(event) {
    let win = this._chromeWindow;

    let x, y;
    if (this._OS == "Linux") {
      
      let windowX = win.screenX + (win.outerWidth - win.innerWidth);
      x = event.screenX - windowX;

      let windowY = win.screenY + (win.outerHeight - win.innerHeight);
      y = event.screenY - windowY;
    }
    else {
      x = event.clientX;
      y = event.clientY;
    }

    
    x = Math.max(0, Math.min(x, win.outerWidth - 1));
    y = Math.max(0, Math.min(y, win.outerHeight - 1));

    this._zoomArea.x = x;
    this._zoomArea.y = y;
  },

  





  _buildPanel: function() {
    let panel = this._chromeDocument.createElement("panel");
    panel.setAttribute("noautofocus", true);
    panel.setAttribute("noautohide", true);
    panel.setAttribute("level", "floating");
    panel.setAttribute("class", "devtools-eyedropper-panel");

    let iframe = this._iframe = this._chromeDocument.createElement("iframe");
    iframe.addEventListener("load", this._onFrameLoaded, true);
    iframe.setAttribute("flex", "1");
    iframe.setAttribute("transparent", "transparent");
    iframe.setAttribute("allowTransparency", true);
    iframe.setAttribute("class", "devtools-eyedropper-iframe");
    iframe.setAttribute("src", EYEDROPPER_URL);
    iframe.setAttribute("width", CANVAS_WIDTH);
    iframe.setAttribute("height", CANVAS_WIDTH);

    panel.appendChild(iframe);

    return panel;
  },

  



  _onFrameLoaded: function() {
    this._iframe.removeEventListener("load", this._onFrameLoaded, true);

    this._iframeDocument = this._iframe.contentDocument;
    this._colorPreview = this._iframeDocument.querySelector("#color-preview");
    this._colorValue = this._iframeDocument.querySelector("#color-value");

    
    let valueBox = this._iframeDocument.querySelector("#color-value-box");
    if (this.format == "hex") {
      valueBox.style.width = HEX_BOX_WIDTH + "px";
    }
    else if (this.format == "hsl") {
      valueBox.style.width = HSL_BOX_WIDTH + "px";
    }

    this._canvas = this._iframeDocument.querySelector("#canvas");
    this._ctx = this._canvas.getContext("2d");

    
    this._ctx.mozImageSmoothingEnabled = false;

    this._drawWindow();

    this._addPanelListeners();
    this._iframe.focus();

    this.loaded = true;
    this.emit("load");
  },

  


  _addPanelListeners: function() {
    this._iframeDocument.addEventListener("keydown", this._onKeyDown);

    let closeCmd = this._iframeDocument.getElementById("eyedropper-cmd-close");
    closeCmd.addEventListener("command", this.destroy.bind(this), true);

    let copyCmd = this._iframeDocument.getElementById("eyedropper-cmd-copy");
    copyCmd.addEventListener("command", this.selectColor.bind(this), true);
  },

  


  _removePanelListeners: function() {
    this._iframeDocument.removeEventListener("keydown", this._onKeyDown);
  },

  


  _addListeners: function() {
    this._chromeDocument.addEventListener("mousemove", this._onMouseMove);
    this._chromeDocument.addEventListener("mousedown", this._onMouseDown);
  },

  


  _removeListeners: function() {
    this._chromeDocument.removeEventListener("mousemove", this._onFirstMouseMove);
    this._chromeDocument.removeEventListener("mousemove", this._onMouseMove);
    this._chromeDocument.removeEventListener("mousedown", this._onMouseDown);
  },

  


  _hideCursor: function() {
    registerStyleSheet(NOCURSOR_URL);
  },

  


  _resetCursor: function() {
    unregisterStyleSheet(NOCURSOR_URL);
  },

  


  _showCrosshairs: function() {
    registerStyleSheet(CROSSHAIRS_URL);
  },

  


  _hideCrosshairs: function() {
    unregisterStyleSheet(CROSSHAIRS_URL);
  },

  






  _onMouseMove: function(event) {
    if (!this._dragging || !this._panel || !this._canvas) {
      return;
    }

    this._setCoordinates(event);
    this._drawWindow();

    let { panelX, panelY } = this._getPanelCoordinates(event);
    this._movePanel(panelX, panelY);
  },

  









  _getPanelCoordinates: function({screenX, screenY}) {
    let win = this._chromeWindow;
    let offset = CANVAS_WIDTH / 2 + CANVAS_OFFSET;

    let panelX = screenX - offset;
    let windowX = win.screenX + (win.outerWidth - win.innerWidth);
    let maxX = win.screenX + win.outerWidth - offset - 1;

    let panelY = screenY - offset;
    let windowY = win.screenY + (win.outerHeight - win.innerHeight);
    let maxY = win.screenY + win.outerHeight - offset - 1;

    
    panelX = Math.max(windowX - offset, Math.min(panelX, maxX));
    panelY = Math.max(windowY - offset, Math.min(panelY, maxY));

    return { panelX: panelX, panelY: panelY };
  },

  







  _movePanel: function(screenX, screenY) {
    this._panelX = screenX;
    this._panelY = screenY;

    this._panel.moveTo(screenX, screenY);
  },

  






  _onMouseDown: function(event) {
    event.preventDefault();
    event.stopPropagation();

    this.selectColor();
  },

  



  selectColor: function() {
    if (this._isSelecting) {
      return;
    }
    this._isSelecting = true;
    this._dragging = false;

    this.emit("select", this._colorValue.value);

    if (this.copyOnSelect) {
      this.copyColor(this.destroy.bind(this));
    }
    else {
      this.destroy();
    }
  },

  





  copyColor: function(callback) {
    Services.appShell.hiddenDOMWindow.clearTimeout(this._copyTimeout);

    let color = this._colorValue.value;
    clipboardHelper.copyString(color);

    this._colorValue.classList.add("highlight");
    this._colorValue.value = "âœ“ " + l10n.GetStringFromName("colorValue.copied");

    this._copyTimeout = Services.appShell.hiddenDOMWindow.setTimeout(() => {
      this._colorValue.classList.remove("highlight");
      this._colorValue.value = color;

      if (callback) {
        callback();
      }
    }, CLOSE_DELAY);
  },

  






  _onKeyDown: function(event) {
    if (event.metaKey && event.keyCode === event.DOM_VK_C) {
      this.copyColor();
      return;
    }

    let offsetX = 0;
    let offsetY = 0;
    let modifier = 1;

    if (event.keyCode === event.DOM_VK_LEFT) {
      offsetX = -1;
    }
    if (event.keyCode === event.DOM_VK_RIGHT) {
      offsetX = 1;
    }
    if (event.keyCode === event.DOM_VK_UP) {
      offsetY = -1;
    }
    if (event.keyCode === event.DOM_VK_DOWN) {
      offsetY = 1;
    }
    if (event.shiftKey) {
      modifier = 10;
    }

    offsetY *= modifier;
    offsetX *= modifier;

    if (offsetX !== 0 || offsetY !== 0) {
      this._zoomArea.x += offsetX;
      this._zoomArea.y += offsetY;

      this._drawWindow();

      this._movePanel(this._panelX + offsetX, this._panelY + offsetY);

      event.preventDefault();
    }
  },

  


  _drawWindow: function() {
    let { width, height, x, y } = this._zoomArea;

    let zoomedWidth = width / this.zoom;
    let zoomedHeight = height / this.zoom;

    let drawX = x - (zoomedWidth / 2);
    let drawY = y - (zoomedHeight / 2);

    
    this._ctx.drawWindow(this._chromeWindow, drawX, drawY, zoomedWidth,
                        zoomedHeight, "white");

    
    let sx = 0;
    let sy = 0;
    let sw = zoomedWidth;
    let sh = zoomedHeight;
    let dx = 0;
    let dy = 0;
    let dw = width;
    let dh = height;

    this._ctx.drawImage(this._canvas, sx, sy, sw, sh, dx, dy, dw, dh);

    let rgb = this.centerColor;
    this._colorPreview.style.backgroundColor = toColorString(rgb, "rgb");
    this._colorValue.value = toColorString(rgb, this.format);

    if (this.zoom > 2) {
      
      this._drawGrid();
    }
    this._drawCrosshair();
  },

  


  _drawGrid: function() {
    let { width, height } = this._zoomArea;

    this._ctx.lineWidth = 1;
    this._ctx.strokeStyle = "rgba(143, 143, 143, 0.2)";

    for (let i = 0; i < width; i += this.cellSize) {
      this._ctx.beginPath();
      this._ctx.moveTo(i - .5, 0);
      this._ctx.lineTo(i - .5, height);
      this._ctx.stroke();

      this._ctx.beginPath();
      this._ctx.moveTo(0, i - .5);
      this._ctx.lineTo(width, i - .5);
      this._ctx.stroke();
    }
  },

  


  _drawCrosshair: function() {
    let x = y = this.centerCell * this.cellSize;

    this._ctx.lineWidth = 1;
    this._ctx.lineJoin = 'miter';
    this._ctx.strokeStyle = "rgba(0, 0, 0, 1)";
    this._ctx.strokeRect(x - 1.5, y - 1.5, this.cellSize + 2, this.cellSize + 2);

    this._ctx.strokeStyle = "rgba(255, 255, 255, 1)";
    this._ctx.strokeRect(x - 0.5, y - 0.5, this.cellSize, this.cellSize);
  },

  


  destroy: function() {
    this._resetCursor();
    this._hideCrosshairs();

    if (this._panel) {
      this._panel.hidePopup();
      this._panel.remove();
      this._panel = null;
    }
    this._removePanelListeners();
    this._removeListeners();

    this.isOpen = false;
    this._isSelecting = false;

    this.emit("destroy");
  }
}




function registerStyleSheet(url) {
  var uri = ioService.newURI(url, null, null);
  if (!ssService.sheetRegistered(uri, ssService.AGENT_SHEET)) {
    ssService.loadAndRegisterSheet(uri, ssService.AGENT_SHEET);
  }
}




function unregisterStyleSheet(url) {
  var uri = ioService.newURI(url, null, null);
  if (ssService.sheetRegistered(uri, ssService.AGENT_SHEET)) {
    ssService.unregisterSheet(uri, ssService.AGENT_SHEET);
  }
}












function toColorString(rgb, format) {
  let [r,g,b] = rgb;

  switch(format) {
    case "hex":
      return hexString(rgb);
    case "rgb":
      return "rgb(" + r + ", " + g + ", " + b + ")";
    case "hsl":
      let [h,s,l] = rgbToHsl(rgb);
      return "hsl(" + h + ", " + s + "%, " + l + "%)";
    case "name":
      let str;
      try {
        str = DOMUtils.rgbToColorName(r, g, b);
      } catch(e) {
        str = hexString(rgb);
      }
      return str;
    default:
      return hexString(rgb);
  }
}










function hexString([r,g,b]) {
  let val = (1 << 24) + (r << 16) + (g << 8) + (b << 0);
  return "#" + val.toString(16).substr(-6).toUpperCase();
}
