




































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


let (commonFile = do_get_file("../head_common.js", false)) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}






const FAVICON_ERRORPAGE_URI =
  NetUtil.newURI("chrome://global/skin/icons/warning-16.png");














function waitForFaviconChanged(aExpectedPageURI, aExpectedFaviconURI,
                               aCallback) {
  let historyObserver = {
    __proto__: NavHistoryObserver.prototype,
    onPageChanged: function WFFC_onPageChanged(aURI, aWhat, aValue, aGUID) {
      if (aWhat != Ci.nsINavHistoryObserver.ATTRIBUTE_FAVICON) {
        return;
      }
      PlacesUtils.history.removeObserver(this);

      do_check_true(aURI.equals(aExpectedPageURI));
      do_check_eq(aValue, aExpectedFaviconURI.spec);
      do_check_guid_for_uri(aURI, aGUID);
      aCallback();
    }
  };
  PlacesUtils.history.addObserver(historyObserver, false);
}













function checkFaviconDataForPage(aPageURI, aExpectedMimeType, aExpectedData,
                                 aCallback) {
  PlacesUtils.favicons.getFaviconDataForPage(aPageURI,
    function CFDFP_onFaviconDataAvailable(aURI, aDataLen, aData, aMimeType) {
      do_check_eq(aExpectedMimeType, aMimeType);
      do_check_true(compareArrays(aExpectedData, aData));
      do_check_guid_for_uri(aPageURI);
      aCallback();
    });
}









function checkFaviconMissingForPage(aPageURI, aCallback) {
  
  let notificationReceived = false;
  PlacesUtils.favicons.getFaviconURLForPage(aPageURI,
    function CFMFP_onFaviconDataAvailable() {
      notificationReceived = true;
    });

  
  
  
  
  waitForAsyncUpdates(function CFMFP_asyncUpdates() {
    do_execute_soon(function CFMFP_soon() {
      do_check_false(notificationReceived);
      aCallback();
    });
  });
}
