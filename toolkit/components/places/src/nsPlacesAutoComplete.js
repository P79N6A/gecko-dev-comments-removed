






































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;


function book_tag_sql_fragment(aName, aColumn, aForTag)
{
  return ["(",
    "SELECT ", aColumn, " ",
    "FROM moz_bookmarks b ",
    "JOIN moz_bookmarks t ",
      "ON t.id = b.parent ",
      "AND t.parent ", (aForTag ? "" : "!"), "= :parent ",
    "WHERE b.type = ", Ci.nsINavBookmarksService.TYPE_BOOKMARK, " ",
      "AND b.fk = h.id ",
    (aForTag ? "AND LENGTH(t.title) > 0" :
               "ORDER BY b.lastModified DESC LIMIT 1"),
  ") AS ", aName].join("");
}





const kBookTagSQLFragment =
  book_tag_sql_fragment("parent", "b.parent", false) + ", " +
  book_tag_sql_fragment("bookmark", "b.title", false) + ", " +
  book_tag_sql_fragment("tags", "GROUP_CONCAT(t.title, ',')", true);


const kXPComShutdown = "xpcom-shutdown";
const kPrefChanged = "nsPref:changed";



const MATCH_ANYWHERE = Ci.mozIPlacesAutoComplete.MATCH_ANYWHERE;
const MATCH_BOUNDARY_ANYWHERE = Ci.mozIPlacesAutoComplete.MATCH_BOUNDARY_ANYWHERE;
const MATCH_BOUNDARY = Ci.mozIPlacesAutoComplete.MATCH_BOUNDARY;
const MATCH_BEGINNING = Ci.mozIPlacesAutoComplete.MATCH_BEGINNING;



const kQueryIndexURL = 0;
const kQueryIndexTitle = 1;
const kQueryIndexFaviconURL = 2;
const kQueryIndexParentId = 3;
const kQueryIndexBookmarkTitle = 4;
const kQueryIndexTags = 5;
const kQueryIndexVisitCount = 6;
const kQueryIndexTyped = 7;
const kQueryIndexPlaceId = 8;
const kQueryIndexQueryType = 9;



const kQueryTypeKeyword = 0;
const kQueryTypeFiltered = 1;




const kTitleTagsSeparator = " \u2013 ";

const kBrowserUrlbarBranch = "browser.urlbar.";













function best_favicon_for_revhost(aTableName)
{
  return "(" +
    "SELECT f.url " +
    "FROM " + aTableName + " " +
    "JOIN moz_favicons f ON f.id = favicon_id " +
    "WHERE rev_host = IFNULL( " +
      "(SELECT rev_host FROM moz_places_temp WHERE id = b.fk), " +
      "(SELECT rev_host FROM moz_places WHERE id = b.fk) " +
    ") " +
    "ORDER BY frecency DESC " +
    "LIMIT 1 " +
  ")";
}













function AutoCompleteStatementCallbackWrapper(aCallback,
                                              aDBConnection)
{
  this._callback = aCallback;
  this._db = aDBConnection;
}

