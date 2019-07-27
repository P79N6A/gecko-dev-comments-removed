








"use strict";










function getExpirablePRTime()
{
  let dateObj = new Date();
  
  dateObj.setHours(0);
  dateObj.setMinutes(0);
  dateObj.setSeconds(0);
  dateObj.setMilliseconds(0);
  dateObj = new Date(dateObj.getTime() - 8 * 86400000);
  return dateObj.getTime() * 1000;
}











function promiseExpirableDownloadVisit(aSourceUrl)
{
  let deferred = Promise.defer();
  PlacesUtils.asyncHistory.updatePlaces(
    {
      uri: NetUtil.newURI(aSourceUrl || httpUrl("source.txt")),
      visits: [{
        transitionType: Ci.nsINavHistoryService.TRANSITION_DOWNLOAD,
        visitDate: getExpirablePRTime(),
      }]
    },
    {
      handleError: function handleError(aResultCode, aPlaceInfo) {
        let ex = new Components.Exception("Unexpected error in adding visits.",
                                          aResultCode);
        deferred.reject(ex);
      },
      handleResult: function () {},
      handleCompletion: function handleCompletion() {
        deferred.resolve();
      }
    });
  return deferred.promise;
}







add_task(function test_construction()
{
  let downloadListOne = yield promiseNewList();
  let downloadListTwo = yield promiseNewList();
  let privateDownloadListOne = yield promiseNewList(true);
  let privateDownloadListTwo = yield promiseNewList(true);

  do_check_neq(downloadListOne, downloadListTwo);
  do_check_neq(privateDownloadListOne, privateDownloadListTwo);
  do_check_neq(downloadListOne, privateDownloadListOne);
});




add_task(function test_add_getAll()
{
  let list = yield promiseNewList();

  let downloadOne = yield promiseNewDownload();
  yield list.add(downloadOne);

  let itemsOne = yield list.getAll();
  do_check_eq(itemsOne.length, 1);
  do_check_eq(itemsOne[0], downloadOne);

  let downloadTwo = yield promiseNewDownload();
  yield list.add(downloadTwo);

  let itemsTwo = yield list.getAll();
  do_check_eq(itemsTwo.length, 2);
  do_check_eq(itemsTwo[0], downloadOne);
  do_check_eq(itemsTwo[1], downloadTwo);

  
  do_check_eq(itemsOne.length, 1);
});




add_task(function test_remove()
{
  let list = yield promiseNewList();

  yield list.add(yield promiseNewDownload());
  yield list.add(yield promiseNewDownload());

  let items = yield list.getAll();
  yield list.remove(items[0]);

  
  yield list.remove(yield promiseNewDownload());

  items = yield list.getAll();
  do_check_eq(items.length, 1);
});






add_task(function test_DownloadCombinedList_add_remove_getAll()
{
  let publicList = yield promiseNewList();
  let privateList = yield Downloads.getList(Downloads.PRIVATE);
  let combinedList = yield Downloads.getList(Downloads.ALL);

  let publicDownload = yield promiseNewDownload();
  let privateDownload = yield Downloads.createDownload({
    source: { url: httpUrl("source.txt"), isPrivate: true },
    target: getTempFile(TEST_TARGET_FILE_NAME).path,
  });

  yield publicList.add(publicDownload);
  yield privateList.add(privateDownload);

  do_check_eq((yield combinedList.getAll()).length, 2);

  yield combinedList.remove(publicDownload);
  yield combinedList.remove(privateDownload);

  do_check_eq((yield combinedList.getAll()).length, 0);

  yield combinedList.add(publicDownload);
  yield combinedList.add(privateDownload);

  do_check_eq((yield publicList.getAll()).length, 1);
  do_check_eq((yield privateList.getAll()).length, 1);
  do_check_eq((yield combinedList.getAll()).length, 2);

  yield publicList.remove(publicDownload);
  yield privateList.remove(privateDownload);

  do_check_eq((yield combinedList.getAll()).length, 0);
});






add_task(function test_notifications_add_remove()
{
  for (let isCombined of [false, true]) {
    
    let list = yield promiseNewList();
    if (isCombined) {
      list = yield Downloads.getList(Downloads.ALL);
    }

    let downloadOne = yield promiseNewDownload();
    let downloadTwo = yield Downloads.createDownload({
      source: { url: httpUrl("source.txt"), isPrivate: true },
      target: getTempFile(TEST_TARGET_FILE_NAME).path,
    });
    yield list.add(downloadOne);
    yield list.add(downloadTwo);

    
    let addNotifications = 0;
    let viewOne = {
      onDownloadAdded: function (aDownload) {
        
        if (addNotifications == 0) {
          do_check_eq(aDownload, downloadOne);
        } else if (addNotifications == 1) {
          do_check_eq(aDownload, downloadTwo);
        }
        addNotifications++;
      },
    };
    yield list.addView(viewOne);
    do_check_eq(addNotifications, 2);

    
    yield list.add(yield promiseNewDownload());
    do_check_eq(addNotifications, 3);

    
    let removeNotifications = 0;
    let viewTwo = {
      onDownloadRemoved: function (aDownload) {
        do_check_eq(aDownload, downloadOne);
        removeNotifications++;
      },
    };
    yield list.addView(viewTwo);
    yield list.remove(downloadOne);
    do_check_eq(removeNotifications, 1);

    
    yield list.removeView(viewTwo);
    yield list.remove(downloadTwo);
    do_check_eq(removeNotifications, 1);

    
    yield list.removeView(viewOne);
    yield list.add(yield promiseNewDownload());
    do_check_eq(addNotifications, 3);
  }
});





