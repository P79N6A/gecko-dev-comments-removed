





"use strict";




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const TOPIC_SHUTDOWN = "places-shutdown";
const TOPIC_PREFCHANGED = "nsPref:changed";

const DEFAULT_BEHAVIOR = 0;

const PREF_BRANCH = "browser.urlbar.";


const PREF_ENABLED =                [ "autocomplete.enabled",   true ];
const PREF_AUTOFILL =               [ "autoFill",               true ];
const PREF_AUTOFILL_TYPED =         [ "autoFill.typed",         true ];
const PREF_AUTOFILL_SEARCHENGINES = [ "autoFill.searchEngines", true ];
const PREF_DELAY =                  [ "delay",                  50 ];
const PREF_BEHAVIOR =               [ "matchBehavior", MATCH_BOUNDARY_ANYWHERE ];
const PREF_DEFAULT_BEHAVIOR =       [ "default.behavior", DEFAULT_BEHAVIOR ];
const PREF_EMPTY_BEHAVIOR =         [ "default.behavior.emptyRestriction",
                                      Ci.mozIPlacesAutoComplete.BEHAVIOR_HISTORY |
                                      Ci.mozIPlacesAutoComplete.BEHAVIOR_TYPED ];
const PREF_FILTER_JS =              [ "filter.javascript",      true ];
const PREF_MAXRESULTS =             [ "maxRichResults",         25 ];
const PREF_RESTRICT_HISTORY =       [ "restrict.history",       "^" ];
const PREF_RESTRICT_BOOKMARKS =     [ "restrict.bookmark",      "*" ];
const PREF_RESTRICT_TYPED =         [ "restrict.typed",         "~" ];
const PREF_RESTRICT_TAG =           [ "restrict.tag",           "+" ];
const PREF_RESTRICT_SWITCHTAB =     [ "restrict.openpage",      "%" ];
const PREF_MATCH_TITLE =            [ "match.title",            "#" ];
const PREF_MATCH_URL =              [ "match.url",              "@" ];



const MATCH_ANYWHERE = Ci.mozIPlacesAutoComplete.MATCH_ANYWHERE;
const MATCH_BOUNDARY_ANYWHERE = Ci.mozIPlacesAutoComplete.MATCH_BOUNDARY_ANYWHERE;
const MATCH_BOUNDARY = Ci.mozIPlacesAutoComplete.MATCH_BOUNDARY;
const MATCH_BEGINNING = Ci.mozIPlacesAutoComplete.MATCH_BEGINNING;
const MATCH_BEGINNING_CASE_SENSITIVE = Ci.mozIPlacesAutoComplete.MATCH_BEGINNING_CASE_SENSITIVE;



const QUERYTYPE_KEYWORD       = 0;
const QUERYTYPE_FILTERED      = 1;
const QUERYTYPE_AUTOFILL_HOST = 2;
const QUERYTYPE_AUTOFILL_URL  = 3;




const TITLE_TAGS_SEPARATOR = " \u2013 ";


const TITLE_SEARCH_ENGINE_SEPARATOR = " \u00B7\u2013\u00B7 ";


const TELEMETRY_1ST_RESULT = "PLACES_AUTOCOMPLETE_1ST_RESULT_TIME_MS";
const TELEMETRY_6_FIRST_RESULTS = "PLACES_AUTOCOMPLETE_6_FIRST_RESULTS_TIME_MS";

const FRECENCY_SEARCHENGINES_DEFAULT = 1000;


const QUERYINDEX_QUERYTYPE     = 0;
const QUERYINDEX_URL           = 1;
const QUERYINDEX_TITLE         = 2;
const QUERYINDEX_ICONURL       = 3;
const QUERYINDEX_BOOKMARKED    = 4;
const QUERYINDEX_BOOKMARKTITLE = 5;
const QUERYINDEX_TAGS          = 6;
const QUERYINDEX_VISITCOUNT    = 7;
const QUERYINDEX_TYPED         = 8;
const QUERYINDEX_PLACEID       = 9;
const QUERYINDEX_SWITCHTAB     = 10;
const QUERYINDEX_FRECENCY      = 11;





const SQL_BOOKMARK_TAGS_FRAGMENT =
  `EXISTS(SELECT 1 FROM moz_bookmarks WHERE fk = h.id) AS bookmarked,
   ( SELECT title FROM moz_bookmarks WHERE fk = h.id AND title NOTNULL
     ORDER BY lastModified DESC LIMIT 1
   ) AS btitle,
   ( SELECT GROUP_CONCAT(t.title, ', ')
     FROM moz_bookmarks b
     JOIN moz_bookmarks t ON t.id = +b.parent AND t.parent = :parent
     WHERE b.fk = h.id
   ) AS tags`;



