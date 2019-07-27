




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = [ "ReaderParent" ];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode", "resource://gre/modules/ReaderMode.jsm");

let ReaderParent = {

  MESSAGES: [
    "Reader:AddToList",
    "Reader:ArticleGet",
    "Reader:FaviconRequest",
    "Reader:ListStatusRequest",
    "Reader:RemoveFromList",
    "Reader:Share",
    "Reader:ShowToast",
    "Reader:ToolbarVisibility",
    "Reader:SystemUIVisibility",
    "Reader:UpdateIsArticle",
  ],

  init: function() {
    let mm = Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageListenerManager);
    for (let msg of this.MESSAGES) {
      mm.addMessageListener(msg, this);
    }
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "Reader:AddToList":
        
        break;

      case "Reader:ArticleGet":
        this._getArticle(message.data.url, message.target).then((article) => {
          message.target.messageManager.sendAsyncMessage("Reader:ArticleData", { article: article });
        });
        break;

      case "Reader:FaviconRequest": {
        
        break;
      }
      case "Reader:ListStatusRequest":
        
        break;

      case "Reader:RemoveFromList":
        
        break;

      case "Reader:Share":
        
        break;

      case "Reader:ShowToast":
        
        break;

      case "Reader:SystemUIVisibility":
        
        break;

      case "Reader:ToolbarVisibility":
        
        break;

      case "Reader:UpdateIsArticle": {
        
        break;
      }
    }
  },

  








  _getArticle: Task.async(function* (url, browser) {
    
    let article = yield this._getSavedArticle(browser);
    if (article && article.url == url) {
      return article;
    }

    
    let uri = Services.io.newURI(url, null, null);
    article = yield ReaderMode.getArticleFromCache(uri);
    if (article) {
      return article;
    }

    
    
    return yield ReaderMode.downloadAndParseDocument(url);
  }),

  _getSavedArticle: function(browser) {
    return new Promise((resolve, reject) => {
      let mm = browser.messageManager;
      let listener = (message) => {
        mm.removeMessageListener("Reader:SavedArticleData", listener);
        resolve(message.data.article);
      };
      mm.addMessageListener("Reader:SavedArticleData", listener);
      mm.sendAsyncMessage("Reader:SavedArticleGet");
    });
  }
};
