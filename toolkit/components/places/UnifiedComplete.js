





"use strict";




const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

const PREF_BRANCH = "browser.urlbar.";


const PREF_ENABLED =                [ "autocomplete.enabled",   true ];
const PREF_AUTOFILL =               [ "autoFill",               true ];
const PREF_AUTOFILL_TYPED =         [ "autoFill.typed",         true ];
const PREF_AUTOFILL_SEARCHENGINES = [ "autoFill.searchEngines", false ];
const PREF_RESTYLESEARCHES        = [ "restyleSearches",        false ];
const PREF_DELAY =                  [ "delay",                  50 ];
const PREF_BEHAVIOR =               [ "matchBehavior", MATCH_BOUNDARY_ANYWHERE ];
const PREF_FILTER_JS =              [ "filter.javascript",      true ];
const PREF_MAXRESULTS =             [ "maxRichResults",         25 ];
const PREF_RESTRICT_HISTORY =       [ "restrict.history",       "^" ];
const PREF_RESTRICT_BOOKMARKS =     [ "restrict.bookmark",      "*" ];
const PREF_RESTRICT_TYPED =         [ "restrict.typed",         "~" ];
const PREF_RESTRICT_TAG =           [ "restrict.tag",           "+" ];
const PREF_RESTRICT_SWITCHTAB =     [ "restrict.openpage",      "%" ];
const PREF_MATCH_TITLE =            [ "match.title",            "#" ];
const PREF_MATCH_URL =              [ "match.url",              "@" ];

const PREF_SUGGEST_HISTORY =        [ "suggest.history",        true ];
const PREF_SUGGEST_BOOKMARK =       [ "suggest.bookmark",       true ];
const PREF_SUGGEST_OPENPAGE =       [ "suggest.openpage",       true ];
const PREF_SUGGEST_HISTORY_ONLYTYPED = [ "suggest.history.onlyTyped", false ];



const MATCH_ANYWHERE = Ci.mozIPlacesAutoComplete.MATCH_ANYWHERE;
const MATCH_BOUNDARY_ANYWHERE = Ci.mozIPlacesAutoComplete.MATCH_BOUNDARY_ANYWHERE;
const MATCH_BOUNDARY = Ci.mozIPlacesAutoComplete.MATCH_BOUNDARY;
const MATCH_BEGINNING = Ci.mozIPlacesAutoComplete.MATCH_BEGINNING;
const MATCH_BEGINNING_CASE_SENSITIVE = Ci.mozIPlacesAutoComplete.MATCH_BEGINNING_CASE_SENSITIVE;



const QUERYTYPE_FILTERED            = 0;
const QUERYTYPE_AUTOFILL_HOST       = 1;
const QUERYTYPE_AUTOFILL_URL        = 2;
const QUERYTYPE_AUTOFILL_PREDICTURL = 3;




const TITLE_TAGS_SEPARATOR = " \u2013 ";


const TITLE_SEARCH_ENGINE_SEPARATOR = " \u00B7\u2013\u00B7 ";


const TELEMETRY_1ST_RESULT = "PLACES_AUTOCOMPLETE_1ST_RESULT_TIME_MS";
const TELEMETRY_6_FIRST_RESULTS = "PLACES_AUTOCOMPLETE_6_FIRST_RESULTS_TIME_MS";

