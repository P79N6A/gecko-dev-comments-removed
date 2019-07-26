

const TRANSITION_LINK = Ci.nsINavHistoryService.TRANSITION_LINK;
const TRANSITION_TYPED = Ci.nsINavHistoryService.TRANSITION_TYPED;

Components.utils.import("resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");











function promiseTopicObserved(aTopic)
{
  let deferred = Promise.defer();

  Services.obs.addObserver(
    function PTO_observe(aSubject, aTopic, aData) {
      Services.obs.removeObserver(PTO_observe, aTopic);
      deferred.resolve([aSubject, aData]);
    }, aTopic, false);

  return deferred.promise;
}








function promiseClearHistory() {
  let promise = promiseTopicObserved(PlacesUtils.TOPIC_EXPIRATION_FINISHED);
  PlacesUtils.bhistory.removeAllPages();
  return promise;
}














function promiseAsyncUpdates()
{
  let deferred = Promise.defer();

  let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                              .DBConnection;
  let begin = db.createAsyncStatement("BEGIN EXCLUSIVE");
  begin.executeAsync();
  begin.finalize();

  let commit = db.createAsyncStatement("COMMIT");
  commit.executeAsync({
    handleResult: function() {},
    handleError: function() {},
    handleCompletion: function(aReason)
    {
      deferred.resolve();
    }
  });
  commit.finalize();

  return deferred.promise;
}









function fieldForUrl(aURI, aFieldName, aCallback)
{
  let url = aURI instanceof Ci.nsIURI ? aURI.spec : aURI;
  let stmt = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                                .DBConnection.createAsyncStatement(
    "SELECT " + aFieldName + " FROM moz_places WHERE url = :page_url"
  );
  stmt.params.page_url = url;
  stmt.executeAsync({
    _value: -1,
    handleResult: function(aResultSet) {
      let row = aResultSet.getNextRow();
      if (!row)
        ok(false, "The page should exist in the database");
      this._value = row.getResultByName(aFieldName);
    },
    handleError: function() {},
    handleCompletion: function(aReason) {
      if (aReason != Ci.mozIStorageStatementCallback.REASON_FINISHED)
         ok(false, "The statement should properly succeed");
      aCallback(this._value);
    }
  });
  stmt.finalize();
}





function NavHistoryObserver() {}

NavHistoryObserver.prototype = {
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onVisit: function () {},
  onTitleChanged: function () {},
  onBeforeDeleteURI: function () {},
  onDeleteURI: function () {},
  onClearHistory: function () {},
  onPageChanged: function () {},
  onDeleteVisits: function () {},
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavHistoryObserver,
  ])
};














function waitForFaviconChanged(aExpectedPageURI, aExpectedFaviconURI, aWindow,
                               aCallback) {
  let historyObserver = {
    __proto__: NavHistoryObserver.prototype,
    onPageChanged: function WFFC_onPageChanged(aURI, aWhat, aValue, aGUID) {
      if (aWhat != Ci.nsINavHistoryObserver.ATTRIBUTE_FAVICON) {
        return;
      }
      aWindow.PlacesUtils.history.removeObserver(this);

      ok(aURI.equals(aExpectedPageURI),
        "Check URIs are equal for the page which favicon changed");
      is(aValue, aExpectedFaviconURI.spec,
        "Check changed favicon URI is the expected");
      checkGuidForURI(aURI, aGUID);

      if (aCallback) {
        aCallback();
      }
    }
  };
  aWindow.PlacesUtils.history.addObserver(historyObserver, false);
}



















function addVisits(aPlaceInfo, aWindow, aCallback, aStack) {
  let stack = aStack || Components.stack.caller;
  let places = [];
  if (aPlaceInfo instanceof Ci.nsIURI) {
    places.push({ uri: aPlaceInfo });
  }
  else if (Array.isArray(aPlaceInfo)) {
    places = places.concat(aPlaceInfo);
  } else {
    places.push(aPlaceInfo)
  }

  
  let now = Date.now();
  for (let i = 0; i < places.length; i++) {
    if (!places[i].title) {
      places[i].title = "test visit for " + places[i].uri.spec;
    }
    places[i].visits = [{
      transitionType: places[i].transition === undefined ? TRANSITION_LINK
                                                         : places[i].transition,
      visitDate: places[i].visitDate || (now++) * 1000,
      referrerURI: places[i].referrer
    }];
  }

  aWindow.PlacesUtils.asyncHistory.updatePlaces(
    places,
    {
      handleError: function AAV_handleError() {
        throw("Unexpected error in adding visit.");
      },
      handleResult: function () {},
      handleCompletion: function UP_handleCompletion() {
        if (aCallback)
          aCallback();
      }
    }
  );
}













function checkFaviconDataForPage(aPageURI, aExpectedMimeType, aExpectedData,
  aWindow, aCallback) {
  aWindow.PlacesUtils.favicons.getFaviconDataForPage(aPageURI,
    function (aURI, aDataLen, aData, aMimeType) {
      is(aExpectedMimeType, aMimeType, "Check expected MimeType");
      is(aExpectedData.length, aData.length,
        "Check favicon data for the given page matches the provided data");
      checkGuidForURI(aPageURI);
      aCallback();
    });
}









function checkGuidForURI(aURI, aGUID) {
  let guid = doGetGuidForURI(aURI);
  if (aGUID) {
    doCheckValidPlacesGuid(aGUID);
    is(guid, aGUID, "Check equal guid for URIs");
  }
}








function doGetGuidForURI(aURI) {
  let stmt = DBConn().createStatement(
    "SELECT guid "
    + "FROM moz_places "
    + "WHERE url = :url "
  );
  stmt.params.url = aURI.spec;
  ok(stmt.executeStep(), "Check get guid for uri from moz_places");
  let guid = stmt.row.guid;
  stmt.finalize();
  doCheckValidPlacesGuid(guid);
  return guid;
}







function doCheckValidPlacesGuid(aGuid) {
  ok(/^[a-zA-Z0-9\-_]{12}$/.test(aGuid), "Check guid for valid places");
}












function DBConn(aForceNewConnection) {
  let gDBConn;
  if (!aForceNewConnection) {
    let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
      .DBConnection;
    if (db.connectionReady)
      return db;
  }

  
  if (!gDBConn || aForceNewConnection) {
    let file = Services.dirsvc.get('ProfD', Ci.nsIFile);
    file.append("places.sqlite");
    let dbConn = gDBConn = Services.storage.openDatabase(file);

    
    Services.obs.addObserver(function DBCloseCallback(aSubject, aTopic, aData) {
      Services.obs.removeObserver(DBCloseCallback, aTopic);
      dbConn.asyncClose();
    }, "profile-before-change", false);
  }

  return gDBConn.connectionReady ? gDBConn : null;
}

function whenNewWindowLoaded(aOptions, aCallback) {
  let win = OpenBrowserWindow(aOptions);
  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);
    aCallback(win);
  }, false);
}
