



"use strict";

this.EXPORTED_SYMBOLS = ["ReaderMode"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;


const DOWNLOAD_SUCCESS = 0;
const DOWNLOAD_ERROR_XHR = 1;
const DOWNLOAD_ERROR_NO_DOC = 2;

const PARSE_SUCCESS = 0;
const PARSE_ERROR_TOO_MANY_ELEMENTS = 1;
const PARSE_ERROR_WORKER = 2;
const PARSE_ERROR_NO_ARTICLE = 3;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.importGlobalProperties(["XMLHttpRequest"]);

XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils", "resource://services-common/utils.js");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReaderWorker", "resource://gre/modules/reader/ReaderWorker.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task", "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch", "resource://gre/modules/TelemetryStopwatch.jsm");

XPCOMUtils.defineLazyGetter(this, "Readability", function() {
  let scope = {};
  scope.dump = this.dump;
  Services.scriptloader.loadSubScript("resource://gre/modules/reader/Readability.js", scope);
  return scope["Readability"];
});

this.ReaderMode = {
  
  CACHE_VERSION: 1,

  DEBUG: 0,

  
  
  get maxElemsToParse() {
    delete this.parseNodeLimit;

    Services.prefs.addObserver("reader.parse-node-limit", this, false);
    return this.parseNodeLimit = Services.prefs.getIntPref("reader.parse-node-limit");
  },

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
        } else if (aData === "reader.parse-node-limit") {
          this.parseNodeLimit = Services.prefs.getIntPref(aData);
        }
        break;
    }
  },

  






  getOriginalUrl: function(url) {
    if (!url.startsWith("about:reader?")) {
      return null;
    }

    let searchParams = new URLSearchParams(url.substring("about:reader?".length));
    if (!searchParams.has("url")) {
      return null;
    }
    let encodedURL = searchParams.get("url");
    try {
      return decodeURIComponent(encodedURL);
    } catch (e) {
      Cu.reportError("Error decoding original URL: " + e);
      return encodedURL;
    }
  },

  





  isProbablyReaderable: function(doc) {
    
    if (doc.mozSyntheticDocument || !(doc instanceof doc.defaultView.HTMLDocument)) {
      return false;
    }

    let uri = Services.io.newURI(doc.location.href, null, null);
    if (!this._shouldCheckUri(uri)) {
      return false;
    }

    let utils = this.getUtilsForWin(doc.defaultView);
    
    
    
    
    
    this._needFlushForVisibilityCheck = true;
    return new Readability(uri, doc).isProbablyReaderable(this.isNodeVisible.bind(this, utils));
  },

  isNodeVisible: function(utils, node) {
    let bounds;
    if (this._needFlushForVisibilityCheck) {
      bounds = node.getBoundingClientRect();
      this._needFlushForVisibilityCheck = false;
    } else {
      bounds = utils.getBoundsWithoutFlushing(node);
    }
    return bounds.height > 0 && bounds.width > 0;
  },

  getUtilsForWin: function(win) {
    return win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
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
    TelemetryStopwatch.start("READER_MODE_DOWNLOAD_MS");
    let doc = yield this._downloadDocument(url).catch(e => {
      TelemetryStopwatch.finish("READER_MODE_DOWNLOAD_MS");
      throw e;
    });
    TelemetryStopwatch.finish("READER_MODE_DOWNLOAD_MS");
    return yield this._readerParse(uri, doc);
  }),

  _downloadDocument: function (url) {
    let histogram = Services.telemetry.getHistogramById("READER_MODE_DOWNLOAD_RESULT");
    return new Promise((resolve, reject) => {
      let xhr = new XMLHttpRequest();
      xhr.open("GET", url, true);
      xhr.onerror = evt => reject(evt.error);
      xhr.responseType = "document";
      xhr.onload = evt => {
        if (xhr.status !== 200) {
          reject("Reader mode XHR failed with status: " + xhr.status);
          histogram.add(DOWNLOAD_ERROR_XHR);
          return;
        }

        let doc = xhr.responseXML;
        if (!doc) {
          reject("Reader mode XHR didn't return a document");
          histogram.add(DOWNLOAD_ERROR_NO_DOC);
          return;
        }

        
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
        histogram.add(DOWNLOAD_SUCCESS);
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

  _blockedHosts: [
    "twitter.com",
    "mail.google.com",
    "github.com",
    "reddit.com",
  ],

  _shouldCheckUri: function (uri) {
    if (!(uri.schemeIs("http") || uri.schemeIs("https"))) {
      this.log("Not parsing URI scheme: " + uri.scheme);
      return false;
    }

    try {
      uri.QueryInterface(Ci.nsIURL);
    } catch (ex) {
      
      return false;
    }
    
    let asciiHost = uri.asciiHost;
    if (this._blockedHosts.some(blockedHost => asciiHost.endsWith(blockedHost))) {
      return false;
    }

    if (!uri.filePath || uri.filePath == "/") {
      this.log("Not parsing home page: " + uri.spec);
      return false;
    }

    return true;
  },

  








  _readerParse: Task.async(function* (uri, doc) {
    let histogram = Services.telemetry.getHistogramById("READER_MODE_PARSE_RESULT");
    if (this.parseNodeLimit) {
      let numTags = doc.getElementsByTagName("*").length;
      if (numTags > this.parseNodeLimit) {
        this.log("Aborting parse for " + uri.spec + "; " + numTags + " elements found");
        histogram.add(PARSE_ERROR_TOO_MANY_ELEMENTS);
        return null;
      }
    }

    let uriParam = {
      spec: uri.spec,
      host: uri.host,
      prePath: uri.prePath,
      scheme: uri.scheme,
      pathBase: Services.io.newURI(".", null, uri).spec
    };

    TelemetryStopwatch.start("READER_MODE_SERIALIZE_DOM_MS");
    let serializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"].
                     createInstance(Ci.nsIDOMSerializer);
    let serializedDoc = serializer.serializeToString(doc);
    TelemetryStopwatch.finish("READER_MODE_SERIALIZE_DOM_MS");

    TelemetryStopwatch.start("READER_MODE_WORKER_PARSE_MS");
    let article = null;
    try {
      article = yield ReaderWorker.post("parseDocument", [uriParam, serializedDoc]);
    } catch (e) {
      Cu.reportError("Error in ReaderWorker: " + e);
      histogram.add(PARSE_ERROR_WORKER);
    }
    TelemetryStopwatch.finish("READER_MODE_WORKER_PARSE_MS");

    if (!article) {
      this.log("Worker did not return an article");
      histogram.add(PARSE_ERROR_NO_ARTICLE);
      return null;
    }

    
    article.url = article.uri.spec;
    delete article.uri;

    let flags = Ci.nsIDocumentEncoder.OutputSelectionOnly | Ci.nsIDocumentEncoder.OutputAbsoluteLinks;
    article.title = Cc["@mozilla.org/parserutils;1"].getService(Ci.nsIParserUtils)
                                                    .convertToPlainText(article.title, flags, 0);

    histogram.add(PARSE_SUCCESS);
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
