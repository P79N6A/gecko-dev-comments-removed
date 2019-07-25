




































 




var EXPORTED_SYMBOLS = ["HistoryEntry", "DumpHistory"];

const CC = Components.classes;
const CI = Components.interfaces;
const CU = Components.utils;

CU.import("resource://gre/modules/Services.jsm");
CU.import("resource://gre/modules/PlacesUtils.jsm");
CU.import("resource://tps/logger.jsm");
CU.import("resource://services-sync/async.js");

var DumpHistory = function TPS_History__DumpHistory() {
  let writer = {
    value: "",
    write: function PlacesItem__dump__write(aStr, aLen) {
      this.value += aStr;
    }
  };

  let query = PlacesUtils.history.getNewQuery();
  let options = PlacesUtils.history.getNewQueryOptions();
  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  Logger.logInfo("\n\ndumping history\n", true);
  for (var i = 0; i < root.childCount; i++) {
    let node = root.getChild(i);
    let uri = node.uri;
    let curvisits = HistoryEntry._getVisits(uri);
    for each (var visit in curvisits) {
      Logger.logInfo("URI: " + uri + ", type=" + visit.type + ", date=" + visit.date, true);
    }
  }
  root.containerOpen = false;
  Logger.logInfo("\nend history dump\n", true);
};






var HistoryEntry = {
  




  get _db() {
    return PlacesUtils.history.QueryInterface(CI.nsPIPlacesDatabase).DBConnection;
  },

  






  get _visitStm() {
    let stm = this._db.createStatement(
      "SELECT visit_type type, visit_date date " +
      "FROM moz_historyvisits " +
      "WHERE place_id = (" +
        "SELECT id " +
        "FROM moz_places " +
        "WHERE url = :url) " +
      "ORDER BY date DESC LIMIT 10");
    this.__defineGetter__("_visitStm", function() stm);
    return stm;
  },

  









  _getVisits: function HistStore__getVisits(uri) {
    this._visitStm.params.url = uri;
    return Async.querySpinningly(this._visitStm, ["date", "type"]);
  },

  









  Add: function(item, usSinceEpoch) {
    Logger.AssertTrue("visits" in item && "uri" in item,
      "History entry in test file must have both 'visits' " +
      "and 'uri' properties");
    let uri = Services.io.newURI(item.uri, null, null);
    for each (visit in item.visits) {
      let visitId = PlacesUtils.history.addVisit(
                    uri, 
                    usSinceEpoch + (visit.date * 60 * 60 * 1000 * 1000), 
                    null, visit.type, 
                    visit.type == 5 || visit.type == 6, 0);
      Logger.AssertTrue(visitId, "Error adding history entry");
      if ("title" in item)
        PlacesUtils.history.setPageTitle(uri, item.title);
    }
  },

  









  Find: function(item, usSinceEpoch) {
    Logger.AssertTrue("visits" in item && "uri" in item,
      "History entry in test file must have both 'visits' " +
      "and 'uri' properties");
    let curvisits = this._getVisits(item.uri);
    for each (visit in curvisits) {
      for each (itemvisit in item.visits) {
        let expectedDate = itemvisit.date * 60 * 60 * 1000 * 1000 
            + usSinceEpoch;
        if (visit.type == itemvisit.type && visit.date == expectedDate) {
          itemvisit.found = true;
        }
      }
    }
    
    let all_items_found = true;
    for each (itemvisit in item.visits) {
      all_items_found = all_items_found && "found" in itemvisit;
      Logger.logInfo("History entry for " + item.uri + ", type:" + 
              itemvisit.type + ", date:" + itemvisit.date + 
              ("found" in itemvisit ? " is present" : " is not present"));
    }
    return all_items_found;
  },

  









  Delete: function(item, usSinceEpoch) {
    if ("uri" in item) {
      let uri = Services.io.newURI(item.uri, null, null);
      PlacesUtils.history.removePage(uri);
    }
    else if ("host" in item) {
      PlacesUtils.history.removePagesFromHost(item.host, false);
    }
    else if ("begin" in item && "end" in item) {
      PlacesUtils.history.removeVisitsByTimeframe(
          usSinceEpoch + (item.begin * 60 * 60 * 1000 * 1000), 
          usSinceEpoch + (item.end * 60 * 60 * 1000 * 1000));
    }
    else {
      Logger.AssertTrue(false, "invalid entry in delete history");
    }
  },
};