function defaultQuery(conditions = "") {
  let query =
    `SELECT :query_type, h.url, h.title, f.url, ${SQL_BOOKMARK_TAGS_FRAGMENT},
            h.visit_count, h.typed, h.id, t.open_count, h.frecency
     FROM moz_places h
     LEFT JOIN moz_favicons f ON f.id = h.favicon_id
     LEFT JOIN moz_openpages_temp t ON t.url = h.url
     WHERE h.frecency <> 0
       AND AUTOCOMPLETE_MATCH(:searchString, h.url,
                              IFNULL(btitle, h.title), tags,
                              h.visit_count, h.typed,
                              bookmarked, t.open_count,
                              :matchBehavior, :searchBehavior)
       ${conditions}
     ORDER BY h.frecency DESC, h.id DESC
     LIMIT :maxResults`;
  return query;
}

const SQL_DEFAULT_QUERY = defaultQuery();




const SQL_HISTORY_QUERY = defaultQuery("AND +h.visit_count > 0");

const SQL_BOOKMARK_QUERY = defaultQuery("AND bookmarked");

const SQL_TAGS_QUERY = defaultQuery("AND tags NOTNULL");

const SQL_TYPED_QUERY = defaultQuery("AND h.typed = 1");

const SQL_SWITCHTAB_QUERY =
  `SELECT :query_type, t.url, t.url, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
          t.open_count, NULL
   FROM moz_openpages_temp t
   LEFT JOIN moz_places h ON h.url = t.url
   WHERE h.id IS NULL
     AND AUTOCOMPLETE_MATCH(:searchString, t.url, t.url, NULL,
                            NULL, NULL, NULL, t.open_count,
                            :matchBehavior, :searchBehavior)
   ORDER BY t.ROWID DESC
   LIMIT :maxResults`;

const SQL_ADAPTIVE_QUERY =
  `/* do not warn (bug 487789) */
   SELECT :query_type, h.url, h.title, f.url, ${SQL_BOOKMARK_TAGS_FRAGMENT},
          h.visit_count, h.typed, h.id, t.open_count, h.frecency
   FROM (
     SELECT ROUND(MAX(use_count) * (1 + (input = :search_string)), 1) AS rank,
            place_id
     FROM moz_inputhistory
     WHERE input BETWEEN :search_string AND :search_string || X'FFFF'
     GROUP BY place_id
   ) AS i
   JOIN moz_places h ON h.id = i.place_id
   LEFT JOIN moz_favicons f ON f.id = h.favicon_id
   LEFT JOIN moz_openpages_temp t ON t.url = h.url
   WHERE AUTOCOMPLETE_MATCH(NULL, h.url,
                            IFNULL(btitle, h.title), tags,
                            h.visit_count, h.typed, bookmarked,
                            t.open_count,
                            :matchBehavior, :searchBehavior)
   ORDER BY rank DESC, h.frecency DESC`;

const SQL_KEYWORD_QUERY =
  `/* do not warn (bug 487787) */
   SELECT :query_type,
     (SELECT REPLACE(url, '%s', :query_string) FROM moz_places WHERE id = b.fk)
     AS search_url, h.title,
     IFNULL(f.url, (SELECT f.url
                    FROM moz_places
                    JOIN moz_favicons f ON f.id = favicon_id
                    WHERE rev_host = (SELECT rev_host FROM moz_places WHERE id = b.fk)
                    ORDER BY frecency DESC
                    LIMIT 1)
           ),
     1, b.title, NULL, h.visit_count, h.typed, IFNULL(h.id, b.fk),
     t.open_count, h.frecency
   FROM moz_keywords k
   JOIN moz_bookmarks b ON b.keyword_id = k.id
   LEFT JOIN moz_places h ON h.url = search_url
   LEFT JOIN moz_favicons f ON f.id = h.favicon_id
   LEFT JOIN moz_openpages_temp t ON t.url = search_url
   WHERE LOWER(k.keyword) = LOWER(:keyword)
   ORDER BY h.frecency DESC`;

function hostQuery(conditions = "") {
  let query =
    `/* do not warn (bug NA): not worth to index on (typed, frecency) */
     SELECT :query_type, host || '/', IFNULL(prefix, '') || host || '/',
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, frecency
     FROM moz_hosts
     WHERE host BETWEEN :searchString AND :searchString || X'FFFF'
     AND frecency <> 0
     ${conditions}
     ORDER BY frecency DESC
     LIMIT 1`;
  return query;
}

const SQL_HOST_QUERY = hostQuery();

const SQL_TYPED_HOST_QUERY = hostQuery("AND typed = 1");

