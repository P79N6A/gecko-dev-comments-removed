



"use strict";

this.EXPORTED_SYMBOLS = ["ReaderMode"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.importGlobalProperties(["XMLHttpRequest"]);

XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils", "resource://services-common/utils.js");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReaderWorker", "resource://gre/modules/reader/ReaderWorker.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task", "resource://gre/modules/Task.jsm");

this.ReaderMode = {
  
  CACHE_VERSION: 1,

  DEBUG: 0,

  
  
  MAX_ELEMS_TO_PARSE: 3000,

  get isEnabledForParseOnLoad() {
    delete this.isEnabledForParseOnLoad;

    
    Services.prefs.addObserver("reader.parse-on-load.", this, false);

    return this.isEnabledForParseOnLoad = this._getStateForParseOnLoad();
  },

  get isOnLowMemoryPlatform() {
    let memory = Cc["@mozilla.org/xpcom/memory-service;1"].getService(Ci.nsIMemory);
    delete this.isOnLowMemoryPlatform;
    return this.isOnLowMemoryPlatform = memory.isLowMemoryPlatform();
  },

  _getStateForParseOnLoad: function () {
    let isEnabled = Services.prefs.getBoolPref("reader.parse-on-load.enabled");
    let isForceEnabled = Services.prefs.getBoolPref("reader.parse-on-load.force-enabled");
    
    
    return isForceEnabled || (isEnabled && !this.isOnLowMemoryPlatform);
  },

  observe: function(aMessage, aTopic, aData) {
    switch(aTopic) {
      case "nsPref:changed":
        if (aData.startsWith("reader.parse-on-load.")) {
          this.isEnabledForParseOnLoad = this._getStateForParseOnLoad();
        }
        break;
    }
  },

  





  isProbablyReaderable: function(doc) {
    let uri = Services.io.newURI(doc.location.href, null, null);

    if (!this._shouldCheckUri(uri)) {
      return false;
    }

    let REGEXPS = {
      unlikelyCandidates: /combx|comment|community|disqus|extra|foot|header|menu|remark|rss|shoutbox|sidebar|sponsor|ad-break|agegate|pagination|pager|popup|tweet|twitter/i,
      okMaybeItsACandidate: /and|article|body|column|main|shadow/i,
    };

    let nodes = doc.getElementsByTagName("p");
    if (nodes.length < 5) {
      return false;
    }

    let possibleParagraphs = 0;
    for (let i = 0; i < nodes.length; i++) {
      let node = nodes[i];
      let matchString = node.className + " " + node.id;

      if (REGEXPS.unlikelyCandidates.test(matchString) &&
          !REGEXPS.okMaybeItsACandidate.test(matchString)) {
        continue;
      }

      if (node.textContent.trim().length < 100) {
        continue;
      }

      possibleParagraphs++;
      if (possibleParagraphs >= 5) {
        return true;
      }
    }
    return false;
  },

  







  parseDocument: Task.async(function* (doc) {
    let uri = Services.io.newURI(doc.documentURI, null, null);
    if (!this._shouldCheckUri(uri)) {
      this.log("Reader mode disabled for URI");
      return null;
    }

    return yield this._readerParse(uri, doc);
  }),

  






  downloadAndParseDocument: Task.async(function* (url) {
    let uri = Services.io.newURI(url, null, null);
    let doc = yield this._downloadDocument(url);
    return yield this._readerParse(uri, doc);
  }),

  _downloadDocument: function (url) {
    return new Promise((resolve, reject) => {
      let xhr = new XMLHttpRequest();
      xhr.open("GET", url, true);
      xhr.onerror = evt => reject(evt.error);
      xhr.responseType = "document";
      xhr.onload = evt => {
        if (xhr.status !== 200) {
          reject("Reader mode XHR failed with status: " + xhr.status);
          return;
        }

        let doc = xhr.responseXML;

        
        let meta = doc.querySelector("meta[http-equiv=refresh]");
        if (meta) {
          let content = meta.getAttribute("content");
          if (content) {
            let urlIndex = content.indexOf("URL=");
            if (urlIndex > -1) {
              let url = content.substring(urlIndex + 4);
              this._downloadDocument(url).then((doc) => resolve(doc));
              return;
            }
          }
        }
        resolve(doc);
      }
      xhr.send();
    });
  },


  







  getArticleFromCache: Task.async(function* (url) {
    let path = this._toHashedPath(url);
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

  







  removeArticleFromCache: Task.async(function* (url) {
    let path = this._toHashedPath(url);
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

  








  _readerParse: Task.async(function* (uri, doc) {
    let numTags = doc.getElementsByTagName("*").length;
    if (numTags > this.MAX_ELEMS_TO_PARSE) {
      this.log("Aborting parse for " + uri.spec + "; " + numTags + " elements found");
      return null;
    }

    let uriParam = {
      spec: uri.spec,
      host: uri.host,
      prePath: uri.prePath,
      scheme: uri.scheme,
      pathBase: Services.io.newURI(".", null, uri).spec
    };

    let serializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"].
                     createInstance(Ci.nsIDOMSerializer);
    let serializedDoc = yield Promise.resolve(serializer.serializeToString(doc));

    let article = null;
    try {
      article = yield ReaderWorker.post("parseDocument", [uriParam, serializedDoc]);
    } catch (e) {
      Cu.reportError("Error in ReaderWorker: " + e);
    }

    if (!article) {
      this.log("Worker did not return an article");
      return null;
    }

    
    article.url = article.uri.spec;
    delete article.uri;

    let flags = Ci.nsIDocumentEncoder.OutputSelectionOnly | Ci.nsIDocumentEncoder.OutputAbsoluteLinks;
    article.title = Cc["@mozilla.org/parserutils;1"].getService(Ci.nsIParserUtils)
                                                    .convertToPlainText(article.title, flags, 0);
    return article;
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
