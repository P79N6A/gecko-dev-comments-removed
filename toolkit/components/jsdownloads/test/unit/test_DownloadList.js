








"use strict";











function promiseNewDownloadList() {
  
  Downloads._publicDownloadList = null;
  return Downloads.getPublicDownloadList();
}








function promiseNewPrivateDownloadList() {
  
  Downloads._privateDownloadList = null;
  return Downloads.getPrivateDownloadList();
}







add_task(function test_construction()
{
  let downloadListOne = yield promiseNewDownloadList();
  let downloadListTwo = yield promiseNewDownloadList();
  let privateDownloadListOne = yield promiseNewPrivateDownloadList();
  let privateDownloadListTwo = yield promiseNewPrivateDownloadList();

  do_check_neq(downloadListOne, downloadListTwo);
  do_check_neq(privateDownloadListOne, privateDownloadListTwo);
  do_check_neq(downloadListOne, privateDownloadListOne);
});




add_task(function test_add_getAll()
{
  let list = yield promiseNewDownloadList();

  let downloadOne = yield promiseSimpleDownload();
  list.add(downloadOne);

  let itemsOne = yield list.getAll();
  do_check_eq(itemsOne.length, 1);
  do_check_eq(itemsOne[0], downloadOne);

  let downloadTwo = yield promiseSimpleDownload();
  list.add(downloadTwo);

  let itemsTwo = yield list.getAll();
  do_check_eq(itemsTwo.length, 2);
  do_check_eq(itemsTwo[0], downloadOne);
  do_check_eq(itemsTwo[1], downloadTwo);

  
  do_check_eq(itemsOne.length, 1);
});




add_task(function test_remove()
{
  let list = yield promiseNewDownloadList();

  list.add(yield promiseSimpleDownload());
  list.add(yield promiseSimpleDownload());

  let items = yield list.getAll();
  list.remove(items[0]);

  
  list.remove(yield promiseSimpleDownload());

  items = yield list.getAll();
  do_check_eq(items.length, 1);
});





add_task(function test_notifications_add_remove()
{
  let list = yield promiseNewDownloadList();

  let downloadOne = yield promiseSimpleDownload();
  let downloadTwo = yield promiseSimpleDownload();
  list.add(downloadOne);
  list.add(downloadTwo);

  
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
  list.addView(viewOne);
  do_check_eq(addNotifications, 2);

  
  list.add(yield promiseSimpleDownload());
  do_check_eq(addNotifications, 3);

  
  let removeNotifications = 0;
  let viewTwo = {
    onDownloadRemoved: function (aDownload) {
      do_check_eq(aDownload, downloadOne);
      removeNotifications++;
    },
  };
  list.addView(viewTwo);
  list.remove(downloadOne);
  do_check_eq(removeNotifications, 1);

  
  list.removeView(viewTwo);
  list.remove(downloadTwo);
  do_check_eq(removeNotifications, 1);

  
  list.removeView(viewOne);
  list.add(yield promiseSimpleDownload());
  do_check_eq(addNotifications, 3);
});




add_task(function test_notifications_change()
{
  let list = yield promiseNewDownloadList();

  let downloadOne = yield promiseSimpleDownload();
  let downloadTwo = yield promiseSimpleDownload();
  list.add(downloadOne);
  list.add(downloadTwo);

  
  let receivedOnDownloadChanged = false;
  list.addView({
    onDownloadChanged: function (aDownload) {
      do_check_eq(aDownload, downloadOne);
      receivedOnDownloadChanged = true;
    },
  });
  yield downloadOne.start();
  do_check_true(receivedOnDownloadChanged);

  
  receivedOnDownloadChanged = false;
  list.remove(downloadTwo);
  yield downloadTwo.start();
  do_check_false(receivedOnDownloadChanged);
});




add_task(function test_history_expiration()
{
  function cleanup() {
    Services.prefs.clearUserPref("places.history.expiration.max_pages");
  }
  do_register_cleanup(cleanup);

  
  Services.prefs.setIntPref("places.history.expiration.max_pages", 0);

  
  yield promiseAddDownloadToHistory();
  yield promiseAddDownloadToHistory(TEST_INTERRUPTIBLE_URI);

  let list = yield promiseNewDownloadList();
  let downloadOne = yield promiseSimpleDownload();
  let downloadTwo = yield promiseSimpleDownload(TEST_INTERRUPTIBLE_URI);
  list.add(downloadOne);
  list.add(downloadTwo);

  let deferred = Promise.defer();
  let removeNotifications = 0;
  let downloadView = {
    onDownloadRemoved: function (aDownload) {
      if (++removeNotifications == 2) {
        deferred.resolve();
      }
    },
  };
  list.addView(downloadView);

  
  yield downloadOne.start();

  
  downloadTwo.start();
  let promiseCanceled = downloadTwo.cancel();

  
  let expire = Cc["@mozilla.org/places/expiration;1"]
                 .getService(Ci.nsIObserver);
  expire.observe(null, "places-debug-start-expiration", -1);

  yield deferred.promise;
  yield promiseCanceled;

  cleanup();
});




add_task(function test_history_clear()
{
  
  yield promiseAddDownloadToHistory();
  yield promiseAddDownloadToHistory();

  let list = yield promiseNewDownloadList();
  let downloadOne = yield promiseSimpleDownload();
  let downloadTwo = yield promiseSimpleDownload();
  list.add(downloadOne);
  list.add(downloadTwo);

  let deferred = Promise.defer();
  let removeNotifications = 0;
  let downloadView = {
    onDownloadRemoved: function (aDownload) {
      if (++removeNotifications == 2) {
        deferred.resolve();
      }
    },
  };
  list.addView(downloadView);

  yield downloadOne.start();
  yield downloadTwo.start();

  PlacesUtils.history.removeAllPages();

  yield deferred.promise;
});
