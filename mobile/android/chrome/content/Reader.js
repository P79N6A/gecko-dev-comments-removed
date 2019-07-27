



"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");

let Reader = {
  
  CACHE_VERSION: 1,

  DEBUG: 0,

  
  
  MAX_ELEMS_TO_PARSE: 3000,

  _requests: {},

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

    if (tab.readerEnabled) {
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
        this.removeArticleFromCache(uri).catch(e => Cu.reportError("Error removing article from cache: " + e));
        break;
      }

      case "nsPref:changed": {
        if (aData.startsWith("reader.parse-on-load.")) {
          this.isEnabledForParseOnLoad = this._getStateForParseOnLoad();
        }
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
      
      
      article = { url: urlWithoutRef, title: tab.browser.contentDocument.title };
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
    });

    this.storeArticleInCache(article).catch(e => Cu.reportError("Error storing article in cache: " + e));
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
        this.log("Saved article found in tab");
        return article;
      }
    }

    
    let uri = Services.io.newURI(url, null, null);
    let article = yield this.getArticleFromCache(uri);
    if (article) {
      this.log("Saved article found in cache");
      return article;
    }

    
    
    return yield this._downloadAndParseDocument(url);
  }),

  







  parseDocumentFromTab: Task.async(function* (tab) {
    let uri = tab.browser.currentURI;
    if (!this._shouldCheckUri(uri)) {
      this.log("Reader mode disabled for URI");
      return null;
    }

    
    let article = yield this.getArticleFromCache(uri);
    if (article) {
      this.log("Page found in cache, return article immediately");
      return article;
    }

    let doc = tab.browser.contentWindow.document;
    return yield this._readerParse(uri, doc);
  }),

  







  getArticleFromCache: Task.async(function* (uri) {
    let path = this._toHashedPath(uri.specIgnoringRef);
    try {
      let array = yield OS.File.read(path);
      return JSON.parse(new TextDecoder().decode(array));
    } catch (e if e instanceof OS.File.Error && e.becauseNoSuchFile) {
      return null;
    }
  }),

  







  storeArticleInCache: Task.async(function* (article) {
    let array = new TextEncoder().encode(JSON.stringify(article));
    let path = this._toHashedPath(article.url);
    yield this._ensureCacheDir();
    yield OS.File.writeAtomic(path, array, { tmpPath: path + ".tmp" });
  }),

  







  removeArticleFromCache: Task.async(function* (uri) {
    let path = this._toHashedPath(uri.specIgnoringRef);
    yield OS.File.remove(path);
  }),

  log: function(msg) {
    if (this.DEBUG)
      dump("Reader: " + msg);
  },

  _shouldCheckUri: function (uri) {
    if ((uri.prePath + "/") === uri.spec) {
      this.log("Not parsing home page: " + uri.spec);
      return false;
    }

    if (!(uri.schemeIs("http") || uri.schemeIs("https") || uri.schemeIs("file"))) {
      this.log("Not parsing URI scheme: " + uri.scheme);
      return false;
    }

    return true;
  },

  _readerParse: function (uri, doc) {
    return new Promise((resolve, reject) => {
      let numTags = doc.getElementsByTagName("*").length;
      if (numTags > this.MAX_ELEMS_TO_PARSE) {
        this.log("Aborting parse for " + uri.spec + "; " + numTags + " elements found");
        resolve(null);
        return;
      }

      let worker = new ChromeWorker("readerWorker.js");
      worker.onmessage = evt => {
        let article = evt.data;

        if (!article) {
          this.log("Worker did not return an article");
          resolve(null);
          return;
        }

        
        
        article.url = uri.specIgnoringRef;
        let flags = Ci.nsIDocumentEncoder.OutputSelectionOnly | Ci.nsIDocumentEncoder.OutputAbsoluteLinks;
        article.title = Cc["@mozilla.org/parserutils;1"].getService(Ci.nsIParserUtils)
                                                        .convertToPlainText(article.title, flags, 0);
        resolve(article);
      };

      worker.onerror = evt => {
        reject("Error in worker: " + evt.message);
      };

      try {
        worker.postMessage({
          uri: {
            spec: uri.spec,
            host: uri.host,
            prePath: uri.prePath,
            scheme: uri.scheme,
            pathBase: Services.io.newURI(".", null, uri).spec
          },
          doc: new XMLSerializer().serializeToString(doc)
        });
      } catch (e) {
        reject("Reader: could not build Readability arguments: " + e);
      }
    });
  },

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

        this.log("Done loading: " + doc);
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
    this.log("Needs to fetch page, creating request: " + url);
    let { browser, doc } = yield this._downloadDocument(url);
    this.log("Finished loading page: " + doc);

    try {
      let uri = Services.io.newURI(url, null, null);
      let article = yield this._readerParse(uri, doc);
      this.log("Document parsed successfully");
      return article;
    } finally {
      browser.parentNode.removeChild(browser);
    }
  }),

  get _cryptoHash() {
    delete this._cryptoHash;
    return this._cryptoHash = Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
  },

  get _unicodeConverter() {
    delete this._unicodeConverter;
    this._unicodeConverter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                              .createInstance(Ci.nsIScriptableUnicodeConverter);
    this._unicodeConverter.charset = "utf8";
    return this._unicodeConverter;
  },

  





  _toHashedPath: function (url) {
    let value = this._unicodeConverter.convertToByteArray(url);
    this._cryptoHash.init(this._cryptoHash.MD5);
    this._cryptoHash.update(value, value.length);

    let hash = CommonUtils.encodeBase32(this._cryptoHash.finish(false));
    let fileName = hash.substring(0, hash.indexOf("=")) + ".json";
    return OS.Path.join(OS.Constants.Path.profileDir, "readercache", fileName);
  },

  






  _ensureCacheDir: function () {
    let dir = OS.Path.join(OS.Constants.Path.profileDir, "readercache");
    return OS.File.exists(dir).then(exists => {
      if (!exists) {
        return OS.File.makeDir(dir);
      }
    });
  }
};
