





const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
let EventEmitter = require("devtools/toolkit/event-emitter");

Cu.import("resource:///modules/devtools/StyleEditorUI.jsm");
Cu.import("resource:///modules/devtools/StyleEditorUtil.jsm");

loader.lazyGetter(this, "StyleSheetsFront",
  () => require("devtools/server/actors/stylesheets").StyleSheetsFront);

loader.lazyGetter(this, "StyleEditorFront",
  () => require("devtools/server/actors/styleeditor").StyleEditorFront);

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

  


  open: Task.async(function* () {
    
    if (!this.target.isRemote) {
      yield this.target.makeRemote();
    }

    this.target.on("close", this.destroy);

    if (this.target.form.styleSheetsActor) {
      this._debuggee = StyleSheetsFront(this.target.client, this.target.form);
    }
    else {
      
      this._debuggee = StyleEditorFront(this.target.client, this.target.form);
    }

    
    this.UI = new StyleEditorUI(this._debuggee, this.target, this._panelDoc);
    this.UI.on("error", this._showError);
    yield this.UI.initialize();

    this.isReady = true;

    return this;
  }),

  








  _showError: function(event, data) {
    if (!this._toolbox) {
      
      return;
    }

    let errorMessage = _(data.key);
    if (data.append) {
      errorMessage += " " + data.append;
    }

    let notificationBox = this._toolbox.getNotificationBox();
    let notification = notificationBox.getNotificationWithValue("styleeditor-error");
    let level = (data.level === "info") ?
                notificationBox.PRIORITY_INFO_LOW :
                notificationBox.PRIORITY_CRITICAL_LOW;

    if (!notification) {
      notificationBox.appendNotification(errorMessage, "styleeditor-error",
                                         "", level);
    }
  },

  












  selectStyleSheet: function(href, line, col) {
    if (!this._debuggee || !this.UI) {
      return;
    }
    return this.UI.selectStyleSheet(href, line - 1, col ? col - 1 : 0);
  },

  


  destroy: function() {
    if (!this._destroyed) {
      this._destroyed = true;

      this._target.off("close", this.destroy);
      this._target = null;
      this._toolbox = null;
      this._panelDoc = null;
      this._debuggee.destroy();
      this._debuggee = null;

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
