



































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;


function DummyObserver() {
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.notifyObservers(null, "dummy-observer-created", null);
}

DummyObserver.prototype = {
  
  onBeginUpdateBatch: function() {},
  onEndUpdateBatch: function() {},
  onVisit: function(aURI, aVisitID, aTime, aSessionID, aReferringID, aTransitionType) {
    let os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    os.notifyObservers(null, "dummy-observer-visited", null);
  },
  onTitleChanged: function(aURI, aPageTitle) {},
  onBeforeDeleteURI: function(aURI) {},
  onDeleteURI: function(aURI) {},
  onClearHistory: function() {},
  onPageChanged: function(aURI, aWhat, aValue) {},
  onDeleteVisits: function(aURI, aVisitTime) {},

  
  
  
  onItemAdded: function(aItemId, aParentId, aIndex, aItemType) {
    let os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    os.notifyObservers(null, "dummy-observer-item-added", null);
  },
  onItemChanged: function () {},
  onBeforeItemRemoved: function() {},
  onItemRemoved: function() {},
  onItemVisited: function() {},
  onItemMoved: function() {},

  classDescription: "Dummy observer used to test category observers",
  classID: Components.ID("62e221d3-68c3-4e1a-8943-a27beb5005fe"),
  contractID: "@mozilla.org/places/test/dummy-observer;1",

  
  
  _xpcom_categories: [
    { category: "bookmark-observers" },
    { category: "history-observers" }
  ],

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavBookmarkObserver,
    Ci.nsINavHistoryObserver,
  ])
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([DummyObserver]);
}
