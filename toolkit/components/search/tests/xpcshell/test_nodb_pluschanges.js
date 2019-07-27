




















function run_test() {
  removeMetadata();
  updateAppInfo();
  do_load_manifest("data/chrome.manifest");
  useHttpServer();

  run_next_test();
}

add_task(function* test_nodb_pluschanges() {
  let [engine1, engine2] = yield addTestEngines([
    { name: "Test search engine", xmlFileName: "engine.xml" },
    { name: "Sherlock test search engine", srcFileName: "engine.src",
      iconFileName: "ico-size-16x16-png.ico" },
  ]);

  let search = Services.search;

  search.moveEngine(engine1, 0);
  search.moveEngine(engine2, 1);

  
  do_print("Next step is forcing flush");
  yield new Promise(resolve => do_execute_soon(resolve));

  do_print("Forcing flush");
  let promiseCommit = new Promise(resolve => afterCommit(resolve));
  search.QueryInterface(Ci.nsIObserver)
        .observe(null, "quit-application", "");
  yield promiseCommit;
  do_print("Commit complete");

  
  let json = getSearchMetadata();
  do_check_eq(json["[app]/test-search-engine.xml"].order, 1);
  do_check_eq(json["[profile]/sherlock-test-search-engine.xml"].order, 2);

  do_print("Cleaning up");
  removeMetadata();
});
