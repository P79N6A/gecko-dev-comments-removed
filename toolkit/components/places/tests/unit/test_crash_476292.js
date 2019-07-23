












































function run_test()
{
  
  
  
  
  let dbFile = do_get_file("bug476292.sqlite");
  let profD = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties).
             get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);
  dbFile.copyTo(profD, "places.sqlite");

  
  let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
}
