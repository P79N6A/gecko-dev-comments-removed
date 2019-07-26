









let gNow = getExpirablePRTime();

add_task(function test_expire_orphans()
{
  
  yield promiseAddVisits({ uri: uri("http://page1.mozilla.org/"),
                           visitDate: gNow++ });
  yield promiseAddVisits({ uri: uri("http://page2.mozilla.org/"),
                           visitDate: gNow++ });
  
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                NetUtil.newURI("http://page3.mozilla.org/"),
                                                PlacesUtils.bookmarks.DEFAULT_INDEX, "");
  PlacesUtils.bookmarks.removeItem(id);

  
  yield promiseForceExpirationStep(0);

  
  do_check_eq(visits_in_database("http://page1.mozilla.org/"), 1);
  do_check_eq(visits_in_database("http://page2.mozilla.org/"), 1);
  do_check_false(page_in_database("http://page3.mozilla.org/"));

  
  yield promiseClearHistory();
});

add_task(function test_expire_orphans_optionalarg()
{
  
  yield promiseAddVisits({ uri: uri("http://page1.mozilla.org/"),
                           visitDate: gNow++ });
  yield promiseAddVisits({ uri: uri("http://page2.mozilla.org/"),
                           visitDate: gNow++ });
  
  let id = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                NetUtil.newURI("http://page3.mozilla.org/"),
                                                PlacesUtils.bookmarks.DEFAULT_INDEX, "");
  PlacesUtils.bookmarks.removeItem(id);

  
  yield promiseForceExpirationStep();

  
  do_check_eq(visits_in_database("http://page1.mozilla.org/"), 1);
  do_check_eq(visits_in_database("http://page2.mozilla.org/"), 1);
  do_check_false(page_in_database("http://page3.mozilla.org/"));

  
  yield promiseClearHistory();
});

add_task(function test_expire_limited()
{
  
  
  yield promiseAddVisits({ uri: uri("http://page1.mozilla.org/"),
                           visitDate: gNow++ });
  yield promiseAddVisits({ uri: uri("http://page2.mozilla.org/"),
                           visitDate: gNow++ });

  
  yield promiseForceExpirationStep(1);

  
  do_check_false(page_in_database("http://page1.mozilla.org/"));
  do_check_eq(visits_in_database("http://page2.mozilla.org/"), 1);

  
  yield promiseClearHistory();
});

add_task(function test_expire_unlimited()
{
  
  
  yield promiseAddVisits({ uri: uri("http://page1.mozilla.org/"),
                           visitDate: gNow++ });
  yield promiseAddVisits({ uri: uri("http://page2.mozilla.org/"),
                           visitDate: gNow++ });

  
  yield promiseForceExpirationStep(-1);

  
  do_check_false(page_in_database("http://page1.mozilla.org/"));
  do_check_false(page_in_database("http://page2.mozilla.org/"));

  
  yield promiseClearHistory();
});

function run_test()
{
  
  setInterval(3600); 
  
  setMaxPages(1);

  run_next_test();
}