add_task(function test_notifications_change()
{
  for (let isCombined of [false, true]) {
    
    let list = yield promiseNewList();
    if (isCombined) {
      list = yield Downloads.getList(Downloads.ALL);
    }

    let downloadOne = yield promiseNewDownload();
    let downloadTwo = yield Downloads.createDownload({
      source: { url: httpUrl("source.txt"), isPrivate: true },
      target: getTempFile(TEST_TARGET_FILE_NAME).path,
    });
    yield list.add(downloadOne);
    yield list.add(downloadTwo);

    
    let receivedOnDownloadChanged = false;
    yield list.addView({
      onDownloadChanged: function (aDownload) {
        do_check_eq(aDownload, downloadOne);
        receivedOnDownloadChanged = true;
      },
    });
    yield downloadOne.start();
    do_check_true(receivedOnDownloadChanged);

    
    receivedOnDownloadChanged = false;
    yield list.remove(downloadTwo);
    yield downloadTwo.start();
    do_check_false(receivedOnDownloadChanged);
  }
});




add_task(function test_notifications_this()
{
  let list = yield promiseNewList();

  
  let receivedOnDownloadAdded = false;
  let receivedOnDownloadChanged = false;
  let receivedOnDownloadRemoved = false;
  let view = {
    onDownloadAdded: function () {
      do_check_eq(this, view);
      receivedOnDownloadAdded = true;
    },
    onDownloadChanged: function () {
      
      if (!receivedOnDownloadChanged) {
        do_check_eq(this, view);
        receivedOnDownloadChanged = true;
      }
    },
    onDownloadRemoved: function () {
      do_check_eq(this, view);
      receivedOnDownloadRemoved = true;
    },
  };
  yield list.addView(view);

  let download = yield promiseNewDownload();
  yield list.add(download);
  yield download.start();
  yield list.remove(download);

  
  do_check_true(receivedOnDownloadAdded);
  do_check_true(receivedOnDownloadChanged);
  do_check_true(receivedOnDownloadRemoved);
});




add_task(function test_history_expiration()
{
  mustInterruptResponses();

  function cleanup() {
    Services.prefs.clearUserPref("places.history.expiration.max_pages");
  }
  do_register_cleanup(cleanup);

  
  Services.prefs.setIntPref("places.history.expiration.max_pages", 0);

  let list = yield promiseNewList();
  let downloadOne = yield promiseNewDownload();
  let downloadTwo = yield promiseNewDownload(httpUrl("interruptible.txt"));

  let deferred = Promise.defer();
  let removeNotifications = 0;
  let downloadView = {
    onDownloadRemoved: function (aDownload) {
      if (++removeNotifications == 2) {
        deferred.resolve();
      }
    },
  };
  yield list.addView(downloadView);

  
  yield downloadOne.start();
  downloadTwo.start();
  yield downloadTwo.cancel();

  
  
  yield PlacesTestUtils.clearHistory();
  yield promiseExpirableDownloadVisit();
  yield promiseExpirableDownloadVisit(httpUrl("interruptible.txt"));

  
  yield list.add(downloadOne);
  yield list.add(downloadTwo);

  
  Cc["@mozilla.org/places/expiration;1"]
    .getService(Ci.nsIObserver).observe(null, "places-debug-start-expiration", -1);

  
  yield deferred.promise;

  cleanup();
});




add_task(function test_history_clear()
{
  let list = yield promiseNewList();
  let downloadOne = yield promiseNewDownload();
  let downloadTwo = yield promiseNewDownload();
  yield list.add(downloadOne);
  yield list.add(downloadTwo);

  let deferred = Promise.defer();
  let removeNotifications = 0;
  let downloadView = {
    onDownloadRemoved: function (aDownload) {
      if (++removeNotifications == 2) {
        deferred.resolve();
      }
    },
  };
  yield list.addView(downloadView);

  yield downloadOne.start();
  yield downloadTwo.start();

  yield PlacesTestUtils.clearHistory();

  
  yield deferred.promise;
});





