








"use strict";








add_task(function test_createDownload()
{
  
  yield Downloads.createDownload({
    source: { uri: NetUtil.newURI("about:blank") },
    target: { file: getTempFile(TEST_TARGET_FILE_NAME) },
    saver: { type: "copy" },
  });
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
