









XPCOMUtils.defineLazyServiceGetter(this, "gDownloadHistory",
                                   "@mozilla.org/browser/download-history;1",
                                   "nsIDownloadHistory");

XPCOMUtils.defineLazyServiceGetter(this, "gHistory",
                                   "@mozilla.org/browser/history;1",
                                   "mozIAsyncHistory");

const DOWNLOAD_URI = NetUtil.newURI("http://www.example.com/");
const REFERRER_URI = NetUtil.newURI("http://www.example.org/");
const PRIVATE_URI = NetUtil.newURI("http://www.example.net/");







function waitForOnVisit(aCallback) {
  let historyObserver = {
    __proto__: NavHistoryObserver.prototype,
    onVisit: function HO_onVisit() {
      PlacesUtils.history.removeObserver(this);
      aCallback.apply(null, arguments);
    }
  };
  PlacesUtils.history.addObserver(historyObserver, false);
}









function uri_in_db(aURI, aExpected)
{
  let options = PlacesUtils.history.getNewQueryOptions();
  options.maxResults = 1;
  options.includeHidden = true;

  let query = PlacesUtils.history.getNewQuery();
  query.uri = aURI;

  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;

  do_check_eq(root.childCount, aExpected ? 1 : 0);

  
  root.containerOpen = false;
}




function run_test()
{
  run_next_test();
}

add_test(function test_dh_is_from_places()
{
  
  do_check_true(gDownloadHistory instanceof Ci.mozIAsyncHistory);

  waitForClearHistory(run_next_test);
});

add_test(function test_dh_addDownload()
{
  waitForOnVisit(function DHAD_onVisit(aURI) {
    do_check_true(aURI.equals(DOWNLOAD_URI));

    
    uri_in_db(DOWNLOAD_URI, true);

    waitForClearHistory(run_next_test);
  });

  gDownloadHistory.addDownload(DOWNLOAD_URI, null, Date.now() * 1000);
});

add_test(function test_dh_addDownload_referrer()
{
  waitForOnVisit(function DHAD_prepareReferrer(aURI, aVisitID) {
    do_check_true(aURI.equals(REFERRER_URI));
    let referrerVisitId = aVisitID;

    waitForOnVisit(function DHAD_onVisit(aURI, aVisitID, aTime, aSessionID,
                                              aReferringID) {
      do_check_true(aURI.equals(DOWNLOAD_URI));
      do_check_eq(aReferringID, referrerVisitId);

      
      uri_in_db(DOWNLOAD_URI, true);

      waitForClearHistory(run_next_test);
    });

    gDownloadHistory.addDownload(DOWNLOAD_URI, REFERRER_URI, Date.now() * 1000);
  });

  
  
  gHistory.updatePlaces({
    uri: REFERRER_URI,
    visits: [{
      transitionType: Ci.nsINavHistoryService.TRANSITION_TYPED,
      visitDate: Date.now() * 1000
    }]
  });
});

add_test(function test_dh_addDownload_privateBrowsing()
{
  if (!("@mozilla.org/privatebrowsing;1" in Cc)) {
    todo(false, "PB service is not available, bail out");
    run_next_test();
    return;
  }

  waitForOnVisit(function DHAD_onVisit(aURI) {
    
    
    
    
    do_check_true(aURI.equals(DOWNLOAD_URI));

    uri_in_db(DOWNLOAD_URI, true);
    uri_in_db(PRIVATE_URI, false);

    waitForClearHistory(run_next_test);
  });

  let pb = Cc["@mozilla.org/privatebrowsing;1"]
           .getService(Ci.nsIPrivateBrowsingService);
  Services.prefs.setBoolPref("browser.privatebrowsing.keep_current_session",
                             true);
  pb.privateBrowsingEnabled = true;
  gDownloadHistory.addDownload(PRIVATE_URI, REFERRER_URI, Date.now() * 1000);

  
  
  pb.privateBrowsingEnabled = false;
  Services.prefs.clearUserPref("browser.privatebrowsing.keep_current_session");
  gDownloadHistory.addDownload(DOWNLOAD_URI, REFERRER_URI, Date.now() * 1000);
});

add_test(function test_dh_addDownload_disabledHistory()
{
  waitForOnVisit(function DHAD_onVisit(aURI) {
    
    
    
    
    do_check_true(aURI.equals(DOWNLOAD_URI));

    uri_in_db(DOWNLOAD_URI, true);
    uri_in_db(PRIVATE_URI, false);

    waitForClearHistory(run_next_test);
  });

  Services.prefs.setBoolPref("places.history.enabled", false);
  gDownloadHistory.addDownload(PRIVATE_URI, REFERRER_URI, Date.now() * 1000);

  
  
  Services.prefs.clearUserPref("places.history.enabled");
  gDownloadHistory.addDownload(DOWNLOAD_URI, REFERRER_URI, Date.now() * 1000);
});





add_test(function test_dh_details()
{
  const REMOTE_URI = NetUtil.newURI("http://localhost/");
  const SOURCE_URI = NetUtil.newURI("http://example.com/test_dh_details");
  const DEST_FILE_NAME = "dest.txt";

  
  let destFileUri = NetUtil.newURI(FileUtils.getFile("TmpD", [DEST_FILE_NAME]));

  let titleSet = false;
  let destinationFileUriSet = false;
  let destinationFileNameSet = false;

  function checkFinished()
  {
    if (titleSet && destinationFileUriSet && destinationFileNameSet) {
      PlacesUtils.annotations.removeObserver(annoObserver);
      PlacesUtils.history.removeObserver(historyObserver);

      waitForClearHistory(run_next_test);
    }
  };

  let annoObserver = {
    onPageAnnotationSet: function AO_onPageAnnotationSet(aPage, aName)
    {
      if (aPage.equals(SOURCE_URI)) {
        let value = PlacesUtils.annotations.getPageAnnotation(aPage, aName);
        switch (aName)
        {
          case "downloads/destinationFileURI":
            destinationFileUriSet = true;
            do_check_eq(value, destFileUri.spec);
            break;
          case "downloads/destinationFileName":
            destinationFileNameSet = true;
            do_check_eq(value, DEST_FILE_NAME);
            break;
        }
        checkFinished();
      }
    },
    onItemAnnotationSet: function() {},
    onPageAnnotationRemoved: function() {},
    onItemAnnotationRemoved: function() {}
  }

  let historyObserver = {
    onBeginUpdateBatch: function() {},
    onEndUpdateBatch: function() {},
    onVisit: function() {},
    onTitleChanged: function HO_onTitleChanged(aURI, aPageTitle)
    {
      if (aURI.equals(SOURCE_URI)) {
        titleSet = true;
        do_check_eq(aPageTitle, DEST_FILE_NAME);
        checkFinished();
      }
    },
    onBeforeDeleteURI: function() {},
    onDeleteURI: function() {},
    onClearHistory: function() {},
    onPageChanged: function() {},
    onDeleteVisits: function() {}
  };

  PlacesUtils.annotations.addObserver(annoObserver, false);
  PlacesUtils.history.addObserver(historyObserver, false);

  
  gDownloadHistory.addDownload(SOURCE_URI, null, Date.now() * 1000);
  gDownloadHistory.addDownload(SOURCE_URI, null, Date.now() * 1000, null);
  gDownloadHistory.addDownload(SOURCE_URI, null, Date.now() * 1000, REMOTE_URI);

  
  gDownloadHistory.addDownload(SOURCE_URI, null, Date.now() * 1000,
                               destFileUri);
});
