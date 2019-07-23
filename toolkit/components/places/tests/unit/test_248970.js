










































try {
  var bhist = Cc["@mozilla.org/browser/global-history;2"].
                getService(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get global history service");
}


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service");
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service");
}


try {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Components.interfaces.nsIIOService);
} catch(ex) {
  do_throw("Could not get the io service");
}








var _PBSvc = null;
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










function add_visit(aURI, aType) {
  var visitId = histsvc.addVisit(uri(aURI),
                                 Date.now() * 1000,
                                 null, 
                                 aType,
                                 false, 
                                 0);
  return visitId;
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

var visited_URIs = ["http://www.test-link.com/",
                    "http://www.test-typed.com/",
                    "http://www.test-bookmark.com/",
                    "http://www.test-redirect-permanent.com/",
                    "http://www.test-redirect-temporary.com/",
                    "http://www.test-embed.com",
                    "http://www.test-download.com"];

var nonvisited_URIs = ["http://www.google.ca/",
                       "http://www.google.com/",
                       "http://www.google.co.il/",
                       "http://www.google.fr/",
                       "http://www.google.es",
                       "http://www.google.com.tr",
                       "http://www.google.de"];





function fill_history_visitedURI() {
  add_visit(visited_URIs[0], histsvc.TRANSITION_LINK);
  add_visit(visited_URIs[1], histsvc.TRANSITION_TYPED);
  add_visit(visited_URIs[2], histsvc.TRANSITION_BOOKMARK);
  add_visit(visited_URIs[3], histsvc.TRANSITION_EMBED);
  add_visit(visited_URIs[4], histsvc.TRANSITION_REDIRECT_PERMANENT);
  add_visit(visited_URIs[5], histsvc.TRANSITION_REDIRECT_TEMPORARY);
  add_visit(visited_URIs[6], histsvc.TRANSITION_DOWNLOAD);
}







function fill_history_nonvisitedURI() {
  add_visit(nonvisited_URIs[0], histsvc.TRANSITION_TYPED);
  add_visit(nonvisited_URIs[1], histsvc.TRANSITION_BOOKMARK);
  add_visit(nonvisited_URIs[2], histsvc.TRANSITION_LINK);
  add_visit(nonvisited_URIs[3], histsvc.TRANSITION_DOWNLOAD);
  add_visit(nonvisited_URIs[4], histsvc.TRANSITION_EMBED);
  add_visit(nonvisited_URIs[5], histsvc.TRANSITION_REDIRECT_PERMANENT);
  add_visit(nonvisited_URIs[6], histsvc.TRANSITION_REDIRECT_TEMPORARY);
}





var num_places_entries = 8; 







function check_placesItem_Count(){
  
  var options = histsvc.getNewQueryOptions();
  options.includeHidden = true;
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  var cc = root.childCount;
  root.containerOpen = false;

  
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
  query = histsvc.getNewQuery();
  result = histsvc.executeQuery(query, options);
  root = result.root;
  root.containerOpen = true;
  cc += root.childCount;
  root.containerOpen = false;

  
  do_check_eq(cc,num_places_entries);
}












var myBookmarks=new Array(2); 
                              

function create_bookmark(aURI, aTitle, aKeyword) {
  var bookmarkID = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder,aURI,
                   bmsvc.DEFAULT_INDEX,aTitle);
  bmsvc.setKeywordForBookmark(bookmarkID,aKeyword);
  return bookmarkID;
}








function is_bookmark_A_altered(){

  var options = histsvc.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.maxResults = 1; 
  options.resultType = options.RESULT_TYPE_VISIT;

  var query  = histsvc.getNewQuery();
  query.setFolders([bmsvc.bookmarksMenuFolder],1);

  var result = histsvc.executeQuery(query, options);
  var root = result.root;

  root.containerOpen = true;

  do_check_eq(root.childCount,options.maxResults);

  var node = root.getChild(0);
  root.containerOpen = false;

  return (node.accessCount!=0);
}

function run_test() {
  
  var pb = get_PBSvc();

  if (pb) { 
    
    var os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    const kSyncFinished = "places-sync-finished";
    do_test_pending();

    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
    prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);

    var bookmark_A_URI = ios.newURI("http://google.com/", null, null);
    var bookmark_B_URI = ios.newURI("http://bugzilla.mozilla.org/", null, null);
    var onBookmarkAAdded = {
      observe: function (aSubject, aTopic, aData) {
        os.removeObserver(this, kSyncFinished);

        check_placesItem_Count();

        
        do_check_true(bmsvc.isBookmarked(bookmark_A_URI));
        do_check_eq("google", bmsvc.getKeywordForURI(bookmark_A_URI));

        
        pb.privateBrowsingEnabled = true;

        
        for each(var visited_uri in visited_URIs)
          do_check_false(bhist.isVisited(uri(visited_uri)));

        
        do_check_false(is_bookmark_A_altered());

        
        
        fill_history_nonvisitedURI();
        for each(var nonvisited_uri in nonvisited_URIs) {
          do_check_false(uri_in_db(uri(nonvisited_uri)));
          do_check_false(bhist.isVisited(uri(nonvisited_uri)));
        }

        
        
        
        check_placesItem_Count();

        
        do_check_true(bmsvc.isBookmarked(bookmark_A_URI));
        do_check_eq("google",bmsvc.getKeywordForURI(bookmark_A_URI));

        os.addObserver(onBookmarkBAdded, kSyncFinished, false);

        
        myBookmarks[1] = create_bookmark(bookmark_B_URI,"title 2", "bugzilla");
      }
    };
    var onBookmarkBAdded = {
      observe: function (aSubject, aTopic, aData) {
        os.removeObserver(this, kSyncFinished);

        
        
        num_places_entries = 9; 
        check_placesItem_Count();

        
        pb.privateBrowsingEnabled = false;

        
        do_check_true(bmsvc.isBookmarked(bookmark_B_URI));
        do_check_eq("bugzilla",bmsvc.getKeywordForURI(bookmark_B_URI));

        
        do_check_true(bmsvc.isBookmarked(bookmark_A_URI));
        do_check_eq("google",bmsvc.getKeywordForURI(bookmark_A_URI));

        
        for each(var visited_uri in visited_URIs) {
          do_check_true(uri_in_db(uri(visited_uri)));
          do_check_true(bhist.isVisited(uri(visited_uri)));
        }

        prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
        do_test_finished();
      }
    };

    
    do_check_false(histsvc.hasHistoryEntries);

    
    fill_history_visitedURI();

    
    do_check_true(histsvc.hasHistoryEntries);

    os.addObserver(onBookmarkAAdded, kSyncFinished, false);

    
    myBookmarks[0] = create_bookmark(bookmark_A_URI,"title 1", "google");

    
    for each(var visited_uri in visited_URIs) {
      do_check_true(bhist.isVisited(uri(visited_uri)));
      do_check_true(uri_in_db(uri(visited_uri)));
    }
  }
}