add_task(function test_removeFinished()
{
  let list = yield promiseNewList();
  let downloadOne = yield promiseNewDownload();
  let downloadTwo = yield promiseNewDownload();
  let downloadThree = yield promiseNewDownload();
  let downloadFour = yield promiseNewDownload();
  yield list.add(downloadOne);
  yield list.add(downloadTwo);
  yield list.add(downloadThree);
  yield list.add(downloadFour);

  let deferred = Promise.defer();
  let removeNotifications = 0;
  let downloadView = {
    onDownloadRemoved: function (aDownload) {
      do_check_true(aDownload == downloadOne ||
                    aDownload == downloadTwo ||
                    aDownload == downloadThree);
      do_check_true(removeNotifications < 3);
      if (++removeNotifications == 3) {
        deferred.resolve();
      }
    },
  };
  yield list.addView(downloadView);

  
  
  
  yield downloadOne.start();
  yield downloadThree.start();
  yield downloadFour.start();
  downloadFour.hasPartialData = true;

  list.removeFinished();
  yield deferred.promise;

  let downloads = yield list.getAll()
  do_check_eq(downloads.length, 1);
});





add_task(function test_DownloadSummary()
{
  mustInterruptResponses();

  let publicList = yield promiseNewList();
  let privateList = yield Downloads.getList(Downloads.PRIVATE);

  let publicSummary = yield Downloads.getSummary(Downloads.PUBLIC);
  let privateSummary = yield Downloads.getSummary(Downloads.PRIVATE);
  let combinedSummary = yield Downloads.getSummary(Downloads.ALL);

  
  let succeededPublicDownload = yield promiseNewDownload();
  yield succeededPublicDownload.start();
  yield publicList.add(succeededPublicDownload);

  
  let canceledPublicDownload =
      yield promiseNewDownload(httpUrl("interruptible.txt"));
  canceledPublicDownload.start();
  yield promiseDownloadMidway(canceledPublicDownload);
  yield canceledPublicDownload.cancel();
  yield publicList.add(canceledPublicDownload);

  
  let inProgressPublicDownload =
      yield promiseNewDownload(httpUrl("interruptible.txt"));
  inProgressPublicDownload.start();
  yield promiseDownloadMidway(inProgressPublicDownload);
  yield publicList.add(inProgressPublicDownload);

  
  let inProgressPrivateDownload = yield Downloads.createDownload({
    source: { url: httpUrl("interruptible.txt"), isPrivate: true },
    target: getTempFile(TEST_TARGET_FILE_NAME).path,
  });
  inProgressPrivateDownload.start();
  yield promiseDownloadMidway(inProgressPrivateDownload);
  yield privateList.add(inProgressPrivateDownload);

  
  
  
  
  
  
  do_check_false(publicSummary.allHaveStopped);
  do_check_eq(publicSummary.progressTotalBytes, TEST_DATA_SHORT.length * 2);
  do_check_eq(publicSummary.progressCurrentBytes, TEST_DATA_SHORT.length);

  do_check_false(privateSummary.allHaveStopped);
  do_check_eq(privateSummary.progressTotalBytes, TEST_DATA_SHORT.length * 2);
  do_check_eq(privateSummary.progressCurrentBytes, TEST_DATA_SHORT.length);

  do_check_false(combinedSummary.allHaveStopped);
  do_check_eq(combinedSummary.progressTotalBytes, TEST_DATA_SHORT.length * 4);
  do_check_eq(combinedSummary.progressCurrentBytes, TEST_DATA_SHORT.length * 2);

  yield inProgressPublicDownload.cancel();

  
  do_check_true(publicSummary.allHaveStopped);
  do_check_eq(publicSummary.progressTotalBytes, 0);
  do_check_eq(publicSummary.progressCurrentBytes, 0);

  do_check_false(privateSummary.allHaveStopped);
  do_check_eq(privateSummary.progressTotalBytes, TEST_DATA_SHORT.length * 2);
  do_check_eq(privateSummary.progressCurrentBytes, TEST_DATA_SHORT.length);

  do_check_false(combinedSummary.allHaveStopped);
  do_check_eq(combinedSummary.progressTotalBytes, TEST_DATA_SHORT.length * 2);
  do_check_eq(combinedSummary.progressCurrentBytes, TEST_DATA_SHORT.length);

  yield inProgressPrivateDownload.cancel();

  
  do_check_true(publicSummary.allHaveStopped);
  do_check_eq(publicSummary.progressTotalBytes, 0);
  do_check_eq(publicSummary.progressCurrentBytes, 0);

  do_check_true(privateSummary.allHaveStopped);
  do_check_eq(privateSummary.progressTotalBytes, 0);
  do_check_eq(privateSummary.progressCurrentBytes, 0);

  do_check_true(combinedSummary.allHaveStopped);
  do_check_eq(combinedSummary.progressTotalBytes, 0);
  do_check_eq(combinedSummary.progressCurrentBytes, 0);
});






add_task(function test_DownloadSummary_notifications()
{
  let list = yield promiseNewList();
  let summary = yield Downloads.getSummary(Downloads.ALL);

  let download = yield promiseNewDownload();
  yield list.add(download);

  
  let receivedOnSummaryChanged = false;
  yield summary.addView({
    onSummaryChanged: function () {
      receivedOnSummaryChanged = true;
    },
  });
  yield download.start();
  do_check_true(receivedOnSummaryChanged);
});




let tailFile = do_get_file("tail.js");
Services.scriptloader.loadSubScript(NetUtil.newURI(tailFile).spec);
