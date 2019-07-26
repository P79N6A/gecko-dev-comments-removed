


















const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

function run_test() {
  do_print("Preparing test");
  removeMetadata();
  updateAppInfo();

  let httpServer = new HttpServer();
  httpServer.start(4444);
  httpServer.registerDirectory("/", do_get_cwd());

  function getSearchMetadata() {
    
    let metadata = gProfD.clone();
    metadata.append("search-metadata.json");
    do_check_true(metadata.exists());

    let stream = NetUtil.newChannel(metadata).open();
    do_print("Parsing metadata");
    let json = parseJsonFromStream(stream);
    stream.close(); 

    return json;
  }

  let search = Services.search;

  do_print("Setting up observer");
  function observer(aSubject, aTopic, aData) {
    do_print("Observing topic " + aTopic);
    if ("engine-added" != aData) {
      return;
    }

    let engine1 = search.getEngineByName("Test search engine");
    let engine2 = search.getEngineByName("Sherlock test search engine");
    do_print("Currently, engine1 is " + engine1);
    do_print("Currently, engine2 is " + engine2);
    if (!engine1 || !engine2) {
      return;
    }

    
    search.moveEngine(engine1, 0);
    search.moveEngine(engine2, 1);

    
    afterCommit(function() {
      do_print("Commit complete after moveEngine");

      
      let json = getSearchMetadata();
      do_check_eq(json["[app]/test-search-engine.xml"].order, 1);
      do_check_eq(json["[profile]/sherlock-test-search-engine.xml"].order, 2);

      
      search.removeEngine(engine1);
      afterCommit(function() {
        do_print("Commit complete after removeEngine");

        
        let json = getSearchMetadata();
        do_check_eq(json["[profile]/sherlock-test-search-engine.xml"].order, 1);

        
        search.addEngineWithDetails("foo", "", "foo", "", "GET", "http://searchget/?search={searchTerms}");
        afterCommit(function() {
          do_print("Commit complete after addEngineWithDetails");

          
          let json = getSearchMetadata();
          do_check_eq(json["[profile]/foo.xml"].alias, "foo");
          do_check_true(json["[profile]/foo.xml"].order > 0);

          do_print("Cleaning up");
          Services.obs.removeObserver(observer, "browser-search-engine-modified");
          httpServer.stop(function() {});
          removeMetadata();
          do_test_finished();
        });
      });
    });
  };
  Services.obs.addObserver(observer, "browser-search-engine-modified", false);

  do_test_pending();

  search.addEngine("http://localhost:4444/data/engine.xml",
                   Ci.nsISearchEngine.DATA_XML,
                   null, false);
  search.addEngine("http://localhost:4444/data/engine.src",
                   Ci.nsISearchEngine.DATA_TEXT,
                   "http://localhost:4444/data/ico-size-16x16-png.ico",
                   false);

  do_timeout(120000, function() {
    do_throw("Timeout");
  });
}
