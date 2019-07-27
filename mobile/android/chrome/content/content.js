




let { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AboutReader", "resource://gre/modules/AboutReader.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode", "resource://gre/modules/ReaderMode.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoginManagerContent", "resource://gre/modules/LoginManagerContent.jsm");

let dump = Cu.import("resource://gre/modules/AndroidLog.jsm", {}).AndroidLog.d.bind(null, "Content");

let global = this;


let AboutReaderListener = {

  _articlePromise: null,

  init: function() {
    addEventListener("AboutReaderContentLoaded", this, false, true);
    addEventListener("DOMContentLoaded", this, false);
    addEventListener("pageshow", this, false);
    addEventListener("pagehide", this, false);
    addMessageListener("Reader:ParseDocument", this);
    addMessageListener("Reader:PushState", this);
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "Reader:ParseDocument":
        this._articlePromise = ReaderMode.parseDocument(content.document).catch(Cu.reportError);
        content.document.location = "about:reader?url=" + encodeURIComponent(message.data.url);
        break;

      case "Reader:PushState":
        this.updateReaderButton(!!(message.data && message.data.isArticle));
        break;
    }
  },

  get isAboutReader() {
    return content.document.documentURI.startsWith("about:reader");
  },

  handleEvent: function(aEvent) {
    if (aEvent.originalTarget.defaultView != content) {
      return;
    }

    switch (aEvent.type) {
      case "AboutReaderContentLoaded":
        if (!this.isAboutReader) {
          return;
        }

        
        
        
        
        if (content.document.body) {
          new AboutReader(global, content, this._articlePromise);
          this._articlePromise = null;
        }
        break;

      case "pagehide":
        sendAsyncMessage("Reader:UpdateReaderButton", { isArticle: false });
        break;

      case "pageshow":
        
        
        if (aEvent.persisted) {
          this.updateReaderButton();
        }
        break;
      case "DOMContentLoaded":
        this.updateReaderButton();
        break;
    }
  },
  updateReaderButton: function(forceNonArticle) {
    if (!ReaderMode.isEnabledForParseOnLoad || this.isAboutReader ||
        !(content.document instanceof content.HTMLDocument) ||
        content.document.mozSyntheticDocument) {
      return;
    }
    
    
    if (ReaderMode.isProbablyReaderable(content.document)) {
      sendAsyncMessage("Reader:UpdateReaderButton", { isArticle: true });
    } else if (forceNonArticle) {
      sendAsyncMessage("Reader:UpdateReaderButton", { isArticle: false });
    }
  },
};
AboutReaderListener.init();

addMessageListener("RemoteLogins:fillForm", function(message) {
  LoginManagerContent.receiveMessage(message, content);
});
