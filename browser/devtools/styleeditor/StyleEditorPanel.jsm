





const Cu = Components.utils;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = ["StyleEditorPanel"];

Cu.import("resource:///modules/devtools/EventEmitter.jsm");
Cu.import("resource:///modules/devtools/StyleEditorChrome.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.StyleEditorPanel = function StyleEditorPanel(panelWin, toolbox) {
  new EventEmitter(this);

  this._toolbox = toolbox;
  this._target = toolbox.target;

  this.reset = this.reset.bind(this);
  this.newPage = this.newPage.bind(this);
  this.destroy = this.destroy.bind(this);

  this._target.on("will-navigate", this.reset);
  this._target.on("navigate", this.newPage);
  this._target.on("close", this.destroy);

  this._panelWin = panelWin;
  this._panelDoc = panelWin.document;

  let contentWin = toolbox.target.tab.linkedBrowser.contentWindow;
  this.setPage(contentWin);

  this.isReady = true;
}

StyleEditorPanel.prototype = {
  


  get target() {
    return this._target;
  },

  


  get panelWindow() this._panelWin,

  


  get styleEditorChrome() this._panelWin.styleEditorChrome,

  


  setPage: function StyleEditor_setPage(contentWindow) {
    if (this._panelWin.styleEditorChrome) {
      this._panelWin.styleEditorChrome.contentWindow = contentWindow;
    } else {
      let chromeRoot = this._panelDoc.getElementById("style-editor-chrome");
      let chrome = new StyleEditorChrome(chromeRoot, contentWindow);
      this._panelWin.styleEditorChrome = chrome;
    }
    this.selectStyleSheet(null, null, null);
  },

  


  newPage: function StyleEditor_newPage(event, window) {
    this.setPage(window);
  },

  


  reset: function StyleEditor_reset() {
    this._panelWin.styleEditorChrome.resetChrome();
  },

  


  selectStyleSheet: function StyleEditor_selectStyleSheet(stylesheet, line, col) {
    this._panelWin.styleEditorChrome.selectStyleSheet(stylesheet, line, col);
  },

  


  destroy: function StyleEditor_destroy() {
    if (this._destroyed) {
      return;
    }
    this._destroyed = true;

    this._target.off("will-navigate", this.reset);
    this._target.off("navigate", this.newPage);
    this._target.off("close", this.destroy);
    this._target = null;
    this._toolbox = null;
    this._panelWin = null;
    this._panelDoc = null;
  },
}
