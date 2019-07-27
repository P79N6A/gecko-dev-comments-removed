











function run_test() {
  updateAppInfo();
  useHttpServer();

  run_next_test();
}

add_task(function test_batchTask() {
  let [engine1, engine2] = yield addTestEngines([
    { name: "Test search engine", xmlFileName: "engine.xml" },
    { name: "Sherlock test search engine", srcFileName: "engine.src",
      iconFileName: "ico-size-16x16-png.ico" },
  ]);

  
  let engineFile1 = engine1.wrappedJSObject._file;
  let engineFile2 = engine2.wrappedJSObject._file;
  do_check_true(engineFile1.exists());
  do_check_true(engineFile2.exists());
  do_check_neq(engineFile1.fileSize, 0);
  do_check_neq(engineFile2.fileSize, 0);
});

add_test(function test_addParam() {
  let engine = Services.search.getEngineByName("Test search engine");
  engine.addParam("param-name", "param-value", null);

  function readAsyncFile(aFile, aCallback) {
    NetUtil.asyncFetch(aFile, function(inputStream, status) {
      do_check_true(Components.isSuccessCode(status));

      let data = NetUtil.readInputStreamToString(inputStream, inputStream.available());
      aCallback(data);
    });
  }

  let observer = function(aSubject, aTopic, aData) {
    
    
    aSubject.QueryInterface(Ci.nsIFile);
    if (aTopic == "browser-search-service" &&
        aData == "write-engine-to-disk-complete" &&
        aSubject.leafName == "test-search-engine.xml") {
      Services.obs.removeObserver(observer, aTopic);

      let engineFile = engine.wrappedJSObject._file;

      readAsyncFile(engineFile, function(engineData) {
        do_check_true(engineData.indexOf("param-name") > 0);
        run_next_test();
      });
    }
  }
  Services.obs.addObserver(observer, "browser-search-service", false);
});
