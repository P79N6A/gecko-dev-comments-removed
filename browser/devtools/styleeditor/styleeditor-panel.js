





const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let promise = require("sdk/core/promise");
let EventEmitter = require("devtools/shared/event-emitter");

Cu.import("resource:///modules/devtools/StyleEditorUI.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");

loader.lazyGetter(this, "StyleSheetsFront",
  () => require("devtools/server/actors/styleeditor").StyleSheetsFront);

this.StyleEditorPanel = function StyleEditorPanel(panelWin, toolbox) {
  EventEmitter.decorate(this);

  this._toolbox = toolbox;
  this._target = toolbox.target;
  this._panelWin = panelWin;
  this._panelDoc = panelWin.document;

  this.destroy = this.destroy.bind(this);
  this._showError = this._showError.bind(this);
}

exports.StyleEditorPanel = StyleEditorPanel;

StyleEditorPanel.prototype = {
  get target() this._toolbox.target,

  get panelWindow() this._panelWin,

  


  open: function() {
    let deferred = promise.defer();

    let targetPromise;
    
    if (!this.target.isRemote) {
      targetPromise = this.target.makeRemote();
    } else {
      targetPromise = promise.resolve(this.target);
    }

    targetPromise.then(() => {
      this.target.on("close", this.destroy);

      this._debuggee = StyleSheetsFront(this.target.client, this.target.form);

      this.UI = new StyleEditorUI(this._debuggee, this.target, this._panelDoc);
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
    this.UI.selectStyleSheet(href, line - 1, col ? col - 1 : 0);
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

    return promise.resolve(null);
  },
}

XPCOMUtils.defineLazyGetter(StyleEditorPanel.prototype, "strings",
  function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/styleeditor.properties");
  });
