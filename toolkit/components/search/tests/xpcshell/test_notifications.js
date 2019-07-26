


"use strict";

const Ci = Components.interfaces;

Components.utils.import("resource://testing-common/httpd.js");

let gTestLog = [];












let expectedLog = [
  "engine-changed", 
  "engine-added",
  "engine-default",
  "engine-current",
  "engine-loaded",
  "engine-removed"
];

function search_observer(subject, topic, data) {
  let engine = subject.QueryInterface(Ci.nsISearchEngine);
  gTestLog.push(data + " for " + engine.name);

  do_print("Observer: " + data + " for " + engine.name);

  switch (data) {
    case "engine-added":
      let retrievedEngine = Services.search.getEngineByName("Test search engine");
      do_check_eq(engine, retrievedEngine);
      Services.search.defaultEngine = engine;
      Services.search.currentEngine = engine;
      do_execute_soon(function () {
        Services.search.removeEngine(engine);
      });
      break;
    case "engine-removed":
      let engineNameOutput = " for Test search engine";
      expectedLog = expectedLog.map(logLine => logLine + engineNameOutput);
      do_print("expectedLog:\n" + expectedLog.join("\n"))
      do_print("gTestLog:\n" + gTestLog.join("\n"))
      for (let i = 0; i < expectedLog.length; i++) {
        do_check_eq(gTestLog[i], expectedLog[i]);
      }
      do_check_eq(gTestLog.length, expectedLog.length);
      do_test_finished();
      break;
  }
}

function run_test() {
  removeMetadata();
  updateAppInfo();

  let httpServer = new HttpServer();
  httpServer.start(-1);
  httpServer.registerDirectory("/", do_get_cwd());

  do_register_cleanup(function cleanup() {
    httpServer.stop(function() {});
    Services.obs.removeObserver(search_observer, "browser-search-engine-modified");
  });

  do_test_pending();

  Services.obs.addObserver(search_observer, "browser-search-engine-modified", false);

  Services.search.addEngine("http://localhost:" +
                            httpServer.identity.primaryPort +
                            "/data/engine.xml",
                            Ci.nsISearchEngine.DATA_XML,
                            null, false);
}
