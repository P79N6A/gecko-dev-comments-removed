





const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

this.EXPORTED_SYMBOLS = ["StyleEditorPanel"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");
Cu.import("resource:///modules/devtools/StyleEditorDebuggee.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUI.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");


XPCOMUtils.defineLazyModuleGetter(this, "StyleEditorChrome",
                        "resource:///modules/devtools/StyleEditorChrome.jsm");

this.StyleEditorPanel = function StyleEditorPanel(panelWin, toolbox) {
  EventEmitter.decorate(this);

  this._toolbox = toolbox;
  this._target = toolbox.target;
  this._panelWin = panelWin;
  this._panelDoc = panelWin.document;

  this.destroy = this.destroy.bind(this);
  this._showError = this._showError.bind(this);
}

StyleEditorPanel.prototype = {
  get target() this._toolbox.target,

  get panelWindow() this._panelWin,

  


  open: function() {
    let deferred = Promise.defer();

    let promise;
    
    if (!this.target.isRemote) {
      promise = this.target.makeRemote();
    } else {
      promise = Promise.resolve(this.target);
    }

    promise.then(() => {
      this.target.on("close", this.destroy);

      this._debuggee = new StyleEditorDebuggee(this.target);

      this.UI = new StyleEditorUI(this._debuggee, this._panelDoc);
      this.UI.on("error", this._showError);

      this.isReady = true;
      deferred.resolve(this);
    })

    return deferred.promise;
  },

  








  _showError: function(event, errorCode) {
    let message = _(errorCode);
    let notificationBox = this._toolbox.getNotificationBox();
    let notification = notificationBox.getNotificationWithValue("styleeditor-error");
    if (!notification) {
      notificationBox.appendNotification(message,
        "styleeditor-error", "", notificationBox.PRIORITY_CRITICAL_LOW);
    }
  },

  









  selectStyleSheet: function(href, line, col) {
    if (!this._debuggee || !this.UI) {
      return;
    }
    let stylesheet = this._debuggee.styleSheetFromHref(href);
    this.UI.selectStyleSheet(href, line, col);
  },

  


  destroy: function() {
    if (!this._destroyed) {
      this._destroyed = true;

      this._target.off("close", this.destroy);
      this._target = null;
      this._toolbox = null;
      this._panelDoc = null;

      this._debuggee.destroy();
      this.UI.destroy();
    }

    return Promise.resolve(null);
  },
}

XPCOMUtils.defineLazyGetter(StyleEditorPanel.prototype, "strings",
  function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/styleeditor.properties");
  });
