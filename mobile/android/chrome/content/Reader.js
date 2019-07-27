




"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode", "resource://gre/modules/ReaderMode.jsm");

let Reader = {
  
  STATUS_UNFETCHED: 0,
  STATUS_FETCH_FAILED_TEMPORARY: 1,
  STATUS_FETCH_FAILED_PERMANENT: 2,
  STATUS_FETCH_FAILED_UNSUPPORTED_FORMAT: 3,
  STATUS_FETCHED_ARTICLE: 4,

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
    for (let msg of this.MESSAGES) {
      window.messageManager.addMessageListener(msg, this);
    }

    Services.obs.addObserver(this, "Reader:Added", false);
    Services.obs.addObserver(this, "Reader:Removed", false);
    Services.obs.addObserver(this, "Gesture:DoubleTap", false);
  },

  observe: function Reader_observe(aMessage, aTopic, aData) {
    switch (aTopic) {
      case "Reader:Added": {
        window.messageManager.broadcastAsyncMessage("Reader:Added", { url: aData });
        break;
      }
      case "Reader:Removed": {
        let uri = Services.io.newURI(aData, null, null);
        ReaderMode.removeArticleFromCache(uri).catch(e => Cu.reportError("Error removing article from cache: " + e));

        window.messageManager.broadcastAsyncMessage("Reader:Removed", { url: aData });
        break;
      }
      case "Gesture:DoubleTap": {
        
        let win = BrowserApp.selectedBrowser.contentWindow;
        let scrollBy;
        
        if (JSON.parse(aData).y < (win.innerHeight / 2)) {
          scrollBy = - win.innerHeight + 50;
        } else {
          scrollBy = win.innerHeight - 50;
        }

        let viewport = BrowserApp.selectedTab.getViewport();
        let newY = Math.min(Math.max(viewport.cssY + scrollBy, viewport.cssPageTop), viewport.cssPageBottom);
        let newRect = new Rect(viewport.cssX, newY, viewport.cssWidth, viewport.cssHeight);
        ZoomHelper.zoomToRect(newRect, -1);
        break;
      }
    }
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "Reader:AddToList":
        this.addArticleToReadingList(message.data.article);
        break;

      case "Reader:ArticleGet":
        this._getArticle(message.data.url, message.target).then((article) => {
          message.target.messageManager.sendAsyncMessage("Reader:ArticleData", { article: article });
        });
        break;

      case "Reader:FaviconRequest": {
        let observer = (s, t, d) => {
          Services.obs.removeObserver(observer, "Reader:FaviconReturn", false);
          message.target.messageManager.sendAsyncMessage("Reader:FaviconReturn", JSON.parse(d));
        };
        Services.obs.addObserver(observer, "Reader:FaviconReturn", false);
        Messaging.sendRequest({
          type: "Reader:FaviconRequest",
          url: message.data.url
        });
        break;
      }

      case "Reader:ListStatusRequest":
        Messaging.sendRequestForResult({
          type: "Reader:ListStatusRequest",
          url: message.data.url
        }).then((data) => {
          message.target.messageManager.sendAsyncMessage("Reader:ListStatusData", JSON.parse(data));
        });
        break;

      case "Reader:RemoveFromList":
        Messaging.sendRequest({
          type: "Reader:RemoveFromList",
          url: message.data.url
        });
        break;

      case "Reader:Share":
        Messaging.sendRequest({
          type: "Reader:Share",
          url: message.data.url,
          title: message.data.title
        });
        break;

      case "Reader:ShowToast":
        NativeWindow.toast.show(message.data.toast, "short");
        break;

      case "Reader:SystemUIVisibility":
        Messaging.sendRequest({
          type: "SystemUI:Visibility",
          visible: message.data.visible
        });
        break;

      case "Reader:ToolbarVisibility":
        Messaging.sendRequest({
          type: "BrowserToolbar:Visibility",
          visible: message.data.visible
        });
        break;

      case "Reader:UpdateIsArticle": {
        let tab = BrowserApp.getTabForBrowser(message.target);
        tab.isArticle = message.data.isArticle;
        this.updatePageAction(tab);
        break;
      }
    }
  },

  pageAction: {
    readerModeCallback: function(tabID) {
      Messaging.sendRequest({
        type: "Reader:Toggle",
        tabID: tabID
      });
    },

    readerModeActiveCallback: function(tabID) {
      Reader._addTabToReadingList(tabID).catch(e => Cu.reportError("Error adding tab to reading list: " + e));
      UITelemetry.addEvent("save.1", "pageaction", null, "reader");
    },
  },

  updatePageAction: function(tab) {
    if (!tab.getActive()) {
      return;
    }

    if (this.pageAction.id) {
      PageActions.remove(this.pageAction.id);
      delete this.pageAction.id;
    }

    if (tab.readerActive) {
      this.pageAction.id = PageActions.add({
        title: Strings.browser.GetStringFromName("readerMode.exit"),
        icon: "drawable://reader_active",
        clickCallback: () => this.pageAction.readerModeCallback(tab.id),
        important: true
      });

      
      
      UITelemetry.startSession("reader.1", null);
      return;
    }

    
    UITelemetry.stopSession("reader.1", "", null);

    if (tab.isArticle) {
      this.pageAction.id = PageActions.add({
        title: Strings.browser.GetStringFromName("readerMode.enter"),
        icon: "drawable://reader",
        clickCallback: () => this.pageAction.readerModeCallback(tab.id),
        longClickCallback: () => this.pageAction.readerModeActiveCallback(tab.id),
        important: true
      });
    }
  },

  _addTabToReadingList: Task.async(function* (tabID) {
    let tab = BrowserApp.getTabForId(tabID);
    if (!tab) {
      throw new Error("Can't add tab to reading list because no tab found for ID: " + tabID);
    }

    let urlWithoutRef = tab.browser.currentURI.specIgnoringRef;
    let article = yield this._getArticle(urlWithoutRef, tab.browser).catch(e => {
      Cu.reportError("Error getting article for tab: " + e);
      return null;
    });
    if (!article) {
      
      
      article = {
        url: urlWithoutRef,
        title: tab.browser.contentDocument.title,
        status: this.STATUS_FETCH_FAILED_UNSUPPORTED_FORMAT,
      };
    } else {
      article.status = this.STATUS_FETCHED_ARTICLE;
    }

    this.addArticleToReadingList(article);
  }),

  addArticleToReadingList: function(article) {
    if (!article || !article.url) {
      Cu.reportError("addArticleToReadingList requires article with valid URL");
      return;
    }

    Messaging.sendRequest({
      type: "Reader:AddToList",
      url: truncate(article.url, MAX_URI_LENGTH),
      title: truncate(article.title || "", MAX_TITLE_LENGTH),
      length: article.length || 0,
      excerpt: article.excerpt || "",
      status: article.status,
    });

    ReaderMode.storeArticleInCache(article).catch(e => Cu.reportError("Error storing article in cache: " + e));
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
  },

  


  migrateCache: Task.async(function* () {
    let cacheDB = yield new Promise((resolve, reject) => {
      let request = window.indexedDB.open("about:reader", 1);
      request.onsuccess = event => resolve(event.target.result);
      request.onerror = event => reject(request.error);

      
      request.onupgradeneeded = event => resolve(null);
    });

    if (!cacheDB) {
      return;
    }

    let articles = yield new Promise((resolve, reject) => {
      let articles = [];

      let transaction = cacheDB.transaction(cacheDB.objectStoreNames);
      let store = transaction.objectStore(cacheDB.objectStoreNames[0]);

      let request = store.openCursor();
      request.onsuccess = event => {
        let cursor = event.target.result;
        if (!cursor) {
          resolve(articles);
        } else {
          articles.push(cursor.value);
          cursor.continue();
        }
      };
      request.onerror = event => reject(request.error);
    });

    for (let article of articles) {
      yield ReaderMode.storeArticleInCache(article);
    }

    
    window.indexedDB.deleteDatabase("about:reader");
  }),
};
