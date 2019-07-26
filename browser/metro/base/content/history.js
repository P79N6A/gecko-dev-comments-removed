



'use strict';



function HistoryView(aSet) {
  this._set = aSet;
  this._set.controller = this;

  let history = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  history.addObserver(this, false);
}

HistoryView.prototype = {
  _set:null,

  handleItemClick: function tabview_handleItemClick(aItem) {
    let url = aItem.getAttribute("value");
    BrowserUI.goToURI(url);
  },

  populateGrid: function populateGrid() {
    let query = gHistSvc.getNewQuery();
    let options = gHistSvc.getNewQueryOptions();
    options.excludeQueries = true;
    options.queryType = options.QUERY_TYPE_HISTORY;
    options.maxResults = StartUI.maxResultsPerSection;
    options.resultType = options.RESULTS_AS_URI;
    options.sortingMode = options.SORT_BY_DATE_DESCENDING;

    let result = gHistSvc.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    let childCount = rootNode.childCount;

    for (let i = 0; i < childCount; i++) {
      let node = rootNode.getChild(i);
      let uri = node.uri;
      let title = node.title || uri;

      let item = this._set.appendItem(title, uri);
      item.setAttribute("iconURI", node.icon);
    }

    rootNode.containerOpen = false;
  },

  destruct: function destruct() {
  },

  

  onBeginUpdateBatch: function() {
  },

  onEndUpdateBatch: function() {
  },

  onVisit: function(aURI, aVisitID, aTime, aSessionID,
                    aReferringID, aTransitionType) {
  },

  onTitleChanged: function(aURI, aPageTitle) {
  },

  onDeleteURI: function(aURI) {
  },

  onClearHistory: function() {
    this._set.clearAll();
  },

  onPageChanged: function(aURI, aWhat, aValue) {
  },

  onPageExpired: function(aURI, aVisitTime, aWholeEntry) {
  },

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsINavHistoryObserver) ||
        iid.equals(Components.interfaces.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

let HistoryStartView = {
  _view: null,
  get _grid() { return document.getElementById("start-history-grid"); },

  show: function show() {
    this._grid.arrangeItems();
  },

  init: function init() {
    this._view = new HistoryView(this._grid);
    this._view.populateGrid();
  },

  uninit: function uninit() {
    this._view.destruct();
  }
};

let HistoryPanelView = {
  _view: null,
  get _grid() { return document.getElementById("history-list"); },
  get visible() { return PanelUI.isPaneVisible("history-container"); },

  show: function show() {
    this._grid.arrangeItems();
  },

  init: function init() {
    this._view = new HistoryView(this._grid);
    this._view.populateGrid();
  },

  uninit: function uninit() {
    this._view.destruct();
  }
};