function bookmarkedHostQuery(conditions = "") {
  let query =
    `/* do not warn (bug NA): not worth to index on (typed, frecency) */
     SELECT :query_type, host || '/', IFNULL(prefix, '') || host || '/',
            NULL, (
              SELECT foreign_count > 0 FROM moz_places
              WHERE rev_host = get_unreversed_host(host || '.') || '.'
                 OR rev_host = get_unreversed_host(host || '.') || '.www.'
            ) AS bookmarked, NULL, NULL, NULL, NULL, NULL, NULL, frecency
     FROM moz_hosts
     WHERE host BETWEEN :searchString AND :searchString || X'FFFF'
     AND bookmarked
     AND frecency <> 0
     ${conditions}
     ORDER BY frecency DESC
     LIMIT 1`;
  return query;
}

const SQL_BOOKMARKED_HOST_QUERY = bookmarkedHostQuery();

const SQL_BOOKMARKED_TYPED_HOST_QUERY = bookmarkedHostQuery("AND typed = 1");

function urlQuery(conditions = "") {
  let query =
    `/* do not warn (bug no): cannot use an index */
     SELECT :query_type, h.url, NULL,
            NULL, foreign_count > 0 AS bookmarked, NULL, NULL, NULL, NULL, NULL, NULL, h.frecency
     FROM moz_places h
     WHERE h.frecency <> 0
     ${conditions}
     AND AUTOCOMPLETE_MATCH(:searchString, h.url,
     h.title, '',
     h.visit_count, h.typed, 0, 0,
     :matchBehavior, :searchBehavior)
     ORDER BY h.frecency DESC, h.id DESC
     LIMIT 1`;
  return query;
}

const SQL_URL_QUERY = urlQuery();

const SQL_TYPED_URL_QUERY = urlQuery("AND typed = 1");


const SQL_BOOKMARKED_URL_QUERY = urlQuery("AND bookmarked");

const SQL_BOOKMARKED_TYPED_URL_QUERY = urlQuery("AND bookmarked AND typed = 1");




Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch",
                                  "resource://gre/modules/TelemetryStopwatch.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Preferences",
                                  "resource://gre/modules/Preferences.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Sqlite",
                                  "resource://gre/modules/Sqlite.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesSearchAutocompleteProvider",
                                  "resource://gre/modules/PlacesSearchAutocompleteProvider.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "textURIService",
                                   "@mozilla.org/intl/texttosuburi;1",
                                   "nsITextToSubURI");












XPCOMUtils.defineLazyGetter(this, "SwitchToTabStorage", () => Object.seal({
  _conn: null,
  
  _queue: new Set(),
  initDatabase: Task.async(function* (conn) {
    
    
    
    yield conn.execute(
      `CREATE TEMP TABLE moz_openpages_temp (
         url TEXT PRIMARY KEY,
         open_count INTEGER
       )`);

    
    
    yield conn.execute(
      `CREATE TEMPORARY TRIGGER moz_openpages_temp_afterupdate_trigger
       AFTER UPDATE OF open_count ON moz_openpages_temp FOR EACH ROW
       WHEN NEW.open_count = 0
       BEGIN
         DELETE FROM moz_openpages_temp
         WHERE url = NEW.url;
       END`);

    this._conn = conn;

    
    this._queue.forEach(this.add, this);
    
    this._queue.clear();
  }),

  add: function (uri) {
    if (!this._conn) {
      this._queue.add(uri);
      return;
    }
    this._conn.executeCached(
      `INSERT OR REPLACE INTO moz_openpages_temp (url, open_count)
         VALUES ( :url, IFNULL( (SELECT open_count + 1
                                  FROM moz_openpages_temp
                                  WHERE url = :url),
                                  1
                              )
                )`
    , { url: uri.spec });
  },

  delete: function (uri) {
    if (!this._conn) {
      this._queue.delete(uri);
      return;
    }
    this._conn.executeCached(
      `UPDATE moz_openpages_temp
       SET open_count = open_count - 1
       WHERE url = :url`
    , { url: uri.spec });
  },

  shutdown: function () {
    this._conn = null;
    this._queue.clear();
  }
}));




