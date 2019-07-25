Cu.import("resource://weave/engines/bookmarks.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/sharing.js");

Function.prototype.async = Async.sugar;

load("bookmark_setup.js");

function FakeMicrosummaryService() {
  return {hasMicrosummary: function() { return false; }};
}

function FakeAnnotationService() {
  this._annotations = {};
}
FakeAnnotationService.prototype = {
  EXPIRE_NEVER: 0,
  getItemAnnotation: function (aItemId, aName) {
    if (this._annotations[aItemId] != undefined)
      if (this._annotations[aItemId][aName])
	return this._annotations[aItemId][aName];
    return null;
  },
  setItemAnnotation: function (aItemId, aName, aValue, aFlags, aExpiration) {
    if (this._annotations[aItemId] == undefined)
      this._annotations[aItemId] = {};
    this._annotations[aItemId][aName] = aValue;
    dump( "Annotated item " + aItemId + " with " + aName + " = " + aValue + "\n");
    
  },
  getItemsWithAnnotation: function(aName, resultCount, results) {
    var list = [];
    for ( var x in this._annotations) {
      if (this._annotations[x][aName] != undefined) {
        return x;
      }
    }
    return list;
  }
}


function FakeSharingApi() {
}
FakeSharingApi.prototype = {
  shareWithUsers: function FakeSharingApi_shareWith(path, users, onComplete) {
    
  }
}
Sharing.Api = FakeSharingApi;


var annoSvc = new FakeAnnotationService();

function makeBookmarksEngine() {
  let engine = new BookmarksEngine();
  engine._store.__ms = new FakeMicrosummaryService();
  let shareManager = engine._sharing;
  shareManager.__annoSvc = annoSvc; 
  return engine;
}

function run_test() {
  let bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
    getService(Ci.nsINavBookmarksService);


  var syncTesting = new SyncTestingInfrastructure( makeBookmarksEngine );

  let folderName = "Funny Pictures of Manatees and Walruses";
  let folderToShare = bms.createFolder( bms.bookmarksMenuFolder,
					folderName, -1 );
  let lolrusBm = bms.insertBookmark(folderToShare,
				    uri("http://www.lolrus.com"),
				    -1, "LOLrus" );
  let lolateeBm = bms.insertBookmark(folderToShare,
				    uri("http://www.lolatee.com"),
				    -1, "LOLatee" );

  

  let username = "rusty";
  let engine = makeBookmarksEngine();


  

























}


