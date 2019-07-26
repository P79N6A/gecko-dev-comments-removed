








"use strict";








add_task(function test_createDownload()
{
  
  yield Downloads.createDownload({
    source: { url: "about:blank" },
    target: { path: getTempFile(TEST_TARGET_FILE_NAME).path },
    saver: { type: "copy" },
  });
});




add_task(function test_createDownload_private()
{
  let download = yield Downloads.createDownload({
    source: { url: "about:blank", isPrivate: true },
    target: { path: getTempFile(TEST_TARGET_FILE_NAME).path },
    saver: { type: "copy" }
  });
  do_check_true(download.source.isPrivate);
});




add_task(function test_createDownload_public()
{
  let tempPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  let download = yield Downloads.createDownload({
    source: { url: "about:blank", isPrivate: false },
    target: { path: tempPath },
    saver: { type: "copy" }
  });
  do_check_false(download.source.isPrivate);

  download = yield Downloads.createDownload({
    source: { url: "about:blank" },
    target: { path: tempPath },
    saver: { type: "copy" }
  });
  do_check_false(download.source.isPrivate);
});




add_task(function test_fetch_uri_file_arguments()
{
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);
  yield Downloads.fetch(NetUtil.newURI(httpUrl("source.txt")), targetFile);
  yield promiseVerifyContents(targetFile.path, TEST_DATA_SHORT);
});




add_task(function test_fetch_object_arguments()
{
  let targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  yield Downloads.fetch({ url: httpUrl("source.txt") }, { path: targetPath });
  yield promiseVerifyContents(targetPath, TEST_DATA_SHORT);
});




add_task(function test_fetch_string_arguments()
{
  let targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  yield Downloads.fetch(httpUrl("source.txt"), targetPath);
  yield promiseVerifyContents(targetPath, TEST_DATA_SHORT);

  targetPath = getTempFile(TEST_TARGET_FILE_NAME).path;
  yield Downloads.fetch(new String(httpUrl("source.txt")),
                        new String(targetPath));
  yield promiseVerifyContents(targetPath, TEST_DATA_SHORT);
});







add_task(function test_getList()
{
  let publicListOne = yield Downloads.getList(Downloads.PUBLIC);
  let privateListOne = yield Downloads.getList(Downloads.PRIVATE);

  let publicListTwo = yield Downloads.getList(Downloads.PUBLIC);
  let privateListTwo = yield Downloads.getList(Downloads.PRIVATE);

  do_check_eq(publicListOne, publicListTwo);
  do_check_eq(privateListOne, privateListTwo);

  do_check_neq(publicListOne, privateListOne);
});







add_task(function test_getSummary()
{
  let publicSummaryOne = yield Downloads.getSummary(Downloads.PUBLIC);
  let privateSummaryOne = yield Downloads.getSummary(Downloads.PRIVATE);

  let publicSummaryTwo = yield Downloads.getSummary(Downloads.PUBLIC);
  let privateSummaryTwo = yield Downloads.getSummary(Downloads.PRIVATE);

  do_check_eq(publicSummaryOne, publicSummaryTwo);
  do_check_eq(privateSummaryOne, privateSummaryTwo);

  do_check_neq(publicSummaryOne, privateSummaryOne);
});





add_task(function test_getSystemDownloadsDirectory()
{
  let downloadDir = yield Downloads.getSystemDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
});





add_task(function test_getPreferredDownloadsDirectory()
{
  let downloadDir = yield Downloads.getPreferredDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
});





add_task(function test_getTemporaryDownloadsDirectory()
{
  let downloadDir = yield Downloads.getTemporaryDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
});




let tailFile = do_get_file("tail.js");
Services.scriptloader.loadSubScript(NetUtil.newURI(tailFile).spec);
