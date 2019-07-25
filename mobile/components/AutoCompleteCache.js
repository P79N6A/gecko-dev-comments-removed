



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const PERMS_FILE      = 0644;
const PERMS_DIRECTORY = 0755;

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;



const CACHE_VERSION = 1;

const RESULT_CACHE = 1;
const RESULT_NEW = 2;


XPCOMUtils.defineLazyGetter(this, "PACS", function() {
  return Components.classesByID["{d0272978-beab-4adc-a3d4-04b76acfa4e7}"]
                   .getService(Ci.nsIAutoCompleteSearch);
});


function getDir(aKey) {
  return Services.dirsvc.get(aKey, Ci.nsIFile);
}






var AutoCompleteUtils = {
  cacheFile: null,
  cache: null,
  query: "",
  busy: false,
  timer: null,
  DELAY: 10000,

  
  fetch: function fetch(query, onResult) {
    
    this.stop();

    
    this.busy = true;
    PACS.startSearch(query, "", null, {
      onSearchResult: function(search, result) {
        
        if (typeof onResult == "function")
          onResult(result, RESULT_NEW);

        
        if (result.searchResult == result.RESULT_NOMATCH_ONGOING ||
            result.searchResult == result.RESULT_SUCCESS_ONGOING)
          return;

        
        if (AutoCompleteUtils.query == query)
          AutoCompleteUtils.cache = result;
        AutoCompleteUtils.busy = false;

        
        if (AutoCompleteUtils.query == query)
          AutoCompleteUtils.saveCache();
      }
    });
  },

  
  stop: function stop() {
    
    if (!this.busy)
      return;

    
    PACS.stopSearch();
    this.busy = false;
  },

  
  update: function update() {
    
    if (this.timer != null)
      return;

    
    this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this.timer.initWithCallback({
      notify: function() {
        AutoCompleteUtils.timer = null;

        
        if (!AutoCompleteUtils.busy)
          AutoCompleteUtils.fetch(AutoCompleteUtils.query);
      }
    }, this.DELAY, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  init: function init() {
    if (this.cacheFile)
      return;

    this.cacheFile = getDir("ProfD");
    this.cacheFile.append("autocomplete.json");

    if (this.cacheFile.exists()) {
      
      this.loadCache();
    } else {
      
      this.fetch(this.query);
    }
  },

  saveCache: function saveCache() {
    if (!this.cache)
      return;

    let cache = {};
    cache.version = CACHE_VERSION;

    
    let result = this.cache;
    let copy = JSON.parse(JSON.stringify(result));
    copy.data = [];
    for (let i = 0; i < result.matchCount; i++)
      copy.data[i] = [result.getValueAt(i), result.getCommentAt(i), result.getStyleAt(i), result.getImageAt(i)];

    cache.result = copy;

    
    let ostream = Cc["@mozilla.org/network/file-output-stream;1"].createInstance(Ci.nsIFileOutputStream);
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Ci.nsIScriptableUnicodeConverter);

    try {
      ostream.init(this.cacheFile, (MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE), PERMS_FILE, 0);
      converter.charset = "UTF-8";
      let data = converter.convertToInputStream(JSON.stringify(cache));

      
      NetUtil.asyncCopy(data, ostream, function(rv) {
        if (!Components.isSuccessCode(rv))
          Cu.reportError("AutoCompleteUtils: failure during asyncCopy: " + rv);
      });
    } catch (ex) {
      Cu.reportError("AutoCompleteUtils: Could not write to cache file: " + this.cacheFile + " | " + ex);
    }
  },

  loadCache: function loadCache() {
    try {
      
      let stream = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
      let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);

      stream.init(this.cacheFile, MODE_RDONLY, PERMS_FILE, 0);
      let cache = json.decodeFromStream(stream, stream.available());

      if (cache.version != CACHE_VERSION) {
        this.fetch(this.query);
        return;
      }

      let result = cache.result;

      
      result.getValueAt = function(index) this.data[index][0];
      result.getLabelAt = function(index) this.data[index][0];
      result.getCommentAt = function(index) this.data[index][1];
      result.getStyleAt = function(index) this.data[index][2];
      result.getImageAt = function(index) this.data[index][3];

      this.cache = result;
    } catch (ex) {
      Cu.reportError("AutoCompleteUtils: Could not read from cache file: " + ex);
    }
  }
};





function AutoCompleteCache() {
  AutoCompleteUtils.init();

  Services.obs.addObserver(this, "browser:purge-session-history", true);
}

AutoCompleteCache.prototype = {
  classID: Components.ID("{a65f9dca-62ab-4b36-a870-972927c78b56}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAutoCompleteSearch, Ci.nsIObserver, Ci.nsISupportsWeakReference]),

  get _searchThreshold() {
    delete this._searchCount;
    return this._searchCount = Services.prefs.getIntPref("browser.urlbar.autocomplete.search_threshold");    
  },

  startSearch: function(query, param, prev, listener) {
    let self = this;
    let done = function(result, aType) {
      let showSearch = (result.matchCount < self._searchThreshold) && (aType == RESULT_NEW);

      if (showSearch && (result.searchResult == Ci.nsIAutoCompleteResult.RESULT_SUCCESS ||
                         result.searchResult == Ci.nsIAutoCompleteResult.RESULT_NOMATCH)) {
        let engines = Services.search.getVisibleEngines();
        try {
          result.QueryInterface(Ci.nsIAutoCompleteSimpleResult);
          if (engines.length > 0) {  
            for (let i = 0; i < engines.length; i++) {
              let url = engines[i].getSubmission(query).uri.spec;
              result.appendMatch(url, engines[i].name, engines[i].iconURI.spec, "search");
            }
            result.setSearchResult(Ci.nsIAutoCompleteResult.RESULT_SUCCESS);
          }
        } catch(ex) {
        }
      }
      listener.onSearchResult(self, result);
    };

    
    query = query.trim();

    if (AutoCompleteUtils.query == query && AutoCompleteUtils.cache) {
      
      done(AutoCompleteUtils.cache, RESULT_CACHE);
    } else {
      
      AutoCompleteUtils.fetch(query, done);
    }

    
    AutoCompleteUtils.update();
  },

  stopSearch: function() {
    
    AutoCompleteUtils.stop();
  },

  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "browser:purge-session-history":
        AutoCompleteUtils.update();
        break;
    }
  }
};




function BookmarkObserver() {
  AutoCompleteUtils.init();
  this._batch = false;
}

BookmarkObserver.prototype = {
  onBeginUpdateBatch: function() {
    this._batch = true;
  },
  onEndUpdateBatch: function() {
    this._batch = false;
    AutoCompleteUtils.update();
  },
  onItemAdded: function(aItemId, aParentId, aIndex, aItemType) {
    if (!this._batch)
      AutoCompleteUtils.update();
  },
  onItemChanged: function () {
    if (!this._batch)
      AutoCompleteUtils.update();
  },
  onBeforeItemRemoved: function() {},
  onItemRemoved: function() {
    if (!this._batch)
      AutoCompleteUtils.update();
  },
  onItemVisited: function() {},
  onItemMoved: function() {},

  classID: Components.ID("f570982e-4f15-48ab-b6a0-ed851ac551b2"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavBookmarkObserver])
};

const components = [AutoCompleteCache, BookmarkObserver];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