XPCOMUtils.defineLazyGetter(this, "Prefs", () => {
  let prefs = new Preferences(PREF_BRANCH);

  function loadPrefs() {
    store.enabled = prefs.get(...PREF_ENABLED);
    store.autofill = prefs.get(...PREF_AUTOFILL);
    store.autofillTyped = prefs.get(...PREF_AUTOFILL_TYPED);
    store.autofillSearchEngines = prefs.get(...PREF_AUTOFILL_SEARCHENGINES);
    store.delay = prefs.get(...PREF_DELAY);
    store.matchBehavior = prefs.get(...PREF_BEHAVIOR);
    store.filterJavaScript = prefs.get(...PREF_FILTER_JS);
    store.maxRichResults = prefs.get(...PREF_MAXRESULTS);
    store.restrictHistoryToken = prefs.get(...PREF_RESTRICT_HISTORY);
    store.restrictBookmarkToken = prefs.get(...PREF_RESTRICT_BOOKMARKS);
    store.restrictTypedToken = prefs.get(...PREF_RESTRICT_TYPED);
    store.restrictTagToken = prefs.get(...PREF_RESTRICT_TAG);
    store.restrictOpenPageToken = prefs.get(...PREF_RESTRICT_SWITCHTAB);
    store.matchTitleToken = prefs.get(...PREF_MATCH_TITLE);
    store.matchURLToken = prefs.get(...PREF_MATCH_URL);
    store.defaultBehavior = prefs.get(...PREF_DEFAULT_BEHAVIOR);
    
    store.emptySearchDefaultBehavior = store.defaultBehavior |
                                       prefs.get(...PREF_EMPTY_BEHAVIOR);

    
    if (store.matchBehavior != MATCH_ANYWHERE &&
        store.matchBehavior != MATCH_BOUNDARY &&
        store.matchBehavior != MATCH_BEGINNING) {
      store.matchBehavior = MATCH_BOUNDARY_ANYWHERE;
    }

    store.tokenToBehaviorMap = new Map([
      [ store.restrictHistoryToken, "history" ],
      [ store.restrictBookmarkToken, "bookmark" ],
      [ store.restrictTagToken, "tag" ],
      [ store.restrictOpenPageToken, "openpage" ],
      [ store.matchTitleToken, "title" ],
      [ store.matchURLToken, "url" ],
      [ store.restrictTypedToken, "typed" ]
    ]);
  }

  let store = {
    observe: function (subject, topic, data) {
      loadPrefs();
    },
    QueryInterface: XPCOMUtils.generateQI([ Ci.nsIObserver ])
  };
  loadPrefs();
  prefs.observe("", store);

  return Object.seal(store);
});












function fixupSearchText(spec)
  textURIService.unEscapeURIForUI("UTF-8", stripPrefix(spec));











function getUnfilteredSearchTokens(searchString)
  searchString.length ? searchString.split(" ") : [];








function stripPrefix(spec)
{
  ["http://", "https://", "ftp://"].some(scheme => {
    
    if (spec.startsWith(scheme) && spec[scheme.length] != " ") {
      spec = spec.slice(scheme.length);
      return true;
    }
    return false;
  });

  
  if (spec.startsWith("www.") && spec[4] != " ") {
    spec = spec.slice(4);
  }
  return spec;
}








function stripHttpAndTrim(spec) {
  if (spec.startsWith("http://")) {
    spec = spec.slice(7);
  }
  if (spec.endsWith("?")) {
    spec = spec.slice(0, -1);
  }
  if (spec.endsWith("/")) {
    spec = spec.slice(0, -1);
  }
  return spec;
}





function Search(searchString, searchParam, autocompleteListener,
                resultListener, autocompleteSearch) {
  
  this._originalSearchString = searchString;
  this._trimmedOriginalSearchString = searchString.trim();
  this._searchString = fixupSearchText(this._trimmedOriginalSearchString.toLowerCase());

  this._matchBehavior = Prefs.matchBehavior;
  
  this._behavior = this._searchString ? Prefs.defaultBehavior
                                      : Prefs.emptySearchDefaultBehavior;
  this._enableActions = searchParam.split(" ").indexOf("enable-actions") != -1;

  this._searchTokens =
    this.filterTokens(getUnfilteredSearchTokens(this._searchString));
  
  
  this._strippedPrefix = this._trimmedOriginalSearchString.slice(
    0, this._trimmedOriginalSearchString.length - this._searchString.length
  ).toLowerCase();
  
  
  let pathIndex =
    this._trimmedOriginalSearchString.indexOf("/", this._strippedPrefix.length);
  this._autofillUrlSearchString = fixupSearchText(
    this._trimmedOriginalSearchString.slice(0, pathIndex).toLowerCase() +
    this._trimmedOriginalSearchString.slice(pathIndex)
  );

  this._listener = autocompleteListener;
  this._autocompleteSearch = autocompleteSearch;

  
  
  let result = Cc["@mozilla.org/autocomplete/simple-result;1"]
                 .createInstance(Ci.nsIAutoCompleteSimpleResult);
  result.setSearchString(searchString);
  result.setListener(resultListener);
  
  result.setDefaultIndex(-1);
  this._result = result;

  
  this._usedURLs = new Set();
  this._usedPlaceIds = new Set();
}

