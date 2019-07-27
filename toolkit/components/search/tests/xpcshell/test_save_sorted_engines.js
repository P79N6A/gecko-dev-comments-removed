


















function run_test() {
  removeMetadata();
  updateAppInfo();
  useHttpServer();

  run_next_test();
}

add_task(function* test_save_sorted_engines() {
  let [engine1, engine2] = yield addTestEngines([
    { name: "Test search engine", xmlFileName: "engine.xml" },
    { name: "Sherlock test search engine", srcFileName: "engine.src",
      iconFileName: "ico-size-16x16-png.ico" },
  ]);

  let search = Services.search;

  
  search.moveEngine(engine1, 0);
  search.moveEngine(engine2, 1);

  
  yield new Promise(resolve => afterCommit(resolve));
  do_print("Commit complete after moveEngine");

  
  let json = getSearchMetadata();
  do_check_eq(json["[app]/test-search-engine.xml"].order, 1);
  do_check_eq(json["[profile]/sherlock-test-search-engine.xml"].order, 2);

  
  search.removeEngine(engine1);
  yield new Promise(resolve => afterCommit(resolve));
  do_print("Commit complete after removeEngine");

  
  json = getSearchMetadata();
  do_check_eq(json["[profile]/sherlock-test-search-engine.xml"].order, 1);

  
  search.addEngineWithDetails("foo", "", "foo", "", "GET",
                              "http://searchget/?search={searchTerms}");
  yield new Promise(resolve => afterCommit(resolve));
  do_print("Commit complete after addEngineWithDetails");

  json = getSearchMetadata();
  do_check_eq(json["[profile]/foo.xml"].alias, "foo");
  do_check_true(json["[profile]/foo.xml"].order > 0);

  do_print("Cleaning up");
  removeMetadata();
});