AutoCompleteStatementCallbackWrapper.prototype = {
  
  

  handleResult: function ACSCW_handleResult(aResultSet)
  {
    this._callback.handleResult.apply(this._callback, arguments);
  },

  handleError: function ACSCW_handleError(aError)
  {
    this._callback.handleError.apply(this._callback, arguments);
  },

  handleCompletion: function ACSCW_handleCompletion(aReason)
  {
    
    
    let callback = this._callback;
    if (!callback.isSearchComplete() && callback.isPendingSearch(this._handle))
      callback.handleCompletion.apply(callback, arguments);
  },

  
  

  








  executeAsync: function ACSCW_executeAsync(aQueries)
  {
    return this._handle = this._db.executeAsync(aQueries, aQueries.length,
                                                this);
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([
    Ci.mozIStorageStatementCallback,
  ])
};




function nsPlacesAutoComplete()
{
  
  

  
  
  
  
  
  function sql_base_fragment(aTableName) {
    return "SELECT h.url, h.title, f.url, " + kBookTagSQLFragment + ", " +
                  "h.visit_count, h.typed, h.id, :query_type, h.frecency " +
           "FROM " + aTableName + " h " +
           "LEFT OUTER JOIN moz_favicons f ON f.id = h.favicon_id " +
           "WHERE h.frecency <> 0 " +
           "AND AUTOCOMPLETE_MATCH(:searchString, h.url, " +
                                  "IFNULL(bookmark, h.title), tags, " +
                                  "h.visit_count, h.typed, parent, " +
                                  ":matchBehavior, :searchBehavior) " +
          "{ADDITIONAL_CONDITIONS} ";
  }
  const SQL_BASE = sql_base_fragment("moz_places_temp") +
                   "UNION ALL " +
                   sql_base_fragment("moz_places") +
                   "AND +h.id NOT IN (SELECT id FROM moz_places_temp) " +
                   "ORDER BY h.frecency DESC, h.id DESC " +
                   "LIMIT :maxResults";

  
  

  XPCOMUtils.defineLazyGetter(this, "_db", function() {
    return Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsPIPlacesDatabase).
           DBConnection;
  });

  XPCOMUtils.defineLazyServiceGetter(this, "_bh",
                                     "@mozilla.org/browser/global-history;2",
                                     "nsIBrowserHistory");

  XPCOMUtils.defineLazyServiceGetter(this, "_textURIService",
                                     "@mozilla.org/intl/texttosuburi;1",
                                     "nsITextToSubURI");

  XPCOMUtils.defineLazyServiceGetter(this, "_bs",
                                     "@mozilla.org/browser/nav-bookmarks-service;1",
                                     "nsINavBookmarksService");

  XPCOMUtils.defineLazyServiceGetter(this, "_ioService",
                                     "@mozilla.org/network/io-service;1",
                                     "nsIIOService");

  XPCOMUtils.defineLazyServiceGetter(this, "_faviconService",
                                     "@mozilla.org/browser/favicon-service;1",
                                     "nsIFaviconService");

  XPCOMUtils.defineLazyGetter(this, "_defaultQuery", function() {
    let replacementText = "";
    return this._db.createStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_historyQuery", function() {
    let replacementText = "AND h.visit_count > 0";
    return this._db.createStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_bookmarkQuery", function() {
    let replacementText = "AND bookmark IS NOT NULL";
    return this._db.createStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_tagsQuery", function() {
    let replacementText = "AND tags IS NOT NULL";
    return this._db.createStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_typedQuery", function() {
    let replacementText = "AND h.typed = 1";
    return this._db.createStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_adaptiveQuery", function() {
    
    
    
    
    
    return this._db.createStatement(
      "/* do not warn (bug 487789) */ " +
      "SELECT IFNULL(h_t.url, h.url), IFNULL(h_t.title, h.title), f.url, " +
              kBookTagSQLFragment + ", IFNULL(h_t.visit_count, h.visit_count), " +
              "IFNULL(h_t.typed, h.typed), IFNULL(h_t.id, h.id), " +
              ":query_type, rank " +
      "FROM ( " +
        "SELECT ROUND(MAX(((i.input = :search_string) + " +
                          "(SUBSTR(i.input, 1, LENGTH(:search_string)) = :search_string)) * " +
                          "i.use_count), 1) AS rank, place_id " +
        "FROM moz_inputhistory i " +
        "GROUP BY i.place_id " +
        "HAVING rank > 0 " +
      ") AS i " +
      "LEFT JOIN moz_places h ON h.id = i.place_id " +
      "LEFT JOIN moz_places_temp h_t ON h_t.id = i.place_id " +
      "LEFT JOIN moz_favicons f ON f.id = IFNULL(h_t.favicon_id, h.favicon_id) "+
      "WHERE IFNULL(h_t.url, h.url) NOTNULL " +
      "AND AUTOCOMPLETE_MATCH(:searchString, 0 /* url */, " +
                             "IFNULL(bookmark, 1 /* title */), tags, " +
                             "6 /* visit_count */, 7 /* typed */, parent, " +
                             ":matchBehavior, :searchBehavior) " +
      "ORDER BY rank DESC, IFNULL(h_t.frecency, h.frecency) DESC"
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_keywordQuery", function() {
    return this._db.createStatement(
      "/* do not warn (bug 487787) */ " +
      "SELECT IFNULL( " +
          "(SELECT REPLACE(url, '%s', :query_string) FROM moz_places_temp WHERE id = b.fk), " +
          "(SELECT REPLACE(url, '%s', :query_string) FROM moz_places WHERE id = b.fk) " +
        ") AS search_url, IFNULL(h_t.title, h.title), " +
        "COALESCE(f.url, " + best_favicon_for_revhost("moz_places_temp") + "," +
                  best_favicon_for_revhost("moz_places") + "), b.parent, " +
        "b.title, NULL, IFNULL(h_t.visit_count, h.visit_count), " +
        "IFNULL(h_t.typed, h.typed), COALESCE(h_t.id, h.id, b.fk), " +
        ":query_type " +
      "FROM moz_keywords k " +
      "JOIN moz_bookmarks b ON b.keyword_id = k.id " +
      "LEFT JOIN moz_places AS h ON h.url = search_url " +
      "LEFT JOIN moz_places_temp AS h_t ON h_t.url = search_url " +
      "LEFT JOIN moz_favicons f ON f.id = IFNULL(h_t.favicon_id, h.favicon_id) " +
      "WHERE LOWER(k.keyword) = LOWER(:keyword) " +
      "ORDER BY IFNULL(h_t.frecency, h.frecency) DESC"
    );
  });

  
  

  
  this._prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefService).
                getBranch(kBrowserUrlbarBranch);
  this._loadPrefs(true);

  
  this._os = Cc["@mozilla.org/observer-service;1"].
              getService(Ci.nsIObserverService);
  this._os.addObserver(this, kXPComShutdown, false);

}

