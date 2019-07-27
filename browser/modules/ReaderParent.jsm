




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = [ "ReaderParent" ];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode", "resource://gre/modules/ReaderMode.jsm");

const gStringBundle = Services.strings.createBundle("chrome://browser/locale/readerMode.properties");

let ReaderParent = {

  MESSAGES: [
    "Reader:AddToList",
    "Reader:ArticleGet",
    "Reader:FaviconRequest",
    "Reader:ListStatusRequest",
    "Reader:RemoveFromList",
    "Reader:Share",
    "Reader:SystemUIVisibility",
    "Reader:UpdateReaderButton",
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
          
          if (message.target.messageManager) {
            message.target.messageManager.sendAsyncMessage("Reader:ArticleData", { article: article });
          }
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

      case "Reader:SystemUIVisibility":
        
        break;

      case "Reader:UpdateReaderButton": {
        let browser = message.target;
        if (message.data && message.data.isArticle !== undefined) {
          browser.isArticle = message.data.isArticle;
        }
        this.updateReaderButton(browser);
        break;
      }
    }
  },

  updateReaderButton: function(browser) {
    let win = browser.ownerDocument.defaultView;
    if (browser != win.gBrowser.selectedBrowser) {
      return;
    }

    let button = win.document.getElementById("reader-mode-button");
    if (browser.currentURI.spec.startsWith("about:reader")) {
      button.setAttribute("readeractive", true);
      button.hidden = false;
      button.setAttribute("tooltiptext", gStringBundle.GetStringFromName("readerMode.exit"));
    } else {
      button.removeAttribute("readeractive");
      button.setAttribute("tooltiptext", gStringBundle.GetStringFromName("readerMode.enter"));
      button.hidden = !browser.isArticle;
    }
  },

  handleReaderButtonEvent: function(event) {
    event.stopPropagation();

    if ((event.type == "click" && event.button != 0) ||
        (event.type == "keypress" && event.charCode != Ci.nsIDOMKeyEvent.DOM_VK_SPACE &&
         event.keyCode != Ci.nsIDOMKeyEvent.DOM_VK_RETURN)) {
      return; 
    }

    let win = event.target.ownerDocument.defaultView;
    let url = win.gBrowser.selectedBrowser.currentURI.spec;
    if (url.startsWith("about:reader")) {
      win.openUILinkIn(this._getOriginalUrl(url), "current");
    } else {
      win.openUILinkIn("about:reader?url=" + encodeURIComponent(url), "current");
    }
  },

  




  _getOriginalUrl: function(url) {
    let searchParams = new URLSearchParams(url.split("?")[1]);
    if (!searchParams.has("url")) {
      Cu.reportError("Error finding original URL for about:reader URL: " + url);
      return url;
    }
    return decodeURIComponent(searchParams.get("url"));
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
