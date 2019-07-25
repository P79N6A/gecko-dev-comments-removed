




















do_load_httpd_js();

function run_test()
{
  removeMetadata();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "2");
  do_load_manifest("data/chrome.manifest");

  let httpServer = new nsHttpServer();
  httpServer.start(4444);
  httpServer.registerDirectory("/", do_get_cwd());

  let search = Services.search;

  function observer(aSubject, aTopic, aData) {
    if ("engine-added" == aData) {
      let engine1 = search.getEngineByName("Test search engine");
      let engine2 = search.getEngineByName("Sherlock test search engine");
      dumpn("Got engine 2: "+engine2);
      if(engine1 && engine2)
      {
        search.moveEngine(engine1, 0);
        search.moveEngine(engine2, 1);
        do_timeout(0,
                   function() {
                     
                     
                     
                     search.QueryInterface(Ci.nsIObserver).
                       observe(observer, "quit-application", "<no verb>");
                   });
        afterCommit(
          function()
          {
            
            let metadata = gProfD.clone();
            metadata.append("search-metadata.json");
            do_check_true(metadata.exists());

            
	    let stream = NetUtil.newChannel(metadata).open();
            let json = parseJsonFromStream(stream);
            do_check_eq(json["[app]/test-search-engine.xml"].order, 1);
            do_check_eq(json["[profile]/sherlock-test-search-engine.xml"].order, 2);
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
}
