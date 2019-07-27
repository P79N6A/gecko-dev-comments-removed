









let gNow = getExpirablePRTime();

add_task(function* test_expire_orphans()
{
  
  yield PlacesTestUtils.addVisits({
    uri: uri("http://page1.mozilla.org/"),
    visitDate: gNow++
  });
  yield PlacesTestUtils.addVisits({
    uri: uri("http://page2.mozilla.org/"),
    visitDate: gNow++
  });
  
  let bm = yield PlacesUtils.bookmarks.insert({
    parentGuid: PlacesUtils.bookmarks.unfiledGuid,
    url: "http://page3.mozilla.org/",
    title: ""
  });
  yield PlacesUtils.bookmarks.remove(bm);

  
  yield promiseForceExpirationStep(0);

  
  do_check_eq(visits_in_database("http://page1.mozilla.org/"), 1);
  do_check_eq(visits_in_database("http://page2.mozilla.org/"), 1);
  do_check_false(page_in_database("http://page3.mozilla.org/"));

  
  yield PlacesTestUtils.clearHistory();
});

add_task(function* test_expire_orphans_optionalarg()
{
  
  yield PlacesTestUtils.addVisits({
    uri: uri("http://page1.mozilla.org/"),
    visitDate: gNow++
  });
  yield PlacesTestUtils.addVisits({
    uri: uri("http://page2.mozilla.org/"),
    visitDate: gNow++
  });
  
  let bm = yield PlacesUtils.bookmarks.insert({
    parentGuid: PlacesUtils.bookmarks.unfiledGuid,
    url: "http://page3.mozilla.org/",
    title: ""
  });
  yield PlacesUtils.bookmarks.remove(bm);

  
  yield promiseForceExpirationStep();

  
  do_check_eq(visits_in_database("http://page1.mozilla.org/"), 1);
  do_check_eq(visits_in_database("http://page2.mozilla.org/"), 1);
  do_check_false(page_in_database("http://page3.mozilla.org/"));

  
  yield PlacesTestUtils.clearHistory();
});

add_task(function test_expire_limited()
{
  
  
  yield PlacesTestUtils.addVisits({
    uri: uri("http://page1.mozilla.org/"),
    visitDate: gNow++
  });
  yield PlacesTestUtils.addVisits({
    uri: uri("http://page2.mozilla.org/"),
    visitDate: gNow++
  });

  
  yield promiseForceExpirationStep(1);

  
  do_check_false(page_in_database("http://page1.mozilla.org/"));
  do_check_eq(visits_in_database("http://page2.mozilla.org/"), 1);

  
  yield PlacesTestUtils.clearHistory();
});

add_task(function test_expire_unlimited()
{
  
  
  yield PlacesTestUtils.addVisits({
    uri: uri("http://page1.mozilla.org/"),
    visitDate: gNow++
  });
  yield PlacesTestUtils.addVisits({
    uri: uri("http://page2.mozilla.org/"),
    visitDate: gNow++
  });

  
  yield promiseForceExpirationStep(-1);

  
  do_check_false(page_in_database("http://page1.mozilla.org/"));
  do_check_false(page_in_database("http://page2.mozilla.org/"));

  
  yield PlacesTestUtils.clearHistory();
});

function run_test()
{
  
  setInterval(3600); 
  
  setMaxPages(1);

  run_next_test();
}