const FRECENCY_DEFAULT = 1000;


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
     h.visit_count, h.typed, bookmarked, 0,
     :matchBehavior, :searchBehavior)
     ORDER BY h.frecency DESC, h.id DESC
     LIMIT 1`;
  return query;
}

const SQL_URL_QUERY = urlQuery();

const SQL_TYPED_URL_QUERY = urlQuery("AND h.typed = 1");


const SQL_BOOKMARKED_URL_QUERY = urlQuery("AND bookmarked");

const SQL_BOOKMARKED_TYPED_URL_QUERY = urlQuery("AND bookmarked AND h.typed = 1");




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
  let types = ["History", "Bookmark", "Openpage", "Typed"];

  function syncEnabledPref(init = false) {
    let suggestPrefs = [PREF_SUGGEST_HISTORY, PREF_SUGGEST_BOOKMARK, PREF_SUGGEST_OPENPAGE];

    if (init) {
      
      store.enabled = prefs.get(...PREF_ENABLED);
      store.suggestHistory = prefs.get(...PREF_SUGGEST_HISTORY);
      store.suggestBookmark = prefs.get(...PREF_SUGGEST_BOOKMARK);
      store.suggestOpenpage = prefs.get(...PREF_SUGGEST_OPENPAGE);
      store.suggestTyped = prefs.get(...PREF_SUGGEST_HISTORY_ONLYTYPED);
    }

    if (store.enabled) {
      
      
      if (types.every(type => store["suggest" + type] == false)) {
        for (let type of suggestPrefs) {
          prefs.set(...type);
        }
      }
    } else {
      
      for (let type of suggestPrefs) {
        prefs.set(type[0], false);
      }
    }
  }

  function loadPrefs(subject, topic, data) {
    store.enabled = prefs.get(...PREF_ENABLED);
    store.autofill = prefs.get(...PREF_AUTOFILL);
    store.autofillTyped = prefs.get(...PREF_AUTOFILL_TYPED);
    store.autofillSearchEngines = prefs.get(...PREF_AUTOFILL_SEARCHENGINES);
    store.restyleSearches = prefs.get(...PREF_RESTYLESEARCHES);
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
    store.suggestHistory = prefs.get(...PREF_SUGGEST_HISTORY);
    store.suggestBookmark = prefs.get(...PREF_SUGGEST_BOOKMARK);
    store.suggestOpenpage = prefs.get(...PREF_SUGGEST_OPENPAGE);
    store.suggestTyped = prefs.get(...PREF_SUGGEST_HISTORY_ONLYTYPED);

    
    if (!store.suggestHistory) {
      store.suggestTyped = false;
    }
    store.defaultBehavior = types.reduce((memo, type) => {
      let prefValue = store["suggest" + type];
      return memo | (prefValue &&
                     Ci.mozIPlacesAutoComplete["BEHAVIOR_" + type.toUpperCase()]);
    }, 0);

    
    
    
    
    store.emptySearchDefaultBehavior = Ci.mozIPlacesAutoComplete.BEHAVIOR_RESTRICT;
    if (store.suggestHistory) {
      store.emptySearchDefaultBehavior |= Ci.mozIPlacesAutoComplete.BEHAVIOR_HISTORY |
                                          Ci.mozIPlacesAutoComplete.BEHAVIOR_TYPED;
    } else if (store.suggestBookmark) {
      store.emptySearchDefaultBehavior |= Ci.mozIPlacesAutoComplete.BEHAVIOR_BOOKMARK;
    } else {
      store.emptySearchDefaultBehavior |= Ci.mozIPlacesAutoComplete.BEHAVIOR_OPENPAGE;
    }

    
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

    
    
    if (data == PREF_BRANCH + PREF_ENABLED[0]) {
      syncEnabledPref();
    }
  }

  let store = {
    observe: loadPrefs,
    QueryInterface: XPCOMUtils.generateQI([ Ci.nsIObserver ])
  };

  
  syncEnabledPref(true);

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











function makeActionURL(action, params) {
  let url = "moz-action:" + action + "," + JSON.stringify(params);
  
  return NetUtil.newURI(url).spec;
}






function Search(searchString, searchParam, autocompleteListener,
                resultListener, autocompleteSearch) {
  
  this._originalSearchString = searchString;
  this._trimmedOriginalSearchString = searchString.trim();
  this._searchString = fixupSearchText(this._trimmedOriginalSearchString.toLowerCase());

  this._matchBehavior = Prefs.matchBehavior;
  
  this._behavior = this._searchString ? Prefs.defaultBehavior
                                      : Prefs.emptySearchDefaultBehavior;

  let params = new Set(searchParam.split(" "));
  this._enableActions = params.has("enable-actions");
  this._disablePrivateActions = params.has("disable-private-actions");

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
    type = type.toUpperCase();
    this._behavior |=
      Ci.mozIPlacesAutoComplete["BEHAVIOR_" + type];

    
    if (type == "TYPED") {
      this.setBehavior("history");
    }
  },

  






  hasBehavior: function (type) {
    let behavior = Ci.mozIPlacesAutoComplete["BEHAVIOR_" + type.toUpperCase()];

    if (this._disablePrivateActions &&
        behavior == Ci.mozIPlacesAutoComplete.BEHAVIOR_OPENPAGE) {
      return false;
    }

    return this._behavior & behavior;
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
    let foundToken = false;
    
    for (let i = tokens.length - 1; i >= 0; i--) {
      let behavior = Prefs.tokenToBehaviorMap.get(tokens[i]);
      
      
      if (behavior && (behavior != "openpage" || this._enableActions)) {
        
        
        if (!foundToken) {
          foundToken = true;
          
          this._behavior = 0;
          this.setBehavior("restrict");
        }
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
    if (!this.pending)
      return;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    let queries = [ this._adaptiveQuery ];

    
    
    if (this.hasBehavior("openpage")) {
      queries.push(this._switchToTabQuery);
    }
    queries.push(this._searchQuery);

    
    
    
    
    
    let hasFirstResult = false;

    if (this._searchTokens.length > 0) {
      
      hasFirstResult = yield this._matchPlacesKeyword();
    }

    if (this.pending && this._enableActions && !hasFirstResult) {
      
      
      hasFirstResult = yield this._matchSearchEngineAlias();
    }

    let shouldAutofill = this._shouldAutofill;
    if (this.pending && !hasFirstResult && shouldAutofill) {
      
      
      
      
      hasFirstResult = yield this._matchKnownUrl(conn, queries);
    }

    if (this.pending && !hasFirstResult && shouldAutofill) {
      
      hasFirstResult = yield this._matchSearchEngineUrl();
    }

    if (this.pending && this._enableActions && !hasFirstResult) {
      
      
      yield this._matchHeuristicFallback();
    }

    
    

    yield this._sleep(Prefs.delay);
    if (!this.pending)
      return;

    for (let [query, params] of queries) {
      let hasResult = yield conn.executeCached(query, params, this._onResultRow.bind(this));

      if (this.pending && this._enableActions && !hasResult &&
          params.query_type == QUERYTYPE_AUTOFILL_URL) {
        
        
        yield this._matchHeuristicFallback();
      }

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
  }),

  _matchKnownUrl: function* (conn, queries) {
    
    let lastSlashIndex = this._searchString.lastIndexOf("/");
    
    if (lastSlashIndex != -1) {
      
      if (lastSlashIndex < this._searchString.length - 1) {
        
        
        
        
        
        let gotResult = false;
        let [ query, params ] = this._urlPredictQuery;
        yield conn.executeCached(query, params, row => {
          gotResult = true;
          queries.unshift(this._urlQuery);
        });
        return gotResult;
      }

      return false;
    }

    let gotResult = false;
    let [ query, params ] = this._hostQuery;
    yield conn.executeCached(query, params, row => {
      gotResult = true;
      this._onResultRow(row);
    });

    return gotResult;
  },

  _matchPlacesKeyword: function* () {
    
    let keyword = this._searchTokens[0];
    let entry = yield PlacesUtils.keywords.fetch(this._searchTokens[0]);
    if (!entry)
      return false;

    
    let searchString = this._trimmedOriginalSearchString;
    let queryString = "";
    let queryIndex = searchString.indexOf(" ");
    if (queryIndex != -1) {
      queryString = searchString.substring(queryIndex + 1);
    }
    
    queryString = encodeURIComponent(queryString).replace(/%20/g, "+");
    let escapedURL = entry.url.href.replace("%s", queryString);

    let style = (this._enableActions ? "action " : "") + "keyword";
    let actionURL = makeActionURL("keyword", { url: escapedURL,
                                               input: this._originalSearchString });
    let value = this._enableActions ? actionURL : escapedURL;
    
    let comment = entry.url.host;

    this._addMatch({ value, comment, style, frecency: FRECENCY_DEFAULT });
    return true;
  },

  _matchSearchEngineUrl: function* () {
    if (!Prefs.autofillSearchEngines)
      return false;

    let match = yield PlacesSearchAutocompleteProvider.findMatchByToken(
                                                           this._searchString);
    if (!match)
      return false;

    
    
    
    
    
    
    
    try {
      let prefixURI = NetUtil.newURI(this._strippedPrefix);
      let finalURI = NetUtil.newURI(match.url);
      if (prefixURI.scheme != finalURI.scheme)
        return false;
    } catch (e) {}

    
    
    if (this._strippedPrefix.endsWith("www.") &&
        !stripHttpAndTrim(match.url).startsWith("www."))
      return false;

    let value = this._strippedPrefix + match.token;

    
    
    
    if (!value.startsWith(this._originalSearchString)) {
      Components.utils.reportError(`Trying to inline complete in-the-middle
                                    ${this._originalSearchString} to ${value}`);
      return false;
    }

    this._result.setDefaultIndex(0);
    this._addMatch({
      value: value,
      comment: match.engineName,
      icon: match.iconUrl,
      style: "priority-search",
      finalCompleteValue: match.url,
      frecency: FRECENCY_DEFAULT
    });
    return true;
  },

  _matchSearchEngineAlias: function* () {
    if (this._searchTokens.length < 2)
      return false;

    let alias = this._searchTokens[0];
    let match = yield PlacesSearchAutocompleteProvider.findMatchByAlias(alias);
    if (!match)
      return false;

    match.engineAlias = alias;
    let query = this._searchTokens.slice(1).join(" ");

    yield this._addSearchEngineMatch(match, query);
    return true;
  },

  _matchCurrentSearchEngine: function* () {
    let match = yield PlacesSearchAutocompleteProvider.getDefaultMatch();
    if (!match)
      return;

    let query = this._originalSearchString;

    yield this._addSearchEngineMatch(match, query);
  },

  _addSearchEngineMatch: function* (match, query) {
    let actionURLParams = {
      engineName: match.engineName,
      input: this._originalSearchString,
      searchQuery: query,
    };
    if (match.engineAlias) {
      actionURLParams.alias = match.engineAlias;
    }
    let value = makeActionURL("searchengine", actionURLParams);

    this._addMatch({
      value: value,
      comment: match.engineName,
      icon: match.iconUrl,
      style: "action searchengine",
      frecency: FRECENCY_DEFAULT,
    });
  },

  
  
  
  
  _matchHeuristicFallback: function* () {
    
    let hasFirstResult = yield this._matchUnknownUrl();
    
    
    

    if (this.pending && !hasFirstResult) {
      
      yield this._matchCurrentSearchEngine();
    }
  },

  
  
  _matchUnknownUrl: function* () {
    let flags = Ci.nsIURIFixup.FIXUP_FLAG_FIX_SCHEME_TYPOS |
                Ci.nsIURIFixup.FIXUP_FLAG_REQUIRE_WHITELISTED_HOST;
    let fixupInfo = null;
    try {
      fixupInfo = Services.uriFixup.getFixupURIInfo(this._originalSearchString,
                                                    flags);
    } catch (e) {
      return false;
    }

    let uri = fixupInfo.preferredURI;
    
    
    
    
    let hostExpected = new Set(["http", "https", "ftp", "chrome", "resource"]);
    if (!uri || (hostExpected.has(uri.scheme) && !uri.host))
      return false;

    let value = makeActionURL("visiturl", {
      url: uri.spec,
      input: this._originalSearchString,
    });

    let match = {
      value: value,
      comment: uri.spec,
      style: "action visiturl",
      frecency: 0,
    };

    try {
      let favicon = yield PlacesUtils.promiseFaviconLinkUrl(uri);
      if (favicon)
        match.icon = favicon.spec;
    } catch (e) {
      
    };

    this._addMatch(match);
    return true;
  },

  _onResultRow: function (row) {
    TelemetryStopwatch.finish(TELEMETRY_1ST_RESULT);
    let queryType = row.getResultByIndex(QUERYINDEX_QUERYTYPE);
    let match;
    switch (queryType) {
      case QUERYTYPE_AUTOFILL_HOST:
        this._result.setDefaultIndex(0);
        
      case QUERYTYPE_AUTOFILL_PREDICTURL:
        match = this._processHostRow(row);
        break;
      case QUERYTYPE_AUTOFILL_URL:
        this._result.setDefaultIndex(0);
        match = this._processUrlRow(row);
        break;
      case QUERYTYPE_FILTERED:
        match = this._processRow(row);
        break;
    }
    this._addMatch(match);
    
    
    if (!this.pending)
      throw StopIteration;
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

    
    let urlMapKey = stripHttpAndTrim(match.value);
    if ((!match.placeId || !this._usedPlaceIds.has(match.placeId)) &&
        !this._usedURLs.has(urlMapKey)) {
      
      
      
      
      
      
      if (match.placeId)
        this._usedPlaceIds.add(match.placeId);
      this._usedURLs.add(urlMapKey);

      if (!match.style) {
        match.style = "favicon";
      }

      
      if (Prefs.restyleSearches && match.style == "favicon") {
        this._maybeRestyleSearchMatch(match);
      }

      this._result.appendMatch(match.value,
                               match.comment,
                               match.icon || PlacesUtils.favicons.defaultFavicon.spec,
                               match.style,
                               match.finalCompleteValue || "");
      notifyResults = true;
    }

    if (this._result.matchCount == 6)
      TelemetryStopwatch.finish(TELEMETRY_6_FIRST_RESULTS);

    if (this._result.matchCount == Prefs.maxRichResults) {
      
      
      
      this.cancel();
    } else if (notifyResults) {
      
      this.notifyResults(true);
    }
  },

  _processHostRow: function (row) {
    let match = {};
    let trimmedHost = row.getResultByIndex(QUERYINDEX_URL);
    let untrimmedHost = row.getResultByIndex(QUERYINDEX_TITLE);
    let frecency = row.getResultByIndex(QUERYINDEX_FRECENCY);

    
    
    if (untrimmedHost &&
        !untrimmedHost.toLowerCase().includes(this._trimmedOriginalSearchString.toLowerCase())) {
      untrimmedHost = null;
    }

    match.value = this._strippedPrefix + trimmedHost;
    
    match.comment = stripHttpAndTrim(trimmedHost);
    match.finalCompleteValue = untrimmedHost;

    try {
      let iconURI = NetUtil.newURI(untrimmedHost);
      iconURI.path = "/favicon.ico";
      match.icon = PlacesUtils.favicons.getFaviconLinkForIcon(iconURI).spec;
    } catch (e) {
      
    }

    
    
    match.frecency = frecency;
    match.style = "autofill";
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
        !untrimmedURL.toLowerCase().includes(this._trimmedOriginalSearchString.toLowerCase())) {
      untrimmedURL = null;
     }

    match.value = this._strippedPrefix + url;
    match.comment = url;
    match.finalCompleteValue = untrimmedURL;
    
    
    match.frecency = frecency;
    match.style = "autofill";
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

    
    
    let url = escapedURL;
    let action = null;
    if (this._enableActions && openPageCount > 0 && this.hasBehavior("openpage")) {
      url = makeActionURL("switchtab", {url: escapedURL});
      action = "switchtab";
    }

    
    let title = bookmarkTitle || historyTitle;

    
    let showTags = !!tags;

    
    
    if (this.hasBehavior("history") && !this.hasBehavior("bookmark") &&
        !showTags) {
      showTags = false;
      match.style = "favicon";
    }

    
    if (showTags) {
      title += TITLE_TAGS_SEPARATOR + tags;
    }

    
    
    
    if (!match.style) {
      
      
      
      if (showTags) {
        
        
        match.style = this.hasBehavior("bookmark") ? "bookmark-tag" : "tag";
      }
      else if (bookmarked) {
        match.style = "bookmark";
      }
    }

    if (action)
      match.style = "action " + action;

    match.value = url;
    match.comment = title;
    if (iconurl) {
      match.icon = PlacesUtils.favicons
                              .getFaviconLinkForIcon(NetUtil.newURI(iconurl)).spec;
    }
    match.frecency = frecency;

    return match;
  },

  



  get _suggestionPrefQuery() {
    if (!this.hasBehavior("restrict") && this.hasBehavior("history") &&
        this.hasBehavior("bookmark")) {
      return this.hasBehavior("typed") ? defaultQuery("AND h.typed = 1")
                                       : defaultQuery();
    }
    let conditions = [];
    if (this.hasBehavior("history")) {
      
      
      
      conditions.push("+h.visit_count > 0");
    }
    if (this.hasBehavior("typed")) {
      conditions.push("h.typed = 1");
    }
    if (this.hasBehavior("bookmark")) {
      conditions.push("bookmarked");
    }
    if (this.hasBehavior("tag")) {
      conditions.push("tags NOTNULL");
    }

    return conditions.length ? defaultQuery("AND " + conditions.join(" AND "))
                             : defaultQuery();
  },

  






  get _searchQuery() {
    let query = this._suggestionPrefQuery;

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

    if (!this._searchTokens.length == 1)
      return false;

    
    if (!this.hasBehavior("history") &&
        !this.hasBehavior("bookmark"))
      return false;

    
    if (this.hasBehavior("title") || this.hasBehavior("tag"))
      return false;

    
    
    
    
    if (/\s/.test(this._originalSearchString))
      return false;

    if (this._searchString.length == 0)
      return false;

    return true;
  },

  





  get _hostQuery() {
    let typed = Prefs.autofillTyped || this.hasBehavior("typed");
    let bookmarked = this.hasBehavior("bookmark") && !this.hasBehavior("history");

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

  




  get _urlPredictQuery() {
    
    
    
    let slashIndex = this._searchString.indexOf("/");

    let host = this._searchString.substring(0, slashIndex);
    host = host.toLowerCase();

    return [
      SQL_HOST_QUERY,
      {
        query_type: QUERYTYPE_AUTOFILL_PREDICTURL,
        searchString: host
      }
    ];
  },

  





  get _urlQuery()  {
    let typed = Prefs.autofillTyped || this.hasBehavior("typed");
    let bookmarked = this.hasBehavior("bookmark") && !this.hasBehavior("history");
    let searchBehavior = Ci.mozIPlacesAutoComplete.BEHAVIOR_URL;

    
    
    if (typed) {
      searchBehavior |= Ci.mozIPlacesAutoComplete.BEHAVIOR_HISTORY |
                        Ci.mozIPlacesAutoComplete.BEHAVIOR_TYPED;
    } else {
      
      searchBehavior |= Ci.mozIPlacesAutoComplete.BEHAVIOR_HISTORY;
    }
    if (bookmarked) {
      searchBehavior |= Ci.mozIPlacesAutoComplete.BEHAVIOR_BOOKMARK;
    }

    return [
      bookmarked ? typed ? SQL_BOOKMARKED_TYPED_URL_QUERY
                         : SQL_BOOKMARKED_URL_QUERY
                 : typed ? SQL_TYPED_URL_QUERY
                         : SQL_URL_QUERY,
      {
        query_type: QUERYTYPE_AUTOFILL_URL,
        searchString: this._autofillUrlSearchString,
        matchBehavior: MATCH_BEGINNING_CASE_SENSITIVE,
        searchBehavior: searchBehavior
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
  
  
  
  
  Prefs;
}

UnifiedComplete.prototype = {
  
  

  



  _promiseDatabase: null,

  






  getDatabaseHandle: function () {
    if (Prefs.enabled && !this._promiseDatabase) {
      this._promiseDatabase = Task.spawn(function* () {
        let conn = yield Sqlite.cloneStorageConnection({
          connection: PlacesUtils.history.DBConnection,
          readOnly: true
        });

        try {
           Sqlite.shutdown.addBlocker("Places UnifiedComplete.js clone closing",
                                      Task.async(function* () {
                                        SwitchToTabStorage.shutdown();
                                        yield conn.close();
                                      }));
        } catch (ex) {
          
          yield conn.close();
          throw ex;
        }

        
        
        
        
        yield conn.execute("PRAGMA cache_size = -6144"); 

        yield SwitchToTabStorage.initDatabase(conn);

        return conn;
      }.bind(this)).then(null, ex => { dump("Couldn't get database handle: " + ex + "\n");
                                       Cu.reportError(ex); });
    }
    return this._promiseDatabase;
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
                            .then(null, ex => {
                              dump(`Query failed: ${ex}\n`);
                              Cu.reportError(ex);
                            })
                            .then(() => {
                              if (search == this._currentSearch) {
                                this.finishSearch(true);
                              }
                            });
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
