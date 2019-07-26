





const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

this.EXPORTED_SYMBOLS = ["StyleEditorPanel"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource:///modules/devtools/EventEmitter.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "StyleEditorChrome",
                        "resource:///modules/devtools/StyleEditorChrome.jsm");

this.StyleEditorPanel = function StyleEditorPanel(panelWin, toolbox) {
  EventEmitter.decorate(this);

  this._toolbox = toolbox;
  this._target = toolbox.target;

  this.newPage = this.newPage.bind(this);
  this.destroy = this.destroy.bind(this);
  this.beforeNavigate = this.beforeNavigate.bind(this);

  this._target.on("will-navigate", this.beforeNavigate);
  this._target.on("navigate", this.newPage);
  this._target.on("close", this.destroy);

  this._panelWin = panelWin;
  this._panelDoc = panelWin.document;
}

StyleEditorPanel.prototype = {
  


  open: function StyleEditor_open() {
    let contentWin = this._toolbox.target.window;
    let deferred = Promise.defer();

    this.setPage(contentWin).then(function() {
      this.isReady = true;
      deferred.resolve(this);
    }.bind(this));

    return deferred.promise;
  },

  


  get target() this._target,

  


  get panelWindow() this._panelWin,

  


  get styleEditorChrome() this._panelWin.styleEditorChrome,

  


  setPage: function StyleEditor_setPage(contentWindow) {
    if (this._panelWin.styleEditorChrome) {
      this._panelWin.styleEditorChrome.contentWindow = contentWindow;
      this.selectStyleSheet(null, null, null);
    } else {
      let chromeRoot = this._panelDoc.getElementById("style-editor-chrome");
      let chrome = new StyleEditorChrome(chromeRoot, contentWindow);
      let promise = chrome.open();

      this._panelWin.styleEditorChrome = chrome;
      this.selectStyleSheet(null, null, null);
      return promise;
    }
  },

  


  newPage: function StyleEditor_newPage(event, window) {
    this.reset();
    this.setPage(window);
  },

  


  beforeNavigate: function StyleEditor_beforeNavigate(event, request) {
    if (this.styleEditorChrome.isDirty) {
      this.preventNavigate(request);
    }
  },

  


  preventNavigate: function StyleEditor_preventNavigate(request) {
    request.suspend();

    let notificationBox = null;
    if (this.target.isLocalTab) {
      let gBrowser = this.target.tab.ownerDocument.defaultView.gBrowser;
      notificationBox = gBrowser.getNotificationBox();
    }
    else {
      notificationBox = this._toolbox.getNotificationBox();
    }

    let notification = notificationBox.
      getNotificationWithValue("styleeditor-page-navigation");

    if (notification) {
      notificationBox.removeNotification(notification, true);
    }

    let cancelRequest = function onCancelRequest() {
      if (request) {
        request.cancel(Cr.NS_BINDING_ABORTED);
        request.resume(); 
        request = null;
      }
    };

    let eventCallback = function onNotificationCallback(event) {
      if (event == "removed") {
        cancelRequest();
      }
    };

    let buttons = [
      {
        id: "styleeditor.confirmNavigationAway.buttonLeave",
        label: this.strings.GetStringFromName("confirmNavigationAway.buttonLeave"),
        accessKey: this.strings.GetStringFromName("confirmNavigationAway.buttonLeaveAccesskey"),
        callback: function onButtonLeave() {
          if (request) {
            request.resume();
            request = null;
          }
        }.bind(this),
      },
      {
        id: "styleeditor.confirmNavigationAway.buttonStay",
        label: this.strings.GetStringFromName("confirmNavigationAway.buttonStay"),
        accessKey: this.strings.GetStringFromName("confirmNavigationAway.buttonStayAccesskey"),
        callback: cancelRequest
      },
    ];

    let message = this.strings.GetStringFromName("confirmNavigationAway.message");

    notification = notificationBox.appendNotification(message,
      "styleeditor-page-navigation", "chrome://browser/skin/Info.png",
      notificationBox.PRIORITY_WARNING_HIGH, buttons, eventCallback);

    
    
    notification.persistence = -1;
  },


  


  reset: function StyleEditor_reset() {
    this._panelWin.styleEditorChrome.resetChrome();
  },

  


  selectStyleSheet: function StyleEditor_selectStyleSheet(stylesheet, line, col) {
    this._panelWin.styleEditorChrome.selectStyleSheet(stylesheet, line, col);
  },

  


  destroy: function StyleEditor_destroy() {
    if (!this._destroyed) {
      this._destroyed = true;

      this._target.off("will-navigate", this.beforeNavigate);
      this._target.off("navigate", this.newPage);
      this._target.off("close", this.destroy);
      this._target = null;
      this._toolbox = null;
      this._panelWin = null;
      this._panelDoc = null;
    }

    return Promise.resolve(null);
  },
}

XPCOMUtils.defineLazyGetter(StyleEditorPanel.prototype, "strings",
  function () {
    return Services.strings.createBundle(
            "chrome://browser/locale/devtools/styleeditor.properties");
  });