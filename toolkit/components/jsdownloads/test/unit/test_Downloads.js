








"use strict";








add_task(function test_createDownload()
{
  
  yield Downloads.createDownload({
    source: { uri: NetUtil.newURI("about:blank") },
    target: { file: getTempFile(TEST_TARGET_FILE_NAME) },
    saver: { type: "copy" },
  });
});




add_task(function test_createDownload_private()
{
  let download = yield Downloads.createDownload({
    source: { uri: NetUtil.newURI("about:blank"),
              isPrivate: true },
    target: { file: getTempFile(TEST_TARGET_FILE_NAME) },
    saver: { type: "copy" }
  });
  do_check_true(download.source.isPrivate);
});




add_task(function test_createDownload_public()
{
  let uri = NetUtil.newURI("about:blank");
  let tempFile = getTempFile(TEST_TARGET_FILE_NAME);
  let download = yield Downloads.createDownload({
    source: { uri: uri, isPrivate: false },
    target: { file: tempFile },
    saver: { type: "copy" }
  });
  do_check_false(download.source.isPrivate);

  download = yield Downloads.createDownload({
    source: { uri: uri },
    target: { file: tempFile },
    saver: { type: "copy" }
  });
  do_check_true(!download.source.isPrivate);
});




add_task(function test_simpleDownload_uri_file_arguments()
{
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);
  yield Downloads.simpleDownload(TEST_SOURCE_URI, targetFile);
  yield promiseVerifyContents(targetFile, TEST_DATA_SHORT);
});




add_task(function test_simpleDownload_object_arguments()
{
  let targetFile = getTempFile(TEST_TARGET_FILE_NAME);
  yield Downloads.simpleDownload({ uri: TEST_SOURCE_URI },
                                 { file: targetFile });
  yield promiseVerifyContents(targetFile, TEST_DATA_SHORT);
});






add_task(function test_getPublicDownloadList()
{
  let downloadListOne = yield Downloads.getPublicDownloadList();
  let downloadListTwo = yield Downloads.getPublicDownloadList();

  do_check_eq(downloadListOne, downloadListTwo);
});






add_task(function test_getPrivateDownloadList()
{
  let downloadListOne = yield Downloads.getPrivateDownloadList();
  let downloadListTwo = yield Downloads.getPrivateDownloadList();

  do_check_eq(downloadListOne, downloadListTwo);
});






add_task(function test_public_and_private_lists_differ()
{
  let publicDownloadList = yield Downloads.getPublicDownloadList();
  let privateDownloadList = yield Downloads.getPrivateDownloadList();

  do_check_neq(publicDownloadList, privateDownloadList);
});





add_task(function test_getSystemDownloadsDirectory()
{
  let downloadDir = yield Downloads.getSystemDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
});





add_task(function test_getUserDownloadsDirectory()
{
  let downloadDir = yield Downloads.getUserDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
});





add_task(function test_getTemporaryDownloadsDirectory()
{
  let downloadDir = yield Downloads.getTemporaryDownloadsDirectory();
  do_check_true(downloadDir instanceof Ci.nsIFile);
});
