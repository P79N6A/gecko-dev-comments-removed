




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = [ "ReaderParent" ];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode", "resource://gre/modules/ReaderMode.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReadingList", "resource:///modules/readinglist/ReadingList.jsm");

const gStringBundle = Services.strings.createBundle("chrome://global/locale/aboutReader.properties");

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
    "Reader:SetIntPref",
    "Reader:SetCharPref",
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
        ReadingList.addItem(message.data.article);
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
        ReadingList.count(message.data).then(count => {
          let mm = message.target.messageManager
          
          if (mm) {
            mm.sendAsyncMessage("Reader:ListStatusData",
                                { inReadingList: !!count, url: message.data.url });
          }
        });
        break;

      case "Reader:RemoveFromList":
        
        ReadingList.getItemForURL(message.data.url).then(item => {
          ReadingList.deleteItem(item)
        });
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
      case "Reader:SetIntPref": {
        if (message.data && message.data.name !== undefined) {
          Services.prefs.setIntPref(message.data.name, message.data.value);
        }
        break;
      }
      case "Reader:SetCharPref": {
        if (message.data && message.data.name !== undefined) {
          Services.prefs.setCharPref(message.data.name, message.data.value);
        }
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
      button.setAttribute("tooltiptext", gStringBundle.GetStringFromName("readerView.close"));
    } else {
      button.removeAttribute("readeractive");
      button.setAttribute("tooltiptext", gStringBundle.GetStringFromName("readerView.enter"));
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
    let browser = win.gBrowser.selectedBrowser;
    let url = browser.currentURI.spec;

    if (url.startsWith("about:reader")) {
      let originalURL = this._getOriginalUrl(url);
      if (!originalURL) {
        Cu.reportError("Error finding original URL for about:reader URL: " + url);
      } else {
        win.openUILinkIn(originalURL, "current", {"allowPinnedTabHostChange": true});
      }
    } else {
      browser.messageManager.sendAsyncMessage("Reader:ParseDocument", { url: url });
    }
  },

  parseReaderUrl: function(url) {
    if (!url.startsWith("about:reader?")) {
      return null;
    }
    return this._getOriginalUrl(url);
  },

  






  _getOriginalUrl: function(url) {
    let searchParams = new URLSearchParams(url.substring("about:reader?".length));
    if (!searchParams.has("url")) {
      return null;
    }
    return decodeURIComponent(searchParams.get("url"));
  },

  







  _getArticle: Task.async(function* (url, browser) {
    return yield ReaderMode.downloadAndParseDocument(url);
  })
};