Search.prototype = {
  





  setBehavior: function (type) {
    this._behavior |=
      Ci.mozIPlacesAutoComplete["BEHAVIOR_" + type.toUpperCase()];
  },

  






  hasBehavior: function (type) {
    return this._behavior &
           Ci.mozIPlacesAutoComplete["BEHAVIOR_" + type.toUpperCase()];
  },

  



  _sleepDeferred: null,
  _sleep: function (aTimeMs) {
    
    
    if (!this._sleepTimer)
      this._sleepTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._sleepDeferred = Promise.defer();
    this._sleepTimer.initWithCallback(() => this._sleepDeferred.resolve(),
                                      aTimeMs, Ci.nsITimer.TYPE_ONE_SHOT);
    return this._sleepDeferred.promise;
  },

  







  filterTokens: function (tokens) {
    
    for (let i = tokens.length - 1; i >= 0; i--) {
      let behavior = Prefs.tokenToBehaviorMap.get(tokens[i]);
      
      
      if (behavior && (behavior != "openpage" || this._enableActions)) {
        this.setBehavior(behavior);
        tokens.splice(i, 1);
      }
    }

    
    
    
    if (!Prefs.filterJavaScript) {
      this.setBehavior("javascript");
    }

    return tokens;
  },

  


  cancel: function () {
    if (this._sleepTimer)
      this._sleepTimer.cancel();
    if (this._sleepDeferred) {
      this._sleepDeferred.resolve();
      this._sleepDeferred = null;
    }
    this.pending = false;
  },

  


  pending: true,

  




  execute: Task.async(function* (conn) {
    
    if (!this.pending)
      return;

    TelemetryStopwatch.start(TELEMETRY_1ST_RESULT);
    if (this._searchString)
      TelemetryStopwatch.start(TELEMETRY_6_FIRST_RESULTS);

    
    
    yield PlacesSearchAutocompleteProvider.ensureInitialized();

    
    
    
    
    
    
    
    
    
    

    
    let queries = [ this._adaptiveQuery,
                    this._switchToTabQuery,
                    this._searchQuery ];

    let hasKeyword = false;
    if (this._searchTokens.length > 0 &&
        PlacesUtils.bookmarks.getURIForKeyword(this._searchTokens[0])) {
      queries.unshift(this._keywordQuery);
      hasKeyword = true;
    }

    if (this._shouldAutofill) {
      if (this._searchTokens.length == 1 && !hasKeyword)
        yield this._matchSearchEngineUrl();

      
      let lastSlashIndex = this._searchString.lastIndexOf("/");
      
      if (lastSlashIndex != -1) {
        
        if (lastSlashIndex < this._searchString.length - 1) {
          queries.unshift(this._urlQuery);
        }
      } else if (this.pending) {
        
        
        let [ query, params ] = this._hostQuery;
        yield conn.executeCached(query, params, this._onResultRow.bind(this));
      }
    }

    yield this._sleep(Prefs.delay);
    if (!this.pending)
      return;

    for (let [query, params] of queries) {
      yield conn.executeCached(query, params, this._onResultRow.bind(this));
      if (!this.pending)
        return;
    }

    
    
    
    if (this._matchBehavior == MATCH_BOUNDARY_ANYWHERE &&
        this._result.matchCount < Prefs.maxRichResults) {
      this._matchBehavior = MATCH_ANYWHERE;
      for (let [query, params] of [ this._adaptiveQuery,
                                    this._searchQuery ]) {
        yield conn.executeCached(query, params, this._onResultRow.bind(this));
        if (!this.pending)
          return;
      }
    }

    
    
    if (this._frecencyMatches) {
      this._frecencyMatches.forEach(this._addMatch, this);
    }
  }),

  _matchSearchEngineUrl: function* () {
    if (!Prefs.autofillSearchEngines)
      return;

    let match = yield PlacesSearchAutocompleteProvider.findMatchByToken(
                                                           this._searchString);
    if (match) {
      this._result.setDefaultIndex(0);
      this._addFrecencyMatch({
        value: match.token,
        comment: match.engineName,
        icon: match.iconUrl,
        style: "priority-search",
        finalCompleteValue: match.url,
        frecency: FRECENCY_SEARCHENGINES_DEFAULT
      });
    }
  },

  _onResultRow: function (row) {
    TelemetryStopwatch.finish(TELEMETRY_1ST_RESULT);
    let queryType = row.getResultByIndex(QUERYINDEX_QUERYTYPE);
    let match;
    switch (queryType) {
      case QUERYTYPE_AUTOFILL_HOST:
        this._result.setDefaultIndex(0);
        match = this._processHostRow(row);
        break;
      case QUERYTYPE_AUTOFILL_URL:
        this._result.setDefaultIndex(0);
        match = this._processUrlRow(row);
        break;
      case QUERYTYPE_FILTERED:
      case QUERYTYPE_KEYWORD:
        match = this._processRow(row);
        break;
    }
    this._addMatch(match);
  },

  


  _addFrecencyMatch: function (match) {
    if (!this._frecencyMatches)
      this._frecencyMatches = [];
    this._frecencyMatches.push(match);
    
    
    
    this._frecencyMatches.sort((a, b) => a.frecency - b.frecency);
  },

  _maybeRestyleSearchMatch: function (match) {
    
    let parseResult =
      PlacesSearchAutocompleteProvider.parseSubmissionURL(match.value);
    if (!parseResult) {
      return;
    }

    
    
    
    
    let terms = parseResult.terms.toLowerCase();
    if (this._searchTokens.length > 0 &&
        this._searchTokens.every(token => terms.indexOf(token) == -1)) {
      return;
    }

    
    match.style = "search " + match.style;
    match.comment = parseResult.terms + TITLE_SEARCH_ENGINE_SEPARATOR +
                    parseResult.engineName;
  },

  _addMatch: function (match) {
    
    
    if (!this.pending)
      return;

    let notifyResults = false;

    if (this._frecencyMatches) {
      for (let i = this._frecencyMatches.length - 1;  i >= 0 ; i--) {
        if (this._frecencyMatches[i].frecency > match.frecency) {
          this._addMatch(this._frecencyMatches.splice(i, 1)[0]);
        }
      }
    }

    
    let urlMapKey = stripHttpAndTrim(match.value);
    if ((!match.placeId || !this._usedPlaceIds.has(match.placeId)) &&
        !this._usedURLs.has(urlMapKey)) {
      
      
      
      
      
      
      if (match.placeId)
        this._usedPlaceIds.add(match.placeId);
      this._usedURLs.add(urlMapKey);

      if (!match.style) {
        match.style = "favicon";
      }

      
      if (match.style == "favicon") {
        this._maybeRestyleSearchMatch(match);
      }

      this._result.appendMatch(match.value,
                               match.comment,
                               match.icon || PlacesUtils.favicons.defaultFavicon.spec,
                               match.style,
                               match.finalCompleteValue);
      notifyResults = true;
    }

    if (this._result.matchCount == 6)
      TelemetryStopwatch.finish(TELEMETRY_6_FIRST_RESULTS);

    if (this._result.matchCount == Prefs.maxRichResults || !this.pending) {
      
      this.cancel();
      
      
      throw StopIteration;
    }

    if (notifyResults) {
      
      this.notifyResults(true);
    }
  },

  _processHostRow: function (row) {
    let match = {};
    let trimmedHost = row.getResultByIndex(QUERYINDEX_URL);
    let untrimmedHost = row.getResultByIndex(QUERYINDEX_TITLE);
    let frecency = row.getResultByIndex(QUERYINDEX_FRECENCY);
    
    
    if (untrimmedHost &&
        !untrimmedHost.toLowerCase().contains(this._trimmedOriginalSearchString.toLowerCase())) {
      untrimmedHost = null;
    }

    match.value = this._strippedPrefix + trimmedHost;
    
    match.comment = stripHttpAndTrim(trimmedHost);
    match.finalCompleteValue = untrimmedHost;
    match.frecency = frecency;
    return match;
  },

  _processUrlRow: function (row) {
    let match = {};
    let value = row.getResultByIndex(QUERYINDEX_URL);
    let url = fixupSearchText(value);
    let frecency = row.getResultByIndex(QUERYINDEX_FRECENCY);

    let prefix = value.slice(0, value.length - stripPrefix(value).length);

    
    let separatorIndex = url.slice(this._searchString.length)
                            .search(/[\/\?\#]/);
    if (separatorIndex != -1) {
      separatorIndex += this._searchString.length;
      if (url[separatorIndex] == "/") {
        separatorIndex++; 
      }
      url = url.slice(0, separatorIndex);
    }

    
    
    let untrimmedURL = prefix + url;
    if (untrimmedURL &&
        !untrimmedURL.toLowerCase().contains(this._trimmedOriginalSearchString.toLowerCase())) {
      untrimmedURL = null;
     }

    match.value = this._strippedPrefix + url;
    match.comment = url;
    match.finalCompleteValue = untrimmedURL;
    match.frecency = frecency;
    return match;
  },

  _processRow: function (row) {
    let match = {};
    match.placeId = row.getResultByIndex(QUERYINDEX_PLACEID);
    let queryType = row.getResultByIndex(QUERYINDEX_QUERYTYPE);
    let escapedURL = row.getResultByIndex(QUERYINDEX_URL);
    let openPageCount = row.getResultByIndex(QUERYINDEX_SWITCHTAB) || 0;
    let historyTitle = row.getResultByIndex(QUERYINDEX_TITLE) || "";
    let iconurl = row.getResultByIndex(QUERYINDEX_ICONURL) || "";
    let bookmarked = row.getResultByIndex(QUERYINDEX_BOOKMARKED);
    let bookmarkTitle = bookmarked ?
      row.getResultByIndex(QUERYINDEX_BOOKMARKTITLE) : null;
    let tags = row.getResultByIndex(QUERYINDEX_TAGS) || "";
    let frecency = row.getResultByIndex(QUERYINDEX_FRECENCY);

    
    
    let [url, action] = this._enableActions && openPageCount > 0 ?
                        ["moz-action:switchtab," + escapedURL, "action "] :
                        [escapedURL, ""];

    
    let title = bookmarkTitle || historyTitle;

    if (queryType == QUERYTYPE_KEYWORD) {
      
      
      
      
      if (!historyTitle) {
        match.style = "keyword";
      }
      else {
        title = historyTitle;
      }
    }

    
    let showTags = !!tags;

    
    
    if (this.hasBehavior("history") &&
        !(this.hasBehavior("bookmark") || this.hasBehavior("tag"))) {
      showTags = false;
      match.style = "favicon";
    }

    
    if (showTags) {
      title += TITLE_TAGS_SEPARATOR + tags;
    }

    
    
    
    if (!match.style) {
      
      
      
      if (showTags) {
        match.style = "tag";
      }
      else if (bookmarked) {
        match.style = "bookmark";
      }
    }

    if (action)
      match.style = "action " + match.style;

    match.value = url;
    match.comment = title;
    if (iconurl) {
      match.icon = PlacesUtils.favicons
                              .getFaviconLinkForIcon(NetUtil.newURI(iconurl)).spec;
    }
    match.frecency = frecency;

    return match;
  },

  






  get _searchQuery() {
    
    
    
    
    
    
    let query = this.hasBehavior("tag") ? SQL_TAGS_QUERY :
                this.hasBehavior("bookmark") ? SQL_BOOKMARK_QUERY :
                this.hasBehavior("typed") ? SQL_TYPED_QUERY :
                this.hasBehavior("history") ? SQL_HISTORY_QUERY :
                SQL_DEFAULT_QUERY;

    return [
      query,
      {
        parent: PlacesUtils.tagsFolderId,
        query_type: QUERYTYPE_FILTERED,
        matchBehavior: this._matchBehavior,
        searchBehavior: this._behavior,
        
        
        searchString: this._searchTokens.join(" "),
        
        
        maxResults: Prefs.maxRichResults
      }
    ];
  },

  





  get _keywordQuery() {
    
    
    let searchString = this._trimmedOriginalSearchString;
    let queryString = "";
    let queryIndex = searchString.indexOf(" ");
    if (queryIndex != -1) {
      queryString = searchString.substring(queryIndex + 1);
    }
    
    queryString = encodeURIComponent(queryString).replace("%20", "+", "g");

    
    let keyword = this._searchTokens[0];

    return [
      SQL_KEYWORD_QUERY,
      {
        keyword: keyword,
        query_string: queryString,
        query_type: QUERYTYPE_KEYWORD
      }
    ];
  },

  





  get _switchToTabQuery() [
    SQL_SWITCHTAB_QUERY,
    {
      query_type: QUERYTYPE_FILTERED,
      matchBehavior: this._matchBehavior,
      searchBehavior: this._behavior,
      
      
      searchString: this._searchTokens.join(" "),
      maxResults: Prefs.maxRichResults
    }
  ],

  





  get _adaptiveQuery() [
    SQL_ADAPTIVE_QUERY,
    {
      parent: PlacesUtils.tagsFolderId,
      search_string: this._searchString,
      query_type: QUERYTYPE_FILTERED,
      matchBehavior: this._matchBehavior,
      searchBehavior: this._behavior
    }
  ],

  


  get _shouldAutofill() {
    
    if (!Prefs.autofill)
      return false;

    
    
    
    if (Prefs.defaultBehavior != DEFAULT_BEHAVIOR) {
      
      
      if (!this.hasBehavior("typed") &&
          !this.hasBehavior("history") &&
          !this.hasBehavior("bookmark"))
        return false;

      
      if (this.hasBehavior("title") || this.hasBehavior("tags"))
        return false;
    }

    
    
    
    
    if (/\s/.test(this._originalSearchString)) {
      return false;
    }

    
    
    
    
    if (this._searchString.length == 0 ||
        PlacesUtils.bookmarks.getURIForKeyword(this._searchString)) {
      return false;
    }

    return true;
  },

  





  get _hostQuery() {
    let typed = Prefs.autofillTyped || this.hasBehavior("typed");
    let bookmarked =  this.hasBehavior("bookmark");

    return [
      bookmarked ? typed ? SQL_BOOKMARKED_TYPED_HOST_QUERY
                         : SQL_BOOKMARKED_HOST_QUERY
                 : typed ? SQL_TYPED_HOST_QUERY
                         : SQL_HOST_QUERY,
      {
        query_type: QUERYTYPE_AUTOFILL_HOST,
        searchString: this._searchString.toLowerCase()
      }
    ];
  },

  





  get _urlQuery()  {
    let typed = Prefs.autofillTyped || this.hasBehavior("typed");
    let bookmarked =  this.hasBehavior("bookmark");

    return [
      bookmarked ? typed ? SQL_BOOKMARKED_TYPED_URL_QUERY
                         : SQL_BOOKMARKED_URL_QUERY
                 : typed ? SQL_TYPED_URL_QUERY
                         : SQL_URL_QUERY,
      {
        query_type: QUERYTYPE_AUTOFILL_URL,
        searchString: this._autofillUrlSearchString,
        matchBehavior: MATCH_BEGINNING_CASE_SENSITIVE,
        searchBehavior: Ci.mozIPlacesAutoComplete.BEHAVIOR_URL
      }
    ];
  },

 





  notifyResults: function (searchOngoing) {
    let result = this._result;
    let resultCode = result.matchCount ? "RESULT_SUCCESS" : "RESULT_NOMATCH";
    if (searchOngoing) {
      resultCode += "_ONGOING";
    }
    result.setSearchResult(Ci.nsIAutoCompleteResult[resultCode]);
    this._listener.onSearchResult(this._autocompleteSearch, result);
  },
}





function UnifiedComplete() {
  Services.obs.addObserver(this, TOPIC_SHUTDOWN, true);
}

UnifiedComplete.prototype = {
  
  

  observe: function (subject, topic, data) {
    if (topic === TOPIC_SHUTDOWN) {
      this.ensureShutdown();
    }
  },

  
  

  



  _promiseDatabase: null,

  






  getDatabaseHandle: function () {
    if (Prefs.enabled && !this._promiseDatabase) {
      this._promiseDatabase = Task.spawn(function* () {
        let conn = yield Sqlite.cloneStorageConnection({
          connection: PlacesUtils.history.DBConnection,
          readOnly: true
        });

        
        
        
        
        yield conn.execute("PRAGMA cache_size = -6144"); 

        yield SwitchToTabStorage.initDatabase(conn);

        return conn;
      }.bind(this)).then(null, ex => { dump("Couldn't get database handle: " + ex + "\n");
                                       Cu.reportError(ex); });
    }
    return this._promiseDatabase;
  },

  


  ensureShutdown: function () {
    if (this._promiseDatabase) {
      Task.spawn(function* () {
        let conn = yield this.getDatabaseHandle();
        SwitchToTabStorage.shutdown();
        yield conn.close()
      }.bind(this)).then(null, Cu.reportError);
      this._promiseDatabase = null;
    }
  },

  
  

  registerOpenPage: function PAC_registerOpenPage(uri) {
    SwitchToTabStorage.add(uri);
  },

  unregisterOpenPage: function PAC_unregisterOpenPage(uri) {
    SwitchToTabStorage.delete(uri);
  },

  
  

  startSearch: function (searchString, searchParam, previousResult, listener) {
    
    if (this._currentSearch) {
      this.stopSearch();
    }

    
    

    this._currentSearch = new Search(searchString, searchParam, listener,
                                     this, this);

    
    
    if (!Prefs.enabled) {
      this.finishSearch(true);
      return;
    }

    let search = this._currentSearch;
    this.getDatabaseHandle().then(conn => search.execute(conn))
                            .then(() => {
                              if (search == this._currentSearch) {
                                this.finishSearch(true);
                              }
                            }, ex => { dump("Query failed: " + ex + "\n");
                                       Cu.reportError(ex); });
  },

  stopSearch: function () {
    if (this._currentSearch) {
      this._currentSearch.cancel();
    }
    
    
    this.finishSearch();
  },

  






  finishSearch: function (notify=false) {
    TelemetryStopwatch.cancel(TELEMETRY_1ST_RESULT);
    TelemetryStopwatch.cancel(TELEMETRY_6_FIRST_RESULTS);
    
    let search = this._currentSearch;
    delete this._currentSearch;

    if (!notify)
      return;

    
    
    
    
    
    
    
    
    
    search.notifyResults(false);
  },

  
  

  onValueRemoved: function (result, spec, removeFromDB) {
    if (removeFromDB) {
      PlacesUtils.history.removePage(NetUtil.newURI(spec));
    }
  },

  
  

  get searchType() Ci.nsIAutoCompleteSearchDescriptor.SEARCH_TYPE_IMMEDIATE,

  
  

  classID: Components.ID("f964a319-397a-4d21-8be6-5cdd1ee3e3ae"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(UnifiedComplete),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIAutoCompleteSearch,
    Ci.nsIAutoCompleteSimpleResultListener,
    Ci.nsIAutoCompleteSearchDescriptor,
    Ci.mozIPlacesAutoComplete,
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference
  ])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([UnifiedComplete]);
