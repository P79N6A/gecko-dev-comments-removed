






































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
    "WHERE b.fk = h.id ",
    (aForTag ? "" : "ORDER BY b.lastModified DESC LIMIT 1"),
  ") AS ", aName].join("");
}





const kBookTagSQLFragment =
  book_tag_sql_fragment("parent", "b.parent", false) + ", " +
  book_tag_sql_fragment("bookmark", "b.title", false) + ", " +
  book_tag_sql_fragment("tags", "GROUP_CONCAT(t.title, ',')", true);


const kTopicShutdown = "places-shutdown";
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
const kQueryIndexOpenPageCount = 10;



const kQueryTypeKeyword = 0;
const kQueryTypeFiltered = 1;




const kTitleTagsSeparator = " \u2013 ";

const kBrowserUrlbarBranch = "browser.urlbar.";











function initTempTable(aDatabase)
{
  
  aDatabase.executeSimpleSQL("PRAGMA temp_store = MEMORY");

  
  
  let stmt = aDatabase.createAsyncStatement(
    "CREATE TEMP TABLE moz_openpages_temp ( "
  + "  url TEXT PRIMARY KEY "
  + ", open_count INTEGER "
  + ") "
  );
  stmt.executeAsync();
  stmt.finalize();

  
  
  stmt = aDatabase.createAsyncStatement(
    "CREATE TEMPORARY TRIGGER moz_openpages_temp_afterupdate_trigger "
  + "AFTER UPDATE OF open_count ON moz_openpages_temp FOR EACH ROW "
  + "WHEN NEW.open_count = 0 "
  + "BEGIN "
  +   "DELETE FROM moz_openpages_temp "
  +   "WHERE url = NEW.url; "
  + "END "
  );
  stmt.executeAsync();
  stmt.finalize();
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
  
  

  
  
  
  const SQL_BASE = "SELECT h.url, h.title, f.url, " + kBookTagSQLFragment + ", "
                 +        "h.visit_count, h.typed, h.id, :query_type, "
                 +        "t.open_count "
                 + "FROM moz_places h "
                 + "LEFT JOIN moz_favicons f ON f.id = h.favicon_id "
                 + "LEFT JOIN moz_openpages_temp t ON t.url = h.url "
                 + "WHERE h.frecency <> 0 "
                 +   "AND AUTOCOMPLETE_MATCH(:searchString, h.url, "
                 +                          "IFNULL(bookmark, h.title), tags, "
                 +                          "h.visit_count, h.typed, parent, "
                 +                          "t.open_count, "
                 +                          ":matchBehavior, :searchBehavior) "
                 +  "{ADDITIONAL_CONDITIONS} "
                 + "ORDER BY h.frecency DESC, h.id DESC "
                 + "LIMIT :maxResults";

  
  

  XPCOMUtils.defineLazyGetter(this, "_db", function() {
    
    
    
    
    let db = Cc["@mozilla.org/browser/nav-history-service;1"].
             getService(Ci.nsPIPlacesDatabase).
             DBConnection.
             clone(true);

    
    initTempTable(db);

    return db;
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
    return this._db.createAsyncStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_historyQuery", function() {
    let replacementText = "AND h.visit_count > 0";
    return this._db.createAsyncStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_bookmarkQuery", function() {
    let replacementText = "AND bookmark IS NOT NULL";
    return this._db.createAsyncStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_tagsQuery", function() {
    let replacementText = "AND tags IS NOT NULL";
    return this._db.createAsyncStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_openPagesQuery", function() {
    return this._db.createAsyncStatement(
      "SELECT t.url, t.url, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "
    +         ":query_type, t.open_count, NULL "
    + "FROM moz_openpages_temp t "
    + "LEFT JOIN moz_places h ON h.url = t.url "
    + "WHERE h.id IS NULL "
    +   "AND AUTOCOMPLETE_MATCH(:searchString, t.url, t.url, NULL, "
    +                          "NULL, NULL, NULL, t.open_count, "
    +                          ":matchBehavior, :searchBehavior) "
    + "ORDER BY t.ROWID DESC "
    + "LIMIT :maxResults "
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_typedQuery", function() {
    let replacementText = "AND h.typed = 1";
    return this._db.createAsyncStatement(
      SQL_BASE.replace("{ADDITIONAL_CONDITIONS}", replacementText, "g")
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_adaptiveQuery", function() {
    
    
    
    
    return this._db.createAsyncStatement(
      "/* do not warn (bug 487789) */ "
    + "SELECT h.url, h.title, f.url, " + kBookTagSQLFragment + ", "
    +         "h.visit_count, h.typed, h.id, :query_type, t.open_count, rank "
    + "FROM ( "
    +   "SELECT ROUND( "
    +     "MAX(((i.input = :search_string) + "
    +          "(SUBSTR(i.input, 1, LENGTH(:search_string)) = :search_string) "
    +         ") * i.use_count "
    +     ") , 1 " 
    +   ") AS rank, place_id "
    +   "FROM moz_inputhistory i "
    +   "GROUP BY i.place_id "
    +   "HAVING rank > 0 "
    + ") AS i "
    + "JOIN moz_places h ON h.id = i.place_id "
    + "LEFT JOIN moz_favicons f ON f.id = h.favicon_id "
    + "LEFT JOIN moz_openpages_temp t ON t.url = h.url "
    + "WHERE AUTOCOMPLETE_MATCH(NULL, h.url, "
    +                          "IFNULL(bookmark, h.title), tags, "
    +                          "h.visit_count, h.typed, parent, "
    +                          "t.open_count, "
    +                          ":matchBehavior, :searchBehavior) "
    + "ORDER BY rank DESC, h.frecency DESC "
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_keywordQuery", function() {
    return this._db.createAsyncStatement(
      "/* do not warn (bug 487787) */ "
    + "SELECT "
    +  "(SELECT REPLACE(url, '%s', :query_string) FROM moz_places WHERE id = b.fk) "
    +  "AS search_url, h.title, "
    +  "IFNULL(f.url, (SELECT f.url "
    +                 "FROM moz_places "
    +                 "JOIN moz_favicons f ON f.id = favicon_id "
    +                 "WHERE rev_host = (SELECT rev_host FROM moz_places WHERE id = b.fk) "
    +                 "ORDER BY frecency DESC "
    +                 "LIMIT 1) "
    + "), b.parent, b.title, NULL, h.visit_count, h.typed, IFNULL(h.id, b.fk), "
    +  ":query_type, t.open_count "
    +  "FROM moz_keywords k "
    +  "JOIN moz_bookmarks b ON b.keyword_id = k.id "
    +  "LEFT JOIN moz_places h ON h.url = search_url "
    +  "LEFT JOIN moz_favicons f ON f.id = h.favicon_id "
    +  "LEFT JOIN moz_openpages_temp t ON t.url = search_url "
    +  "WHERE LOWER(k.keyword) = LOWER(:keyword) "
    +  "ORDER BY h.frecency DESC "
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_registerOpenPageQuery", function() {
    return this._db.createAsyncStatement(
      "INSERT OR REPLACE INTO moz_openpages_temp (url, open_count) "
    + "VALUES (:page_url, "
    +   "IFNULL("
    +     "("
    +        "SELECT open_count + 1 "
    +        "FROM moz_openpages_temp "
    +        "WHERE url = :page_url "
    +      "), "
    +     "1"
    +   ")"
    + ")"
    );
  });

  XPCOMUtils.defineLazyGetter(this, "_unregisterOpenPageQuery", function() {
    return this._db.createAsyncStatement(
      "UPDATE moz_openpages_temp "
    + "SET open_count = open_count - 1 "
    + "WHERE url = :page_url"
    );
  });

  
  

  
  this._prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefService).
                getBranch(kBrowserUrlbarBranch);
  this._loadPrefs(true);

  
  this._os = Cc["@mozilla.org/observer-service;1"].
              getService(Ci.nsIObserverService);
  this._os.addObserver(this, kTopicShutdown, false);

}

nsPlacesAutoComplete.prototype = {
  
  

  startSearch: function PAC_startSearch(aSearchString, aSearchParam,
                                        aPreviousResult, aListener)
  {
    
    
    if ("_pendingQuery" in this)
      this.stopSearch();

    
    

    
    
    this._originalSearchString = aSearchString.trim();

    this._currentSearchString =
      this._fixupSearchText(this._originalSearchString.toLowerCase());

    var searchParamParts = aSearchParam.split(" ");
    this._enableActions = searchParamParts.indexOf("enable-actions") != -1;

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
      [this._getBoundKeywordQuery(tokens), this._getBoundAdaptiveQuery(), this._getBoundOpenPagesQuery(tokens), query] :
      [this._getBoundAdaptiveQuery(), this._getBoundOpenPagesQuery(tokens), query];

    
    this._executeQueries(queries);

    
    this._searchTokens = tokens;
    this._usedPlaces = {};
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

  
  

  registerOpenPage: function PAC_registerOpenPage(aURI)
  {
    let stmt = this._registerOpenPageQuery;
    stmt.params.page_url = aURI.spec;

    stmt.executeAsync();
  },

  unregisterOpenPage: function PAC_unregisterOpenPage(aURI)
  {
    let stmt = this._unregisterOpenPageQuery;
    stmt.params.page_url = aURI.spec;

    stmt.executeAsync();
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
    Components.utils.reportError("Places AutoComplete: An async statement encountered an " +
                                 "error: " + aError.result + ", '" + aError.message + "'");
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
    if (aTopic == kTopicShutdown) {
      this._os.removeObserver(this, kTopicShutdown);

      
      this._prefs.QueryInterface(Ci.nsIPrefBranch2).removeObserver("", this);
      delete this._prefs;

      
      let stmts = [
        "_defaultQuery",
        "_historyQuery",
        "_bookmarkQuery",
        "_tagsQuery",
        "_openPagesQuery",
        "_typedQuery",
        "_adaptiveQuery",
        "_keywordQuery",
        "_registerOpenPageQuery",
        "_unregisterOpenPageQuery",
      ];
      for (let i = 0; i < stmts.length; i++) {
        
        
        if (Object.getOwnPropertyDescriptor(this, stmts[i]).value !== undefined) {
          this[stmts[i]].finalize();
        }
      }

      if (Object.getOwnPropertyDescriptor(this, "_db").value !== undefined) {
        this._db.asyncClose();
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

    if (uri.indexOf("www.") == 0)
      uri = uri.slice(4);

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
    delete this._usedPlaces;
    delete this._pendingQuery;
    this._secondPass = false;
    this._enableActions = false;
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
    this._restrictOpenPageToken = safeGetter("restrict.openpage", "%");
    this._matchTitleToken = safeGetter("match.title", "#");
    this._matchURLToken = safeGetter("match.url", "@");
    this._defaultBehavior = safeGetter("default.behavior", 0);
    
    this._emptySearchDefaultBehavior =
      this._defaultBehavior |
      safeGetter("default.behavior.emptyRestriction",
                 Ci.mozIPlacesAutoComplete.BEHAVIOR_HISTORY |
                 Ci.mozIPlacesAutoComplete.BEHAVIOR_TYPED);

    
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
        case this._restrictOpenPageToken:
          if (!this._enableActions)
            continue;
          this._setBehavior("openpage");
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

  _getBoundOpenPagesQuery: function PAC_getBoundOpenPagesQuery(aTokens)
  {
    let query = this._openPagesQuery;

    
    let (params = query.params) {
      params.query_type = kQueryTypeFiltered;
      params.matchBehavior = this._matchBehavior;
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
    let escapedEntryURL = aRow.getResultByIndex(kQueryIndexURL);
    let openPageCount = aRow.getResultByIndex(kQueryIndexOpenPageCount) || 0;

    
    
    let [url, action] = this._enableActions && openPageCount > 0 ?
                        ["moz-action:switchtab," + escapedEntryURL, "action "] :
                        [escapedEntryURL, ""];

    if (this._inResults(entryId || url)) {
      return false;
    }

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

    this._addToResults(entryId, url, title, entryFavicon, action + style);
    return true;
  },

  






  _inResults: function PAC_inResults(aPlaceIdOrUrl)
  {
    return aPlaceIdOrUrl in this._usedPlaces;
  },

  















  _addToResults: function PAC_addToResults(aPlaceId, aURISpec, aTitle,
                                           aFaviconSpec, aStyle)
  {
    
    
    
    
    
    
    this._usedPlaces[aPlaceId || aURISpec] = true;

    
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

  
  

  classID: Components.ID("d0272978-beab-4adc-a3d4-04b76acfa4e7"),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIAutoCompleteSearch,
    Ci.nsIAutoCompleteSimpleResultListener,
    Ci.mozIPlacesAutoComplete,
    Ci.mozIStorageStatementCallback,
    Ci.nsIObserver,
  ])
};

let components = [nsPlacesAutoComplete];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
