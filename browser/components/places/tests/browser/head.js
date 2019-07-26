


Components.utils.import("resource://gre/modules/NetUtil.jsm");


let cachedLeftPaneFolderIdGetter;
let (getter = PlacesUIUtils.__lookupGetter__("leftPaneFolderId")) {
  if (!cachedLeftPaneFolderIdGetter && typeof(getter) == "function")
    cachedLeftPaneFolderIdGetter = getter;
}

registerCleanupFunction(function(){
  let (getter = PlacesUIUtils.__lookupGetter__("leftPaneFolderId")) {
    if (cachedLeftPaneFolderIdGetter && typeof(getter) != "function")
      PlacesUIUtils.__defineGetter__("leftPaneFolderId",
                                     cachedLeftPaneFolderIdGetter);
  }
});

function openLibrary(callback, aLeftPaneRoot) {
  let library = window.openDialog("chrome://browser/content/places/places.xul",
                                  "", "chrome,toolbar=yes,dialog=no,resizable",
                                  aLeftPaneRoot);
  waitForFocus(function () {
    callback(library);
  }, library);

  return library;
}








function waitForClearHistory(aCallback) {
  Services.obs.addObserver(function observeCH(aSubject, aTopic, aData) {
    Services.obs.removeObserver(observeCH, PlacesUtils.TOPIC_EXPIRATION_FINISHED);
    aCallback();
  }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);
  PlacesUtils.bhistory.removeAllPages();
}


















function waitForAsyncUpdates(aCallback, aScope, aArguments)
{
  let scope = aScope || this;
  let args = aArguments || [];
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
      aCallback.apply(scope, args);
    }
  });
  commit.finalize();
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
      transitionType: places[i].transition === undefined ? Ci.nsINavHistoryService.TRANSITION_LINK
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

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/commonjs/sdk/core/promise.js");

