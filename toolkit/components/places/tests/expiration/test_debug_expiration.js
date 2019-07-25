









let gNow = Date.now() * 1000;

add_test(function test_expire_orphans()
{
  
  PlacesUtils.history.addVisit(NetUtil.newURI("http://page1.mozilla.org/"),
                               gNow++, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  PlacesUtils.history.addVisit(NetUtil.newURI("http://page2.mozilla.org/"),
                               gNow++, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                NetUtil.newURI("http://page3.mozilla.org/"),
                                                PlacesUtils.bookmarks.DEFAULT_INDEX, "");
  PlacesUtils.bookmarks.removeItem(id);

  
  Services.obs.addObserver(function (aSubject, aTopic, aData)
  {
    Services.obs.removeObserver(arguments.callee, aTopic);

    
    do_check_eq(visits_in_database("http://page1.mozilla.org/"), 1);
    do_check_eq(visits_in_database("http://page2.mozilla.org/"), 1);
    do_check_false(page_in_database("http://page3.mozilla.org/"));

    
    waitForClearHistory(run_next_test);
  }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  
  force_expiration_step(0);
});

add_test(function test_expire_orphans_optionalarg()
{
  
  PlacesUtils.history.addVisit(NetUtil.newURI("http://page1.mozilla.org/"),
                               gNow++, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  PlacesUtils.history.addVisit(NetUtil.newURI("http://page2.mozilla.org/"),
                               gNow++, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                NetUtil.newURI("http://page3.mozilla.org/"),
                                                PlacesUtils.bookmarks.DEFAULT_INDEX, "");
  PlacesUtils.bookmarks.removeItem(id);

  
  Services.obs.addObserver(function (aSubject, aTopic, aData)
  {
    Services.obs.removeObserver(arguments.callee, aTopic);

    
    do_check_eq(visits_in_database("http://page1.mozilla.org/"), 1);
    do_check_eq(visits_in_database("http://page2.mozilla.org/"), 1);
    do_check_false(page_in_database("http://page3.mozilla.org/"));

    
    waitForClearHistory(run_next_test);
  }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  
  force_expiration_step();
});

add_test(function test_expire_limited()
{
  
  
  PlacesUtils.history.addVisit(NetUtil.newURI("http://page1.mozilla.org/"),
                               gNow++, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  PlacesUtils.history.addVisit(NetUtil.newURI("http://page2.mozilla.org/"),
                               gNow++, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  
  Services.obs.addObserver(function (aSubject, aTopic, aData)
  {
    Services.obs.removeObserver(arguments.callee, aTopic);

    
    do_check_false(page_in_database("http://page1.mozilla.org/"));
    do_check_eq(visits_in_database("http://page2.mozilla.org/"), 1);

    
    waitForClearHistory(run_next_test);
  }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  
  force_expiration_step(1);
});

add_test(function test_expire_unlimited()
{
  
  
  PlacesUtils.history.addVisit(NetUtil.newURI("http://page1.mozilla.org/"),
                               gNow++, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  PlacesUtils.history.addVisit(NetUtil.newURI("http://page2.mozilla.org/"),
                               gNow++, null,
                               PlacesUtils.history.TRANSITION_TYPED, false, 0);
  
  Services.obs.addObserver(function (aSubject, aTopic, aData)
  {
    Services.obs.removeObserver(arguments.callee, aTopic);

    
    do_check_false(page_in_database("http://page1.mozilla.org/"));
    do_check_false(page_in_database("http://page2.mozilla.org/"));

    
    waitForClearHistory(run_next_test);
  }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  
  force_expiration_step(-1);
});

function run_test()
{
  
  setInterval(3600); 
  
  setMaxPages(1);

  run_next_test();
}
