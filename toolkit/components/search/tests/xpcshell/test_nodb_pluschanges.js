




















const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");


function run_test()
{
  do_print("Preparing test");
  removeMetadata();
  updateAppInfo();
  do_load_manifest("data/chrome.manifest");

  let httpServer = new HttpServer();
  httpServer.start(4444);
  httpServer.registerDirectory("/", do_get_cwd());

  let search = Services.search;

  do_print("Setting up observer");
  function observer(aSubject, aTopic, aData) {
    do_print("Observing topic " + aTopic);
    if ("engine-added" == aData) {
      let engine1 = search.getEngineByName("Test search engine");
      let engine2 = search.getEngineByName("Sherlock test search engine");
      do_print("Currently, engine1 is " + engine1);
      do_print("Currently, engine2 is " + engine2);
      if(engine1 && engine2)
      {
        search.moveEngine(engine1, 0);
        search.moveEngine(engine2, 1);
        do_print("Next step is forcing flush");
        do_timeout(0,
                   function() {
                     do_print("Forcing flush");
                     
                     
                     
                     search.QueryInterface(Ci.nsIObserver).
                       observe(observer, "quit-application", "<no verb>");
                   });
        afterCommit(
          function()
          {
            do_print("Commit complete");
            
            let metadata = gProfD.clone();
            metadata.append("search-metadata.json");
            do_check_true(metadata.exists());

            
            let stream = NetUtil.newChannel(metadata).open();
            do_print("Parsing metadata");
            let json = parseJsonFromStream(stream);
            do_check_eq(json["[app]/test-search-engine.xml"].order, 1);
            do_check_eq(json["[profile]/sherlock-test-search-engine.xml"].order, 2);

            do_print("Cleaning up");
            httpServer.stop(function() {});
            stream.close(); 
            removeMetadata();
            do_test_finished();
          }
        );
      }
    }
  };
  Services.obs.addObserver(observer, "browser-search-engine-modified",
                           false);

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
