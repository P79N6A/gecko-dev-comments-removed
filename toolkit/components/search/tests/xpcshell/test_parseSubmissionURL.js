






"use strict";

function run_test() {
  removeMetadata();
  updateAppInfo();
  useHttpServer();

  run_next_test();
}

add_task(function* test_parseSubmissionURL() {
  
  for (let engine of Services.search.getEngines()) {
    Services.search.removeEngine(engine);
  }

  let [engine1, engine2, engine3] = yield addTestEngines([
    { name: "Test search engine", xmlFileName: "engine.xml" },
    { name: "Test search engine (fr)", xmlFileName: "engine-fr.xml" },
    { name: "bacon_addParam", details: ["", "bacon_addParam", "Search Bacon",
                                        "GET", "http://www.bacon.test/find"] },
    
    { name: "A second test engine", xmlFileName: "engine2.xml" },
    { name: "Sherlock test search engine", srcFileName: "engine.src",
      iconFileName: "ico-size-16x16-png.ico" },
    { name: "bacon", details: ["", "bacon", "Search Bacon", "GET",
                               "http://www.bacon.moz/search?q={searchTerms}"] },
  ]);

  engine3.addParam("q", "{searchTerms}", null);

  
  let result = Services.search.parseSubmissionURL(
                               "http://www.google.com/search?q=caff%C3%A8");
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, "caff\u00E8");

  
  
  
  let result = Services.search.parseSubmissionURL(
                               "http://www.google.fr/search?q=caff%E8");
  do_check_eq(result.engine, engine2);
  do_check_eq(result.terms, "caff\u00E8");

  
  
  let result = Services.search.parseSubmissionURL(
                               "http://www.google.co.uk/search?q=caff%C3%A8");
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, "caff\u00E8");

  
  
  let result = Services.search.parseSubmissionURL(
                               "http://www.bacon.test/find?q=caff%E8");
  do_check_eq(result.engine, engine3);
  do_check_eq(result.terms, "caff\u00E8");

  
  do_check_eq(Services.search.parseSubmissionURL(
                              "http://www.bacon.moz/search?q=").engine, null);
  do_check_eq(Services.search.parseSubmissionURL(
                              "https://duckduckgo.com?q=test").engine, null);
  do_check_eq(Services.search.parseSubmissionURL(
                              "https://duckduckgo.com/?q=test").engine, null);

  
  do_check_eq(Services.search.parseSubmissionURL(
                              "http://getfirefox.com?q=test").engine, null);
  do_check_eq(Services.search.parseSubmissionURL(
                              "http://getfirefox.com/?q=test").engine, null);

  
  let result = Services.search.parseSubmissionURL(
                               "https://www.google.com/search?q=caff%C3%A8");
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, "caff\u00E8");

  
  let result = Services.search.parseSubmissionURL(
                               "http://www.google.com/search?q=");
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, "");

  
  let result = Services.search.parseSubmissionURL(
                               "http://www.google.com/search/?q=test");
  do_check_eq(result.engine, null);
  do_check_eq(result.terms, "");

  
  let result = Services.search.parseSubmissionURL(
                               "http://www.google.com/search?q2=test");
  do_check_eq(result.engine, null);
  do_check_eq(result.terms, "");

  
  let result = Services.search.parseSubmissionURL(
                               "file://localhost/search?q=test");
  do_check_eq(result.engine, null);
  do_check_eq(result.terms, "");
});
