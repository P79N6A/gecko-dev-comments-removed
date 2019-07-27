




"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode", "resource://gre/modules/ReaderMode.jsm");

let Reader = {
  
  STATUS_UNFETCHED: 0,
  STATUS_FETCH_FAILED_TEMPORARY: 1,
  STATUS_FETCH_FAILED_PERMANENT: 2,
  STATUS_FETCH_FAILED_UNSUPPORTED_FORMAT: 3,
  STATUS_FETCHED_ARTICLE: 4,

  get _hasUsedToolbar() {
    delete this._hasUsedToolbar;
    return this._hasUsedToolbar = Services.prefs.getBoolPref("reader.has_used_toolbar");
  },

  observe: function Reader_observe(aMessage, aTopic, aData) {
    switch (aTopic) {
      case "Reader:FetchContent": {
        let data = JSON.parse(aData);
        this._fetchContent(data.url, data.id);
        break;
      }
      case "Reader:Removed": {
        let uri = Services.io.newURI(aData, null, null);
        ReaderMode.removeArticleFromCache(uri).catch(e => Cu.reportError("Error removing article from cache: " + e));

        let mm = window.getGroupMessageManager("browsers");
        mm.broadcastAsyncMessage("Reader:Removed", { url: aData });
        break;
      }
    }
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "Reader:AddToList": {
        
        let article = message.data.article;
        article.status = this.STATUS_FETCHED_ARTICLE;
        this._addArticleToReadingList(article);
        break;
      }
      case "Reader:ArticleGet":
        this._getArticle(message.data.url, message.target).then((article) => {
          message.target.messageManager.sendAsyncMessage("Reader:ArticleData", { article: article });
        });
        break;

      case "Reader:FaviconRequest": {
        Messaging.sendRequestForResult({
          type: "Reader:FaviconRequest",
          url: message.data.url
        }).then(data => {
          message.target.messageManager.sendAsyncMessage("Reader:FaviconReturn", JSON.parse(data));
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

      case "Reader:SystemUIVisibility":
        Messaging.sendRequest({
          type: "SystemUI:Visibility",
          visible: message.data.visible
        });
        break;

      case "Reader:ToolbarHidden":
        if (!this._hasUsedToolbar) {
          NativeWindow.toast.show(Strings.browser.GetStringFromName("readerMode.toolbarTip"), "short");
          Services.prefs.setBoolPref("reader.has_used_toolbar", true);
          this._hasUsedToolbar = true;
        }
        break;

      case "Reader:UpdateReaderButton": {
        let tab = BrowserApp.getTabForBrowser(message.target);
        tab.browser.isArticle = message.data.isArticle;
        this.updatePageAction(tab);
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

    let browser = tab.browser;
    if (browser.currentURI.spec.startsWith("about:reader")) {
      this.pageAction.id = PageActions.add({
        title: Strings.reader.GetStringFromName("readerView.close"),
        icon: "drawable://reader_active",
        clickCallback: () => this.pageAction.readerModeCallback(tab.id),
        important: true
      });

      
      
      UITelemetry.startSession("reader.1", null);
      return;
    }

    
    UITelemetry.stopSession("reader.1", "", null);

    if (browser.isArticle) {
      this.pageAction.id = PageActions.add({
        title: Strings.reader.GetStringFromName("readerView.enter"),
        icon: "drawable://reader",
        clickCallback: () => this.pageAction.readerModeCallback(tab.id),
        longClickCallback: () => this.pageAction.readerModeActiveCallback(tab.id),
        important: true
      });
    }
  },

  


  _fetchContent: function(url, id) {
    this._downloadAndCacheArticle(url).then(article => {
      if (article == null) {
        Messaging.sendRequest({
          type: "Reader:UpdateList",
          id: id,
          status: this.STATUS_FETCH_FAILED_UNSUPPORTED_FORMAT,
        });
      } else {
        Messaging.sendRequest({
          type: "Reader:UpdateList",
          id: id,
          url: truncate(article.url, MAX_URI_LENGTH),
          title: truncate(article.title, MAX_TITLE_LENGTH),
          length: article.length,
          excerpt: article.excerpt,
          status: this.STATUS_FETCHED_ARTICLE,
        });
      }
    }).catch(e => {
      Cu.reportError("Error fetching content: " + e);
      Messaging.sendRequest({
        type: "Reader:UpdateList",
        id: id,
        status: this.STATUS_FETCH_FAILED_TEMPORARY,
      });
    });
  },

  _downloadAndCacheArticle: Task.async(function* (url) {
    let article = yield ReaderMode.downloadAndParseDocument(url);
    if (article != null) {
      yield ReaderMode.storeArticleInCache(article);
    }
    return article;
  }),

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
        length: 0,
        excerpt: "",
        status: this.STATUS_FETCH_FAILED_UNSUPPORTED_FORMAT,
      };
    } else {
      article.status = this.STATUS_FETCHED_ARTICLE;
    }

    this._addArticleToReadingList(article);
  }),

  _addArticleToReadingList: function(article) {
    Messaging.sendRequestForResult({
      type: "Reader:AddToList",
      url: truncate(article.url, MAX_URI_LENGTH),
      title: truncate(article.title, MAX_TITLE_LENGTH),
      length: article.length,
      excerpt: article.excerpt,
      status: article.status,
    }).then((url) => {
      let mm = window.getGroupMessageManager("browsers");
      mm.broadcastAsyncMessage("Reader:Added", { url: url });
      ReaderMode.storeArticleInCache(article).catch(e => Cu.reportError("Error storing article in cache: " + e));
    }).catch(Cu.reportError);
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
