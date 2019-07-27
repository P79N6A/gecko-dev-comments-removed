




let { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AboutReader", "resource://gre/modules/AboutReader.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode", "resource://gre/modules/ReaderMode.jsm");

let dump = Cu.import("resource://gre/modules/AndroidLog.jsm", {}).AndroidLog.d.bind(null, "Content");

let global = this;

let AboutReaderListener = {
  _savedArticle: null,

  init: function() {
    addEventListener("AboutReaderContentLoaded", this, false, true);
    addEventListener("pageshow", this, false);
    addMessageListener("Reader:SavedArticleGet", this);
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "Reader:SavedArticleGet":
        sendAsyncMessage("Reader:SavedArticleData", { article: this._savedArticle });
        break;
    }
  },

  get isAboutReader() {
    return content.document.documentURI.startsWith("about:reader");
  },

  handleEvent: function(event) {
    if (event.originalTarget.defaultView != content) {
      return;
    }

    switch (event.type) {
      case "AboutReaderContentLoaded":
        if (!this.isAboutReader) {
          return;
        }

        
        
        
        
        if (content.document.body) {
          new AboutReader(global, content);
        }
        break;

      case "pageshow":
        if (!ReaderMode.isEnabledForParseOnLoad || this.isAboutReader) {
          return;
        }

        
        this._savedArticle = null;
        sendAsyncMessage("Reader:UpdateReaderButton", { isArticle: false });

        ReaderMode.parseDocument(content.document).then(article => {
          
          if (article === null || content === null) {
            return;
          }

          
          
          let url = Services.io.newURI(content.document.documentURI, null, null).spec;
          if (article.url !== url) {
            return;
          }

          this._savedArticle = article;
          sendAsyncMessage("Reader:UpdateReaderButton", { isArticle: true });

        }).catch(e => Cu.reportError("Error parsing document: " + e));
        break;
    }
  }
};
AboutReaderListener.init();
