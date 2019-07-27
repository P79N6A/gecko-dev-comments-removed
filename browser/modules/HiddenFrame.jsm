



"use strict";

this.EXPORTED_SYMBOLS = ["HiddenFrame"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/PromiseUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_PAGE = "data:application/vnd.mozilla.xul+xml;charset=utf-8,<window%20id='win'/>";





function HiddenFrame() {}

HiddenFrame.prototype = {
  _frame: null,
  _deferred: null,
  _retryTimerId: null,

  get hiddenDOMDocument() {
    return Services.appShell.hiddenDOMWindow.document;
  },

  get isReady() {
    return this.hiddenDOMDocument.readyState === "complete";
  },

  




  get: function () {
    if (!this._deferred) {
      this._deferred = PromiseUtils.defer();
      this._create();
    }

    return this._deferred.promise;
  },

  destroy: function () {
    clearTimeout(this._retryTimerId);

    if (this._frame) {
      if (!Cu.isDeadWrapper(this._frame)) {
        this._frame.removeEventListener("load", this, true);
        this._frame.remove();
      }

      this._frame = null;
      this._deferred = null;
    }
  },

  handleEvent: function () {
    let contentWindow = this._frame.contentWindow;
    if (contentWindow.location.href === XUL_PAGE) {
      this._frame.removeEventListener("load", this, true);
      this._deferred.resolve(contentWindow);
    } else {
      contentWindow.location = XUL_PAGE;
    }
  },

  _create: function () {
    if (this.isReady) {
      let doc = this.hiddenDOMDocument;
      this._frame = doc.createElementNS(HTML_NS, "iframe");
      this._frame.addEventListener("load", this, true);
      doc.documentElement.appendChild(this._frame);
    } else {
      
      this._retryTimerId = setTimeout(this._create.bind(this), 0);
    }
  }
};
