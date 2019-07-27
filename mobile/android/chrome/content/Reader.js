



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

    return this.isEnabledForParseOnLoad = this.getStateForParseOnLoad();
  },

  pageAction: {
    readerModeCallback: function(tabID) {
      Messaging.sendRequest({
        type: "Reader:Toggle",
        tabID: tabID
      });
    },

    readerModeActiveCallback: function(tabID) {
      Reader._addTabToReadingList(tabID);
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
          this.isEnabledForParseOnLoad = this.getStateForParseOnLoad();
        }
        break;
      }
    }
  },

  _addTabToReadingList: function(tabID) {
    let tab = BrowserApp.getTabForId(tabID);
    if (!tab) {
      Cu.reportError("Can't add tab to reading list because no tab found for ID: " + tabID);
      return;
    }
    let uri = tab.browser.currentURI;

    this.getArticleFromCache(uri).then(article => {
      
      if (article) {
        this.addArticleToReadingList(article);
        return;
      }

      
      this.getArticleForTab(tabID, uri.specIgnoringRef, article => {
        if (article) {
          this.addArticleToReadingList(article);
        } else {
          
          
          this.addArticleToReadingList({
            url: urlWithoutRef,
            title: tab.browser.contentDocument.title,
          });
        }
      });
    }, e => Cu.reportError("Error trying to get article from cache: " + e));
  },

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

  getStateForParseOnLoad: function Reader_getStateForParseOnLoad() {
    let isEnabled = Services.prefs.getBoolPref("reader.parse-on-load.enabled");
    let isForceEnabled = Services.prefs.getBoolPref("reader.parse-on-load.force-enabled");
    
    
    return isForceEnabled || (isEnabled && !BrowserApp.isOnLowMemoryPlatform);
  },

  parseDocumentFromURL: function Reader_parseDocumentFromURL(url, callback) {
    
    
    if (url in this._requests) {
      let request = this._requests[url];
      request.callbacks.push(callback);
      return;
    }

    let request = { url: url, callbacks: [callback] };
    this._requests[url] = request;

    let uri = Services.io.newURI(url, null, null);

    
    this.getArticleFromCache(uri).then(article => {
      if (article) {
        this.log("Page found in cache, return article immediately");
        this._runCallbacksAndFinish(request, article);
        return;
      }

      if (!this._requests) {
        this.log("Reader has been destroyed, abort");
        return;
      }

      
      
      this._downloadAndParseDocument(url, request);
    }, e => {
      Cu.reportError("Error trying to get article from cache: " + e);
      this._runCallbacksAndFinish(request, null);
    });
  },

  getArticleForTab: function Reader_getArticleForTab(tabId, url, callback) {
    let tab = BrowserApp.getTabForId(tabId);
    if (tab) {
      let article = tab.savedArticle;
      if (article && article.url == url) {
        this.log("Saved article found in tab");
        callback(article);
        return;
      }
    }

    this.parseDocumentFromURL(url, callback);
  },

  parseDocumentFromTab: function (tab, callback) {
    let uri = tab.browser.currentURI;
    if (!this._shouldCheckUri(uri)) {
      callback(null);
      return;
    }

    
    this.getArticleFromCache(uri).then(article => {
      if (article) {
        this.log("Page found in cache, return article immediately");
        callback(article);
        return;
      }

      let doc = tab.browser.contentWindow.document;
      this._readerParse(uri, doc, article => {
        if (!article) {
          this.log("Failed to parse page");
          callback(null);
          return;
        }
        callback(article);
      });
    }, e => {
      Cu.reportError("Error trying to get article from cache: " + e);
      callback(null);
    });
  },

  







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

  uninit: function Reader_uninit() {
    Services.prefs.removeObserver("reader.parse-on-load.", this);

    Services.obs.removeObserver(this, "Reader:Removed");

    let requests = this._requests;
    for (let url in requests) {
      let request = requests[url];
      if (request.browser) {
        let browser = request.browser;
        browser.parentNode.removeChild(browser);
      }
    }
    delete this._requests;
  },

  log: function(msg) {
    if (this.DEBUG)
      dump("Reader: " + msg);
  },

  _shouldCheckUri: function Reader_shouldCheckUri(uri) {
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

  _readerParse: function Reader_readerParse(uri, doc, callback) {
    let numTags = doc.getElementsByTagName("*").length;
    if (numTags > this.MAX_ELEMS_TO_PARSE) {
      this.log("Aborting parse for " + uri.spec + "; " + numTags + " elements found");
      callback(null);
      return;
    }

    let worker = new ChromeWorker("readerWorker.js");
    worker.onmessage = function (evt) {
      let article = evt.data;

      
      
      if (article) {
        article.url = uri.specIgnoringRef;
        let flags = Ci.nsIDocumentEncoder.OutputSelectionOnly | Ci.nsIDocumentEncoder.OutputAbsoluteLinks;
        article.title = Cc["@mozilla.org/parserutils;1"].getService(Ci.nsIParserUtils)
                                                        .convertToPlainText(article.title, flags, 0);
      }

      callback(article);
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
      Cu.reportError("Reader: could not build Readability arguments: " + e);
      callback(null);
    }
  },

  _runCallbacksAndFinish: function Reader_runCallbacksAndFinish(request, result) {
    delete this._requests[request.url];

    request.callbacks.forEach(function(callback) {
      callback(result);
    });
  },

  _downloadDocument: function Reader_downloadDocument(url, callback) {
    
    
    

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

      
      if (doc != browser.contentDocument)
        return;

      this.log("Done loading: " + doc);
      if (doc.location.href == "about:blank") {
        callback(null);

        
        browser.parentNode.removeChild(browser);
        return;
      }

      callback(doc);
    });

    browser.loadURIWithFlags(url, Ci.nsIWebNavigation.LOAD_FLAGS_NONE,
                             null, null, null);

    return browser;
  },

  _downloadAndParseDocument: function Reader_downloadAndParseDocument(url, request) {
    try {
      this.log("Needs to fetch page, creating request: " + url);

      request.browser = this._downloadDocument(url, doc => {
        this.log("Finished loading page: " + doc);

        if (!doc) {
          this.log("Error loading page");
          this._runCallbacksAndFinish(request, null);
          return;
        }

        this.log("Parsing response with Readability");

        let uri = Services.io.newURI(url, null, null);
        this._readerParse(uri, doc, article => {
          
          let browser = request.browser;
          if (browser) {
            browser.parentNode.removeChild(browser);
            delete request.browser;
          }

          if (!article) {
            this.log("Failed to parse page");
            this._runCallbacksAndFinish(request, null);
            return;
          }

          this.log("Parsing has been successful");
          this._runCallbacksAndFinish(request, article);
        });
      });
    } catch (e) {
      this.log("Error downloading and parsing document: " + e);
      this._runCallbacksAndFinish(request, null);
    }
  },

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
