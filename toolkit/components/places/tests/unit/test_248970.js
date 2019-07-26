













let _PBSvc = null;
function get_PBSvc() {
  if (_PBSvc)
    return _PBSvc;

  try {
    _PBSvc = Components.classes["@mozilla.org/privatebrowsing;1"].
             getService(Components.interfaces.nsIPrivateBrowsingService);
    return _PBSvc;
  } catch (e) {}
  return null;
}

let visited_URIs = ["http://www.test-link.com/",
                    "http://www.test-typed.com/",
                    "http://www.test-bookmark.com/",
                    "http://www.test-redirect-permanent.com/",
                    "http://www.test-redirect-temporary.com/",
                    "http://www.test-embed.com/",
                    "http://www.test-framed.com/",
                    "http://www.test-download.com/"];

let nonvisited_URIs = ["http://www.google.ca/typed/",
                       "http://www.google.com/bookmark/",
                       "http://www.google.co.il/link/",
                       "http://www.google.fr/download/",
                       "http://www.google.es/embed/",
                       "http://www.google.it/framed-link/",
                       "http://www.google.com.tr/redirect-permanent/",
                       "http://www.google.de/redirect-temporary/"];




function task_fill_history_visitedURI() {
  yield promiseAddVisits([
    { uri: uri(visited_URIs[0]), transition: TRANSITION_LINK },
    { uri: uri(visited_URIs[1]), transition: TRANSITION_TYPED },
    { uri: uri(visited_URIs[2]), transition: TRANSITION_BOOKMARK },
    { uri: uri(visited_URIs[3]), transition: TRANSITION_REDIRECT_PERMANENT },
    { uri: uri(visited_URIs[4]), transition: TRANSITION_REDIRECT_TEMPORARY },
    { uri: uri(visited_URIs[5]), transition: TRANSITION_EMBED },
    { uri: uri(visited_URIs[6]), transition: TRANSITION_FRAMED_LINK },
    { uri: uri(visited_URIs[7]), transition: TRANSITION_DOWNLOAD },
  ]);
}





let num_places_entries = 8;





function check_placesItem_Count(){
  
  let options = PlacesUtils.history.getNewQueryOptions();
  options.includeHidden = true;
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  let root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  let cc = root.childCount;
  root.containerOpen = false;

  
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
  let root = PlacesUtils.history.executeQuery(PlacesUtils.history.getNewQuery(),
                                              options).root;
  root.containerOpen = true;
  cc += root.childCount;
  root.containerOpen = false;

  
  do_check_eq(cc,num_places_entries);
}












let myBookmarks = new Array(2); 

function create_bookmark(aURI, aTitle, aKeyword) {
  let bookmarkID = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarksMenuFolderId,
                                                        aURI,
                                                        PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                        aTitle);
  PlacesUtils.bookmarks.setKeywordForBookmark(bookmarkID, aKeyword);
  return bookmarkID;
}








function is_bookmark_A_altered(){

  let options = PlacesUtils.history.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.maxResults = 1; 
  options.resultType = options.RESULT_TYPE_VISIT;

  let query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.bookmarks.bookmarksMenuFolder],1);

  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  do_check_eq(root.childCount,options.maxResults);
  let node = root.getChild(0);
  root.containerOpen = false;

  return (node.accessCount!=0);
}

function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  
  let pb = get_PBSvc();
  if (!pb) {
    return;
  }

  Services.prefs.setBoolPref("browser.privatebrowsing.keep_current_session", true);

  let bookmark_A_URI = NetUtil.newURI("http://google.com/");
  let bookmark_B_URI = NetUtil.newURI("http://bugzilla.mozilla.org/");

  
  do_check_false(PlacesUtils.history.hasHistoryEntries);

  
  yield task_fill_history_visitedURI();

  
  do_check_true(PlacesUtils.history.hasHistoryEntries);

  
  myBookmarks[0] = create_bookmark(bookmark_A_URI,"title 1", "google");

  
  visited_URIs.forEach(function (visited_uri) {
    do_check_true(PlacesUtils.bhistory.isVisited(uri(visited_uri)));
    if (/embed/.test(visited_uri)) {
      do_check_false(!!page_in_database(visited_uri));
    }
    else {
      do_check_true(!!page_in_database(visited_uri));
    }
  });

  check_placesItem_Count();

  
  do_check_true(PlacesUtils.bookmarks.isBookmarked(bookmark_A_URI));
  do_check_eq("google", PlacesUtils.bookmarks.getKeywordForURI(bookmark_A_URI));

  
  pb.privateBrowsingEnabled = true;

  
  do_check_false(is_bookmark_A_altered());

  
  check_placesItem_Count();

  
  do_check_true(PlacesUtils.bookmarks.isBookmarked(bookmark_A_URI));
  do_check_eq("google",PlacesUtils.bookmarks.getKeywordForURI(bookmark_A_URI));

  
  myBookmarks[1] = create_bookmark(bookmark_B_URI,"title 2", "bugzilla");

  
  
  num_places_entries++; 
  check_placesItem_Count();

  
  pb.privateBrowsingEnabled = false;

  
  do_check_true(PlacesUtils.bookmarks.isBookmarked(bookmark_B_URI));
  do_check_eq("bugzilla",PlacesUtils.bookmarks.getKeywordForURI(bookmark_B_URI));

  
  do_check_true(PlacesUtils.bookmarks.isBookmarked(bookmark_A_URI));
  do_check_eq("google",PlacesUtils.bookmarks.getKeywordForURI(bookmark_A_URI));

  
  visited_URIs.forEach(function (visited_uri) {
    do_check_true(PlacesUtils.bhistory.isVisited(uri(visited_uri)));
    if (/embed/.test(visited_uri)) {
      do_check_false(!!page_in_database(visited_uri));
    }
    else {
      do_check_true(!!page_in_database(visited_uri));
    }
  });

  Services.prefs.clearUserPref("browser.privatebrowsing.keep_current_session");
});
