
















































function PrefHandler(pref, defaultValue, serializable) {
  this._pref = pref;
  this._defaultValue = defaultValue;
  this._serializable = serializable;

  this._pb = 
    Cc["@mozilla.org/preferences-service;1"].
    getService(Components.interfaces.nsIPrefBranch2);
  this._pb.addObserver(this._pref, this, false);
}

PrefHandler.prototype = {
  


  destroy: function PC_PH_destroy() {
    this._pb.removeObserver(this._pref, this);
    this._pref = null;
    this._pb = null;
    this._defaultValue = null;
    this._serializable = null;
  },

  







  observe: function PC_PH_observe(subject, topic, data) {
    if (topic == "nsPref:changed" && data == this._pref)
      this._value = null;
  },
  
  


  _value: null,

  


  get value() { 
    if (!this._value) {
      if (this._pb.prefHasUserValue(this._pref)) {
        var valueString = this._pb.getCharPref(this._pref);
        this._value = this._serializable.deserialize(valueString);
      }
      else
        this._value = this._defaultValue;
    }
    return this._value;
  },
  
  




  set value(value) {
    if (value != this._value) {
      this._pb.setCharPref(this._pref, this._serializable.serialize(value));
      var ps = this._pb.QueryInterface(Ci.nsIPrefService);
      ps.savePrefFile(null);
    }
    return value;
  }
};



const PREF_PLACES_ORGANIZER_OPTIONS_HISTORY = 
  "browser.places.organizer.options.history";
const PREF_PLACES_ORGANIZER_OPTIONS_BOOKMARKS = 
  "browser.places.organizer.options.bookmarks";
const PREF_PLACES_ORGANIZER_OPTIONS_SUBSCRIPTIONS = 
  "browser.places.organizer.options.subscriptions";

























var OptionsFilter = {
  






  deserialize: function OF_deserialize(string) {
    var optionsRef = { };
    PlacesUtils.history.queryStringToQueries(string, {}, {}, optionsRef);
    return optionsRef.value;    
  },
  
  






  serialize: function OF_serialize(options) {
    var query = PlacesUtils.history.getNewQuery();
    return PlacesUtils.history.queriesToQueryString([query], 1, options);
  },
  
  


  historyHandler: null,

  


  bookmarksHandler: null,

  



  overrideHandlers: { },
  
  


  _grouper: null,

  


  init: function OF_init(grouper) {
    this._grouper = grouper;
  
    var history = PlacesUtils.history;
    const NHQO = Ci.nsINavHistoryQueryOptions;
    
    var defaultHistoryOptions = history.getNewQueryOptions();
    defaultHistoryOptions.sortingMode = NHQO.SORT_BY_DATE_DESCENDING;
    var defaultBookmarksOptions = history.getNewQueryOptions();
    defaultBookmarksOptions.setGroupingMode([NHQO.GROUP_BY_FOLDER], 1);
    var defaultSubscriptionsOptions = history.getNewQueryOptions();
  
    this.historyHandler = 
      new PrefHandler(PREF_PLACES_ORGANIZER_OPTIONS_HISTORY, 
                      defaultHistoryOptions, this);
    this.bookmarksHandler = 
      new PrefHandler(PREF_PLACES_ORGANIZER_OPTIONS_BOOKMARKS, 
                      defaultBookmarksOptions, this);
    this.overrideHandlers["livemark/"] = 
      new PrefHandler(PREF_PLACES_ORGANIZER_OPTIONS_SUBSCRIPTIONS, 
                      defaultSubscriptionsOptions, this);
  },
  
  


  destroy: function OF_destroy() {
    this.historyHandler.destroy();
    this.bookmarksHandler.destroy();
    this.overrideHandlers["livemark/"].destroy();
  },
  
  







  getHandler: function OF_getHandler(queries) {
    var countRef = { };
    queries[0].getFolders(countRef);
    if (countRef.value > 0 || queries[0].onlyBookmarks)
      return this.bookmarksHandler;
    for (var substr in this.overrideHandlers) {
      if (queries[0].annotation.substr(0, substr.length) == substr) 
        return this.overrideHandlers[substr];
    }
    return this.historyHandler;
  },
  
  











  filter: function OF_filter(queries, options, handler) {
    if (!handler)
      handler = this.getHandler(queries);
    var overrideOptions = handler.value;
    
    var overrideGroupings = overrideOptions.getGroupingMode({});
    options.setGroupingMode(overrideGroupings, overrideGroupings.length);
    options.sortingMode = overrideOptions.sortingMode;
    
    this._grouper.updateGroupingUI(queries, options, handler);

    return options;
  },
  
  





  update: function OF_update(result) {
    var queryNode = asQuery(result.root);
    var queries = queryNode.getQueries({});
    
    var options = queryNode.queryOptions.clone();
    options.sortingMode = result.sortingMode;
    this.getHandler(queries).value = options;

    
    
    window.document.commandDispatcher.updateCommands("sort");
  }
};
