






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

  let [engine1, engine2, engine3, engine4] = yield addTestEngines([
    { name: "Test search engine", xmlFileName: "engine.xml" },
    { name: "Test search engine (fr)", xmlFileName: "engine-fr.xml" },
    { name: "bacon_addParam", details: ["", "bacon_addParam", "Search Bacon",
                                        "GET", "http://www.bacon.test/find"] },
    { name: "idn_addParam", details: ["", "idn_addParam", "Search IDN",
                                        "GET", "http://www.xn--bcher-kva.ch/search"] },
    
    { name: "A second test engine", xmlFileName: "engine2.xml" },
    { name: "Sherlock test search engine", srcFileName: "engine.src",
      iconFileName: "ico-size-16x16-png.ico" },
    { name: "bacon", details: ["", "bacon", "Search Bacon", "GET",
                               "http://www.bacon.moz/search?q={searchTerms}"] },
  ]);

  engine3.addParam("q", "{searchTerms}", null);
  engine4.addParam("q", "{searchTerms}", null);

  
  let url = "http://www.google.com/search?foo=bar&q=caff%C3%A8";
  let result = Services.search.parseSubmissionURL(url);
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, "caff\u00E8");
  do_check_true(url.slice(result.termsOffset).startsWith("caff%C3%A8"));
  do_check_eq(result.termsLength, "caff%C3%A8".length);

  
  
  
  url = "http://www.google.fr/search?q=caff%E8";
  result = Services.search.parseSubmissionURL(url);
  do_check_eq(result.engine, engine2);
  do_check_eq(result.terms, "caff\u00E8");
  do_check_true(url.slice(result.termsOffset).startsWith("caff%E8"));
  do_check_eq(result.termsLength, "caff%E8".length);

  
  
  url = "http://www.google.co.uk/search?q=caff%C3%A8";
  result = Services.search.parseSubmissionURL(url);
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, "caff\u00E8");
  do_check_true(url.slice(result.termsOffset).startsWith("caff%C3%A8"));
  do_check_eq(result.termsLength, "caff%C3%A8".length);

  
  
  url = "http://www.bacon.test/find?q=caff%E8";
  result = Services.search.parseSubmissionURL(url);
  do_check_eq(result.engine, engine3);
  do_check_eq(result.terms, "caff\u00E8");
  do_check_true(url.slice(result.termsOffset).startsWith("caff%E8"));
  do_check_eq(result.termsLength, "caff%E8".length);

  
  url = "http://www.google.com/search?q=foo+b\u00E4r";
  result = Services.search.parseSubmissionURL(url);
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, "foo b\u00E4r");
  do_check_true(url.slice(result.termsOffset).startsWith("foo+b\u00E4r"));
  do_check_eq(result.termsLength, "foo+b\u00E4r".length);

  
  url = "http://www.b\u00FCcher.ch/search?q=foo+bar";
  result = Services.search.parseSubmissionURL(url);
  do_check_eq(result.engine, engine4);
  do_check_eq(result.terms, "foo bar");
  do_check_true(url.slice(result.termsOffset).startsWith("foo+bar"));
  do_check_eq(result.termsLength, "foo+bar".length);

  
  url = "http://www.xn--bcher-kva.ch/search?q=foo+bar";
  result = Services.search.parseSubmissionURL(url);
  do_check_eq(result.engine, engine4);
  do_check_eq(result.terms, "foo bar");
  do_check_true(url.slice(result.termsOffset).startsWith("foo+bar"));
  do_check_eq(result.termsLength, "foo+bar".length);

  
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

  
  url = "https://www.google.com/search?q=caff%C3%A8";
  result = Services.search.parseSubmissionURL(url);
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, "caff\u00E8");
  do_check_true(url.slice(result.termsOffset).startsWith("caff%C3%A8"));

  
  result = Services.search.parseSubmissionURL(
             "http://www.google.com/search?q=+with++spaces+");
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, " with  spaces ");

  
  url = "http://www.google.com/search?q=";
  result = Services.search.parseSubmissionURL(url);
  do_check_eq(result.engine, engine1);
  do_check_eq(result.terms, "");
  do_check_eq(result.termsOffset, url.length);

  
  result = Services.search.parseSubmissionURL(
             "http://www.google.com/search/?q=test");
  do_check_eq(result.engine, null);
  do_check_eq(result.terms, "");
  do_check_eq(result.termsOffset, -1);

  
  result = Services.search.parseSubmissionURL(
             "http://www.google.com/search?q2=test");
  do_check_eq(result.engine, null);
  do_check_eq(result.terms, "");
  do_check_eq(result.termsOffset, -1);

  
  result = Services.search.parseSubmissionURL(
             "file://localhost/search?q=test");
  do_check_eq(result.engine, null);
  do_check_eq(result.terms, "");
  do_check_eq(result.termsOffset, -1);
});
