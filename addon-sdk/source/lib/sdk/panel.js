


"use strict";



module.metadata = {
  "stability": "stable",
  "engines": {
    "Firefox": "*"
  }
};

const { Ci } = require("chrome");
const { validateOptions: valid } = require('./deprecated/api-utils');
const { Symbiont } = require('./content/content');
const { EventEmitter } = require('./deprecated/events');
const { setTimeout } = require('./timers');
const { on, off, emit } = require('./system/events');
const runtime = require('./system/runtime');
const { getDocShell } = require("./frame/utils");
const { getWindow } = require('./panel/window');
const { isPrivateBrowsingSupported } = require('./self');
const { isWindowPBSupported } = require('./private-browsing/utils');
const { getNodeView } = require('./view/core');

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
      ON_SHOW = 'popupshown',
      ON_HIDE = 'popuphidden',
      validNumber = { is: ['number', 'undefined', 'null'] },
      validBoolean = { is: ['boolean', 'undefined', 'null'] },
      ADDON_ID = require('./self').id;

if (isPrivateBrowsingSupported && isWindowPBSupported) {
  throw Error('The panel module cannot be used with per-window private browsing at the moment, see Bug 816257');
}




const Panel = Symbiont.resolve({
  constructor: '_init',
  _onInit: '_onSymbiontInit',
  destroy: '_symbiontDestructor',
  _documentUnload: '_workerDocumentUnload'
}).compose({
  _frame: Symbiont.required,
  _init: Symbiont.required,
  _onSymbiontInit: Symbiont.required,
  _symbiontDestructor: Symbiont.required,
  _emit: Symbiont.required,
  on: Symbiont.required,
  removeListener: Symbiont.required,

  _inited: false,

  





  set _frameLoadersSwapped(value) {
    if (this.__frameLoadersSwapped == value) return;
    this._frame.QueryInterface(Ci.nsIFrameLoaderOwner)
      .swapFrameLoaders(this._viewFrame);
    this.__frameLoadersSwapped = value;
  },
  __frameLoadersSwapped: false,

  constructor: function Panel(options) {
    this._onShow = this._onShow.bind(this);
    this._onHide = this._onHide.bind(this);
    this._onAnyPanelShow = this._onAnyPanelShow.bind(this);
    on('sdk-panel-show', this._onAnyPanelShow);

    this.on('inited', this._onSymbiontInit.bind(this));
    this.on('propertyChange', this._onChange.bind(this));

    options = options || {};
    if ('onShow' in options)
      this.on('show', options.onShow);
    if ('onHide' in options)
      this.on('hide', options.onHide);
    if ('width' in options)
      this.width = options.width;
    if ('height' in options)
      this.height = options.height;
    if ('contentURL' in options)
      this.contentURL = options.contentURL;
    if ('focus' in options) {
      var value = options.focus;
      var validatedValue = valid({ $: value }, { $: validBoolean }).$;
      this._focus =
        (typeof validatedValue == 'boolean') ? validatedValue : this._focus;
    }

    this._init(options);
  },
  _destructor: function _destructor() {
    this.hide();
    this._removeAllListeners('show');
    this._removeAllListeners('hide');
    this._removeAllListeners('propertyChange');
    this._removeAllListeners('inited');
    off('sdk-panel-show', this._onAnyPanelShow);
    
    this._xulPanel = null;
    this._symbiontDestructor(this);
    this._removeAllListeners();
  },
  destroy: function destroy() {
    this._destructor();
  },
  
  get width() this._width,
  set width(value)
    this._width = valid({ $: value }, { $: validNumber }).$ || this._width,
  _width: 320,
  
  get height() this._height,
  set height(value)
    this._height =  valid({ $: value }, { $: validNumber }).$ || this._height,
  _height: 240,
  
  get focus() this._focus,
  _focus: true,

  
  get isShowing() !!this._xulPanel && this._xulPanel.state == "open",

  
  show: function show(anchor) {
    anchor = anchor ? getNodeView(anchor) : null;
    let anchorWindow = getWindow(anchor);

    
    
    if (!anchorWindow) {
      return;
    }

    let document = anchorWindow.document;
    let xulPanel = this._xulPanel;
    let panel = this;
    if (!xulPanel) {
      xulPanel = this._xulPanel = document.createElementNS(XUL_NS, 'panel');
      xulPanel.setAttribute("type", "arrow");

      
      
      
      
      let css = ".panel-inner-arrowcontent, .panel-arrowcontent {padding: 0;}";
      let originalXBL = "chrome://global/content/bindings/popup.xml#arrowpanel";
      let binding =
      '<bindings xmlns="http://www.mozilla.org/xbl">' +
        '<binding id="id" extends="' + originalXBL + '">' +
          '<resources>' +
            '<stylesheet src="data:text/css;charset=utf-8,' +
              document.defaultView.encodeURIComponent(css) + '"/>' +
          '</resources>' +
        '</binding>' +
      '</bindings>';
      xulPanel.style.MozBinding = 'url("data:text/xml;charset=utf-8,' +
        document.defaultView.encodeURIComponent(binding) + '")';

      let frame = document.createElementNS(XUL_NS, 'iframe');
      frame.setAttribute('type', 'content');
      frame.setAttribute('flex', '1');
      frame.setAttribute('transparent', 'transparent');

      if (runtime.OS === "Darwin") {
        frame.style.borderRadius = "6px";
        frame.style.padding = "1px";
      }

      
      
      frame.setAttribute("src","data:;charset=utf-8,");

      xulPanel.appendChild(frame);
      document.getElementById("mainPopupSet").appendChild(xulPanel);
    }
    let { width, height, focus } = this, x, y, position;

    if (!anchor) {
      
      x = document.documentElement.clientWidth / 2 - width / 2;
      y = document.documentElement.clientHeight / 2 - height / 2;
      position = null;
    }
    else {
      
      let rect = anchor.getBoundingClientRect();

      let window = anchor.ownerDocument.defaultView;

      let zoom = window.mozScreenPixelsPerCSSPixel;
      let screenX = rect.left + window.mozInnerScreenX * zoom;
      let screenY = rect.top + window.mozInnerScreenY * zoom;

      
      
      let horizontal, vertical;
      if (screenY > window.screen.availHeight / 2 + height)
        vertical = "top";
      else
        vertical = "bottom";

      if (screenY > window.screen.availWidth / 2 + width)
        horizontal = "left";
      else
        horizontal = "right";

      let verticalInverse = vertical == "top" ? "bottom" : "top";
      position = vertical + "center " + verticalInverse + horizontal;

      
      
      
      xulPanel.setAttribute("flip","both");
    }

    
    
    xulPanel.firstChild.style.width = width + "px";
    xulPanel.firstChild.style.height = height + "px";

    
    
    
    emit('sdk-panel-show', { data: ADDON_ID, subject: xulPanel });

    
    
    xulPanel.setAttribute("noautofocus",!focus);

    
    function waitForBinding() {
      if (!xulPanel.openPopup) {
        setTimeout(waitForBinding, 50);
        return;
      }

      if (xulPanel.state !== 'hiding') {
        xulPanel.openPopup(anchor, position, x, y);
      }
    }
    waitForBinding();

    return this._public;
  },
  
  hide: function hide() {
    
    
    
    
    
    
    
    let xulPanel = this._xulPanel;
    if (xulPanel && "hidePopup" in xulPanel)
      xulPanel.hidePopup();
    return this._public;
  },

  
  resize: function resize(width, height) {
    this.width = width;
    this.height = height;
    
    
    let xulPanel = this._xulPanel;
    if (xulPanel) {
      xulPanel.firstChild.style.width = width + "px";
      xulPanel.firstChild.style.height = height + "px";
    }
  },

  
  
  get _xulPanel() this.__xulPanel,
  set _xulPanel(value) {
    let xulPanel = this.__xulPanel;
    if (value === xulPanel) return;
    if (xulPanel) {
      xulPanel.removeEventListener(ON_HIDE, this._onHide, false);
      xulPanel.removeEventListener(ON_SHOW, this._onShow, false);
      xulPanel.parentNode.removeChild(xulPanel);
    }
    if (value) {
      value.addEventListener(ON_HIDE, this._onHide, false);
      value.addEventListener(ON_SHOW, this._onShow, false);
    }
    this.__xulPanel = value;
  },
  __xulPanel: null,
  get _viewFrame() this.__xulPanel.children[0],
  



  _onHide: function _onHide() {
    try {
      this._frameLoadersSwapped = false;
      this._xulPanel = null;
      this._emit('hide');
    } catch(e) {
      this._emit('error', e);
    }
  },

  




  _applyStyleToDocument: function _applyStyleToDocument() {
    if (this._defaultStyleApplied)
      return;
    try {
      let win = this._xulPanel.ownerDocument.defaultView;
      let node = win.document.getAnonymousElementByAttribute(
        this._xulPanel, "class", "panel-arrowcontent");
      if (!node) {
        
        
        node = win.document.getAnonymousElementByAttribute(
          this._xulPanel, "class", "panel-inner-arrowcontent");
      }
      let textColor = win.getComputedStyle(node).getPropertyValue("color");
      let doc = this._xulPanel.firstChild.contentDocument;
      let style = doc.createElement("style");
      style.textContent = "body { color: " + textColor + "; }";
      let container = doc.head ? doc.head : doc.documentElement;

      if (container.firstChild)
        container.insertBefore(style, container.firstChild);
      else
        container.appendChild(style);
      this._defaultStyleApplied = true;
    }
    catch(e) {
      console.error("Unable to apply panel style");
      console.exception(e);
    }
  },

  



  _onShow: function _onShow() {
    try {
      if (!this._inited) { 
        this.on('inited', this._onShow.bind(this));
      } else {
        this._frameLoadersSwapped = true;
        this._applyStyleToDocument();
        this._emit('show');
      }
    } catch(e) {
      this._emit('error', e);
    }
  },

  



  _onAnyPanelShow: function _onAnyPanelShow(e) {
    if (e.subject !== this._xulPanel)
      this.hide();
  },

  


  _onInit: function _onInit() {
    this._inited = true;

    
    
    let docShell = getDocShell(this._frame);
    if (docShell && "allowWindowControl" in docShell)
      docShell.allowWindowControl = false;

    
    
    
    this._emit('inited');
  },

  
  
  _documentUnload: function(subject, topic, data) {
    if (this._workerDocumentUnload(subject, topic, data)) {
      this._initFrame(this._frame);
      return true;
    }
    return false;
  },

  _onChange: function _onChange(e) {
    this._frameLoadersSwapped = false;
    if ('contentURL' in e && this._frame) {
      
      
      this._workerCleanup();
      this._initFrame(this._frame);
    }
  }
});
exports.Panel = function(options) Panel(options)
exports.Panel.prototype = Panel.prototype;
