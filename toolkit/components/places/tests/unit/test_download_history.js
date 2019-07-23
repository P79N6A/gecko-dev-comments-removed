







































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 








function uri_in_db(aURI) {
  var options = histsvc.getNewQueryOptions();
  options.maxResults = 1;
  options.resultType = options.RESULTS_AS_URI;
  options.includeHidden = true;
  var query = histsvc.getNewQuery();
  query.uri = aURI;
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  return (root.childCount == 1);
}


function run_test() {
  
  
  var dh = Cc["@mozilla.org/browser/download-history;1"].
           getService(Ci.nsIDownloadHistory);
  do_check_true(dh instanceof Ci.nsINavHistoryService);

  
  const NS_LINK_VISITED_EVENT_TOPIC = "link-visited";
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  var testURI = ios.newURI("http://google.com/", null, null);

  do_check_false(uri_in_db(testURI));

  var topicReceived = false;
  var obs = {
    observe: function tlvo_observe(aSubject, aTopic, aData)
    {
      if (NS_LINK_VISITED_EVENT_TOPIC == aTopic) {
        do_check_eq(testURI, aSubject);
        topicReceived = true;
      }
    }
  };

  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver(obs, NS_LINK_VISITED_EVENT_TOPIC, false);

  var referrerURI = ios.newURI("http://yahoo.com", null, null);
  do_check_false(uri_in_db(referrerURI));
  do_check_false(uri_in_db(testURI));
  dh.addDownload(testURI, referrerURI, Date.now() * 1000);
  do_check_true(topicReceived);
  do_check_true(uri_in_db(testURI));
  do_check_true(uri_in_db(referrerURI));
}