nsPlacesAutoComplete.prototype = {
  
  

  startSearch: function PAC_startSearch(aSearchString, aSearchParam,
                                        aPreviousResult, aListener)
  {
    
    

    
    
    this._originalSearchString = aSearchString.trim();

    this._currentSearchString =
      this._fixupSearchText(this._originalSearchString.toLowerCase());

    this._listener = aListener;
    let result = Cc["@mozilla.org/autocomplete/simple-result;1"].
                 createInstance(Ci.nsIAutoCompleteSimpleResult);
    result.setSearchString(aSearchString);
    result.setListener(this);
    this._result = result;

    
    if (!this._enabled) {
      this._finishSearch(true);
      return;
    }

    
    if (this._currentSearchString)
      this._behavior = this._defaultBehavior;
    else
      this._behavior = this._emptySearchDefaultBehavior;

    
    
    
    
    
    
    
    let {query, tokens} =
      this._getSearch(this._getUnfilteredSearchTokens(this._currentSearchString));
    let queries = tokens.length ?
      [this._getBoundKeywordQuery(tokens), this._getBoundAdaptiveQuery(), query] :
      [this._getBoundAdaptiveQuery(), query];

    
    this._executeQueries(queries);

    
    this._searchTokens = tokens;
    this._usedPlaceIds = {};
  },

  stopSearch: function PAC_stopSearch()
  {
    
    
    
    if (this._pendingQuery)
      this._stopActiveQuery();

    this._finishSearch(false);
  },

  
  

  onValueRemoved: function PAC_onValueRemoved(aResult, aURISpec, aRemoveFromDB)
  {
    if (aRemoveFromDB)
      this._bh.removePage(this._ioService.newURI(aURISpec, null, null));
  },

  
  

  handleResult: function PAC_handleResult(aResultSet)
  {
    let row, haveMatches = false;
    while (row = aResultSet.getNextRow()) {
      let match = this._processRow(row);
      haveMatches = haveMatches || match;

      if (this._result.matchCount == this._maxRichResults) {
        
        this._stopActiveQuery();

        
        this._finishSearch(true);
        return;
      }

    }

    
    if (haveMatches)
      this._notifyResults(true);
  },

  handleError: function PAC_handleError(aError)
  {
    Components.utils.reportError("Places AutoComplete: " + aError);
  },

  handleCompletion: function PAC_handleCompletion(aReason)
  {
    
    if (this.isSearchComplete())
      return;

    
    
    
    if (this._matchBehavior == MATCH_BOUNDARY_ANYWHERE &&
        this._result.matchCount < this._maxRichResults && !this._secondPass) {
      this._secondPass = true;
      let queries = [
        this._getBoundAdaptiveQuery(MATCH_ANYWHERE),
        this._getBoundSearchQuery(MATCH_ANYWHERE, this._searchTokens),
      ];
      this._executeQueries(queries);
      return;
    }

    this._finishSearch(true);
  },

  
  

  observe: function PAC_observe(aSubject, aTopic, aData)
  {
    if (aTopic == kXPComShutdown) {
      this._os.removeObserver(this, kXPComShutdown);

      
      this._prefs.removeObserver("", this);
      delete this._prefs;

      
      let stmts = [
        "_defaultQuery",
        "_historyQuery",
        "_bookmarkQuery",
        "_tagsQuery",
        "_typedQuery",
        "_adaptiveQuery",
        "_keywordQuery",
      ];
      for (let i = 0; i < stmts.length; i++) {
        
        
        
        if (!this.__lookupGetter__(stmts[i]))
          this[stmts[i]].finalize();
      }
    }
    else if (aTopic == kPrefChanged) {
      this._loadPrefs();
    }
  },

  
  

  







  _fixupSearchText: function PAC_fixupSearchText(aURIString)
  {
    let uri = aURIString;

    if (uri.indexOf("http://") == 0)
      uri = uri.slice(7);
    else if (uri.indexOf("https://") == 0)
      uri = uri.slice(8);
    else if (uri.indexOf("ftp://") == 0)
      uri = uri.slice(6);

    return this._textURIService.unEscapeURIForUI("UTF-8", uri);
  },

  






  _getUnfilteredSearchTokens: function PAC_unfilteredSearchTokens(aSearchString)
  {
    
    
    
    return aSearchString.length ? aSearchString.split(" ") : [];
  },

  






  _finishSearch: function PAC_finishSearch(aNotify)
  {
    
    if (aNotify)
      this._notifyResults(false);

    
    delete this._originalSearchString;
    delete this._currentSearchString;
    delete this._searchTokens;
    delete this._listener;
    delete this._result;
    delete this._usedPlaceIds;
    delete this._pendingQuery;
    this._secondPass = false;
  },

  





  _executeQueries: function PAC_executeQueries(aQueries)
  {
    
    
    

    
    let wrapper = new AutoCompleteStatementCallbackWrapper(this, this._db);
    this._pendingQuery = wrapper.executeAsync(aQueries);
  },

  


  _stopActiveQuery: function PAC_stopActiveQuery()
  {
    this._pendingQuery.cancel();
    delete this._pendingQuery;
  },

  





  _notifyResults: function PAC_notifyResults(aSearchOngoing)
  {
    let result = this._result;
    let resultCode = result.matchCount ? "RESULT_SUCCESS" : "RESULT_NOMATCH";
    if (aSearchOngoing)
      resultCode += "_ONGOING";
    result.setSearchResult(Ci.nsIAutoCompleteResult[resultCode]);
    result.setDefaultIndex(result.matchCount ? 0 : -1);
    this._listener.onSearchResult(this, result);
  },

  






  _loadPrefs: function PAC_loadPrefs(aRegisterObserver)
  {
    let self = this;
    function safeGetter(aName, aDefault) {
      let types = {
        boolean: "Bool",
        number: "Int",
        string: "Char"
      };
      let type = types[typeof(aDefault)];
      if (!type)
        throw "Unknown type!";

      
      try {
        return self._prefs["get" + type + "Pref"](aName);
      }
      catch (e) {
        return aDefault;
      }
    }

    this._enabled = safeGetter("autocomplete.enabled", true);
    this._matchBehavior = safeGetter("matchBehavior", MATCH_BOUNDARY_ANYWHERE);
    this._filterJavaScript = safeGetter("filter.javascript", true);
    this._maxRichResults = safeGetter("maxRichResults", 25);
    this._restrictHistoryToken = safeGetter("restrict.history", "^");
    this._restrictBookmarkToken = safeGetter("restrict.bookmark", "*");
    this._restrictTypedToken = safeGetter("restrict.typed", "~");
    this._restrictTagToken = safeGetter("restrict.tag", "+");
    this._matchTitleToken = safeGetter("match.title", "#");
    this._matchURLToken = safeGetter("match.url", "@");
    this._defaultBehavior = safeGetter("default.behavior", 0);
    
    
    this._emptySearchDefaultBehavior = this._defaultBehavior |
                                       safeGetter("default.behavior.emptyRestriction", 33);

    
    if (this._matchBehavior != MATCH_ANYWHERE &&
        this._matchBehavior != MATCH_BOUNDARY &&
        this._matchBehavior != MATCH_BEGINNING)
      this._matchBehavior = MATCH_BOUNDARY_ANYWHERE;

    
    if (aRegisterObserver) {
      let pb = this._prefs.QueryInterface(Ci.nsIPrefBranch2);
      pb.addObserver("", this, false);
    }
  },

  










  _getSearch: function PAC_getSearch(aTokens)
  {
    
    
    for (let i = aTokens.length - 1; i >= 0; i--) {
      switch (aTokens[i]) {
        case this._restrictHistoryToken:
          this._setBehavior("history");
          break;
        case this._restrictBookmarkToken:
          this._setBehavior("bookmark");
          break;
        case this._restrictTagToken:
          this._setBehavior("tag");
          break;
        case this._matchTitleToken:
          this._setBehavior("title");
          break;
        case this._matchURLToken:
          this._setBehavior("url");
          break;
        case this._restrictTypedToken:
          this._setBehavior("typed");
          break;
        default:
          
          continue;
      };

      aTokens.splice(i, 1);
    }

    
    
    
    if (!this._filterJavaScript)
      this._setBehavior("javascript");

    return {
      query: this._getBoundSearchQuery(this._matchBehavior, aTokens),
      tokens: aTokens
    };
  },

  













  _getBoundSearchQuery: function PAC_getBoundSearchQuery(aMatchBehavior,
                                                         aTokens)
  {
    
    
    
    let query = this._hasBehavior("tag") ? this._tagsQuery :
                this._hasBehavior("bookmark") ? this._bookmarkQuery :
                this._hasBehavior("typed") ? this._typedQuery :
                this._hasBehavior("history") ? this._historyQuery :
                this._defaultQuery;

    
    let (params = query.params) {
      params.parent = this._bs.tagsFolder;
      params.query_type = kQueryTypeFiltered;
      params.matchBehavior = aMatchBehavior;
      params.searchBehavior = this._behavior;

      
      
      params.searchString = aTokens.join(" ");

      
      
      params.maxResults = this._maxRichResults;
    }

    return query;
  },

  






  _getBoundKeywordQuery: function PAC_getBoundKeywordQuery(aTokens)
  {
    
    
    let searchString = this._originalSearchString;
    let queryString = "";
    let queryIndex = searchString.indexOf(" ");
    if (queryIndex != -1)
      queryString = searchString.substring(queryIndex + 1);

    
    queryString = encodeURIComponent(queryString).replace("%20", "+", "g");

    
    let keyword = aTokens[0];

    let query = this._keywordQuery;
    let (params = query.params) {
      params.keyword = keyword;
      params.query_string = queryString;
      params.query_type = kQueryTypeKeyword;
    }

    return query;
  },

  




  _getBoundAdaptiveQuery: function PAC_getBoundAdaptiveQuery(aMatchBehavior)
  {
    
    if (arguments.length == 0)
      aMatchBehavior = this._matchBehavior;

    let query = this._adaptiveQuery;
    let (params = query.params) {
      params.parent = this._bs.tagsFolder;
      params.search_string = this._currentSearchString;
      params.query_type = kQueryTypeFiltered;
      params.matchBehavior = aMatchBehavior;
      params.searchBehavior = this._behavior;
    }

    return query;
  },

  








  _processRow: function PAC_processRow(aRow)
  {
    
    let entryId = aRow.getResultByIndex(kQueryIndexPlaceId);
    if (this._inResults(entryId))
      return false;

    let escapedEntryURL = aRow.getResultByIndex(kQueryIndexURL);
    let entryTitle = aRow.getResultByIndex(kQueryIndexTitle) || "";
    let entryFavicon = aRow.getResultByIndex(kQueryIndexFaviconURL) || "";
    let entryParentId = aRow.getResultByIndex(kQueryIndexParentId);
    let entryBookmarkTitle = entryParentId ?
      aRow.getResultByIndex(kQueryIndexBookmarkTitle) : null;
    let entryTags = aRow.getResultByIndex(kQueryIndexTags) || "";

    
    let title = entryBookmarkTitle || entryTitle;

    let style;
    if (aRow.getResultByIndex(kQueryIndexQueryType) == kQueryTypeKeyword) {
      
      
      
      
      if (!entryTitle)
        style = "keyword";
      else
        title = entryTitle;
    }

    
    let showTags = !!entryTags;

    
    
    if (this._hasBehavior("history") &&
        !(this._hasBehavior("bookmark") || this._hasBehavior("tag"))) {
      showTags = false;
      style = "favicon";
    }

    
    if (showTags)
      title += kTitleTagsSeparator + entryTags;

    
    
    
    if (!style) {
      
      
      
      if (showTags)
        style = "tag";
      else if (entryParentId)
        style = "bookmark";
      else
        style = "favicon";
    }

    
    this._addToResults(entryId, escapedEntryURL, title, entryFavicon, style);
    return true;
  },

  






  _inResults: function PAC_inResults(aPlaceId)
  {
    return (aPlaceId in this._usedPlaceIds);
  },

  















  _addToResults: function PAC_addToResults(aPlaceId, aURISpec, aTitle,
                                           aFaviconSpec, aStyle)
  {
    
    
    this._usedPlaceIds[aPlaceId] = true;

    
    let favicon;
    if (aFaviconSpec) {
      let uri = this._ioService.newURI(aFaviconSpec, null, null);
      favicon = this._faviconService.getFaviconLinkForIcon(uri).spec;
    }
    favicon = favicon || this._faviconService.defaultFavicon.spec;

    this._result.appendMatch(aURISpec, aTitle, favicon, aStyle);
  },

  






  _hasBehavior: function PAC_hasBehavior(aType)
  {
    return (this._behavior &
            Ci.mozIPlacesAutoComplete["BEHAVIOR_" + aType.toUpperCase()]);
  },

  





  _setBehavior: function PAC_setBehavior(aType)
  {
    this._behavior |=
      Ci.mozIPlacesAutoComplete["BEHAVIOR_" + aType.toUpperCase()];
  },

  




  isSearchComplete: function PAC_isSearchComplete()
  {
    
    
    return this._pendingQuery == null;
  },

  








  isPendingSearch: function PAC_isPendingSearch(aHandle)
  {
    return this._pendingQuery == aHandle;
  },

  
  

  classDescription: "AutoComplete result generator for Places.",
  classID: Components.ID("d0272978-beab-4adc-a3d4-04b76acfa4e7"),
  contractID: "@mozilla.org/autocomplete/search;1?name=history",

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIAutoCompleteSearch,
    Ci.nsIAutoCompleteSimpleResultListener,
    Ci.mozIStorageStatementCallback,
    Ci.nsIObserver,
  ])
};




let components = [nsPlacesAutoComplete];
function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule(components);
}
