



"use strict";

let Reader = {

  
  STATUS_UNFETCHED: 0,
  STATUS_FETCH_FAILED_TEMPORARY: 1,
  STATUS_FETCH_FAILED_PERMANENT: 2,
  STATUS_FETCH_FAILED_UNSUPPORTED_FORMAT: 3,
  STATUS_FETCHED_ARTICLE: 4,

  get isEnabledForParseOnLoad() {
    delete this.isEnabledForParseOnLoad;

    
    Services.prefs.addObserver("reader.parse-on-load.", this, false);

    return this.isEnabledForParseOnLoad = this._getStateForParseOnLoad();
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

    if (tab.savedArticle) {
      this.pageAction.id = PageActions.add({
        title: Strings.browser.GetStringFromName("readerMode.enter"),
        icon: "drawable://reader",
        clickCallback: () => this.pageAction.readerModeCallback(tab.id),
        longClickCallback: () => this.pageAction.readerModeActiveCallback(tab.id),
        important: true
      });
    }
  },

  observe: function(aMessage, aTopic, aData) {
    switch(aTopic) {
      case "Reader:Removed": {
        let uri = Services.io.newURI(aData, null, null);
        ReaderMode.removeArticleFromCache(uri).catch(e => Cu.reportError("Error removing article from cache: " + e));
        break;
      }
    }
  },

  _addTabToReadingList: Task.async(function* (tabID) {
    let tab = BrowserApp.getTabForId(tabID);
    if (!tab) {
      throw new Error("Can't add tab to reading list because no tab found for ID: " + tabID);
    }

    let uri = tab.browser.currentURI;
    let urlWithoutRef = uri.specIgnoringRef;

    let article = yield this.getArticle(urlWithoutRef, tabID).catch(e => {
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

  _getStateForParseOnLoad: function () {
    let isEnabled = Services.prefs.getBoolPref("reader.parse-on-load.enabled");
    let isForceEnabled = Services.prefs.getBoolPref("reader.parse-on-load.force-enabled");
    
    
    return isForceEnabled || (isEnabled && !BrowserApp.isOnLowMemoryPlatform);
  },

  








  getArticle: Task.async(function* (url, tabId) {
    
    let tab = BrowserApp.getTabForId(tabId);
    if (tab) {
      let article = tab.savedArticle;
      if (article && article.url == url) {
        return article;
      }
    }

    
    let uri = Services.io.newURI(url, null, null);
    let article = yield ReaderMode.getArticleFromCache(uri);
    if (article) {
      return article;
    }

    
    
    return yield this._downloadAndParseDocument(url);
  }),

  _downloadDocument: function (url) {
    return new Promise((resolve, reject) => {
      
      
      
      let browser = document.createElement("browser");
      browser.setAttribute("type", "content");
      browser.setAttribute("collapsed", "true");
      browser.setAttribute("disablehistory", "true");

      document.documentElement.appendChild(browser);
      browser.stop();

      browser.webNavigation.allowAuth = false;
      browser.webNavigation.allowImages = false;
      browser.webNavigation.allowJavascript = false;
      browser.webNavigation.allowMetaRedirects = true;
      browser.webNavigation.allowPlugins = false;

      browser.addEventListener("DOMContentLoaded", event => {
        let doc = event.originalTarget;

        
        if (doc != browser.contentDocument) {
          return;
        }

        if (doc.location.href == "about:blank") {
          reject("about:blank loaded; aborting");

          
          browser.parentNode.removeChild(browser);
          return;
        }

        resolve({ browser, doc });
      });

      browser.loadURIWithFlags(url, Ci.nsIWebNavigation.LOAD_FLAGS_NONE,
                               null, null, null);
    });
  },

  _downloadAndParseDocument: Task.async(function* (url) {
    let { browser, doc } = yield this._downloadDocument(url);

    try {
      let uri = Services.io.newURI(url, null, null);
      let article = yield ReaderMode.readerParse(uri, doc);
      return article;
    } finally {
      browser.parentNode.removeChild(browser);
    }
  }),

  


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
      yield this.storeArticleInCache(article);
    }

    
    window.indexedDB.deleteDatabase("about:reader");
  }),
};
