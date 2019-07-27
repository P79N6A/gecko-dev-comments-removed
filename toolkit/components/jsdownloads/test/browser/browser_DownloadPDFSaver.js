









"use strict";




function* test_download_windowRef(aTab, aDownload) {
  ok(aDownload.source.windowRef, "Download source had a window reference");
  ok(aDownload.source.windowRef instanceof Ci.xpcIJSWeakReference, "Download window reference is a weak ref");
  is(aDownload.source.windowRef.get(), aTab.linkedBrowser.contentWindow, "Download window exists during test");
}




function* test_download_state_complete(aTab, aDownload, aPrivate, aCanceled) {
  ok(aDownload.source, "Download has a source");
  is(aDownload.source.url, aTab.linkedBrowser.contentWindow.location, "Download source has correct url");
  is(aDownload.source.isPrivate, aPrivate, "Download source has correct private state");
  ok(aDownload.stopped, "Download is stopped");
  is(aCanceled, aDownload.canceled, "Download has correct canceled state");
  is(!aCanceled, aDownload.succeeded, "Download has correct succeeded state");
  is(aDownload.error, null, "Download error is not defined");
}

function* test_createDownload_common(aPrivate, aType) {
  let tab = gBrowser.addTab(getRootDirectory(gTestPath) + "testFile.html");
  yield promiseBrowserLoaded(tab.linkedBrowser);

  if (aPrivate) {
    tab.linkedBrowser.docShell.QueryInterface(Ci.nsILoadContext)
                              .usePrivateBrowsing = true;
  }

  let download = yield Downloads.createDownload({
    source: tab.linkedBrowser.contentWindow,
    target: { path: getTempFile(TEST_TARGET_FILE_NAME_PDF).path },
    saver: { type: aType },
  });

  yield test_download_windowRef(tab, download);
  yield download.start();

  yield test_download_state_complete(tab, download, aPrivate, false);
  if (aType == "pdf") {
    let signature = yield OS.File.read(download.target.path,
                                       { bytes: 4, encoding: "us-ascii" });
    is(signature, "%PDF", "File exists and signature matches");
  } else {
    ok((yield OS.File.exists(download.target.path)), "File exists");
  }

  gBrowser.removeTab(tab);
}

add_task(function* test_createDownload_pdf_private() {
  yield test_createDownload_common(true, "pdf");
});
add_task(function* test_createDownload_pdf_not_private() {
  yield test_createDownload_common(false, "pdf");
});


add_task(function* test_createDownload_copy_private() {
  yield test_createDownload_common(true, "copy");
});
add_task(function* test_createDownload_copy_not_private() {
  yield test_createDownload_common(false, "copy");
});

add_task(function* test_cancel_pdf_download() {
  let tab = gBrowser.addTab(getRootDirectory(gTestPath) + "testFile.html");
  yield promiseBrowserLoaded(tab.linkedBrowser);

  let download = yield Downloads.createDownload({
    source: tab.linkedBrowser.contentWindow,
    target: { path: getTempFile(TEST_TARGET_FILE_NAME_PDF).path },
    saver: "pdf",
  });

  yield test_download_windowRef(tab, download);
  download.start();

  
  yield download.cancel();
  yield test_download_state_complete(tab, download, false, true);

  let exists = yield OS.File.exists(download.target.path)
  ok(!exists, "Target file does not exist");

  gBrowser.removeTab(tab);
});
