










Cu.import("resource://testing-common/httpd.js");

var httpServer = null;
var cacheUpdateObserver = null;

function make_channel(url, callback, ctx) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newChannel(url, "", null);
}

function make_uri(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newURI(url, null, null);
}


function masterEntryHandler(metadata, response)
{
  var masterEntryContent = "<html manifest='/manifest'></html>";
  response.setHeader("Content-Type", "text/html");
  response.bodyOutputStream.write(masterEntryContent, masterEntryContent.length);
}


function manifestHandler(metadata, response)
{
  var manifestContent = "CACHE MANIFEST\n";
  response.setHeader("Content-Type", "text/cache-manifest");
  response.bodyOutputStream.write(manifestContent, manifestContent.length);
}


function finish_test(customDir)
{
  var offlineCacheDir = customDir.clone();
  offlineCacheDir.append("OfflineCache");

  var indexSqlFile = offlineCacheDir.clone();
  indexSqlFile.append('index.sqlite');
  do_check_eq(indexSqlFile.exists(), true);

  var file1 = offlineCacheDir.clone();
  file1.append("2");
  file1.append("E");
  file1.append("2C99DE6E7289A5-0");
  do_check_eq(file1.exists(), true);

  var file2 = offlineCacheDir.clone();
  file2.append("8");
  file2.append("6");
  file2.append("0B457F75198B29-0");
  do_check_eq(file2.exists(), true);

  
  
  
  

  
  
  
  try {
    indexSqlFile.remove(false);
    do_check_true(true);
  }
  catch (ex) {
    do_throw("Could not remove the sqlite.index file, we still keep it open \n" + ex + "\n");
  }

  httpServer.stop(do_test_finished);
}

function run_test()
{
  httpServer = new HttpServer();
  httpServer.registerPathHandler("/masterEntry", masterEntryHandler);
  httpServer.registerPathHandler("/manifest", manifestHandler);
  httpServer.start(4444);

  var profileDir = do_get_profile();
  var customDir = profileDir.clone();
  customDir.append("customOfflineCacheDir" + Math.random());

  var pm = Cc["@mozilla.org/permissionmanager;1"]
    .getService(Ci.nsIPermissionManager);
  var uri = make_uri("http://localhost:4444");
  var principal = Cc["@mozilla.org/scriptsecuritymanager;1"]
                    .getService(Ci.nsIScriptSecurityManager)
                    .getNoAppCodebasePrincipal(uri);

  if (pm.testPermissionFromPrincipal(principal, "offline-app") != 0) {
    dump("Previous test failed to clear offline-app permission!  Expect failures.\n");
  }
  pm.addFromPrincipal(principal, "offline-app", Ci.nsIPermissionManager.ALLOW_ACTION);

  var ps = Cc["@mozilla.org/preferences-service;1"]
    .getService(Ci.nsIPrefBranch);
  ps.setBoolPref("browser.cache.offline.enable", true);
  
  ps.setComplexValue("browser.cache.offline.parent_directory", Ci.nsILocalFile, profileDir);

  var us = Cc["@mozilla.org/offlinecacheupdate-service;1"].
           getService(Ci.nsIOfflineCacheUpdateService);
  var update = us.scheduleCustomProfileUpdate(
      make_uri("http://localhost:4444/manifest"),
      make_uri("http://localhost:4444/masterEntry"),
      customDir);

  var expectedStates = [
      Ci.nsIOfflineCacheUpdateObserver.STATE_DOWNLOADING,
      Ci.nsIOfflineCacheUpdateObserver.STATE_ITEMSTARTED,
      Ci.nsIOfflineCacheUpdateObserver.STATE_ITEMPROGRESS,
      Ci.nsIOfflineCacheUpdateObserver.STATE_ITEMCOMPLETED,
      Ci.nsIOfflineCacheUpdateObserver.STATE_FINISHED
  ];

  update.addObserver({
    updateStateChanged: function(update, state)
    {
      do_check_eq(state, expectedStates.shift());

      if (state == Ci.nsIOfflineCacheUpdateObserver.STATE_FINISHED)
          finish_test(customDir);
    },

    applicationCacheAvailable: function(appCache)
    {
    }
  }, false);

  do_test_pending();
}
