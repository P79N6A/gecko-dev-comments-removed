



Components.utils.import("resource://gre/modules/Services.jsm");

const _Cc = Components.classes;
const _Ci = Components.interfaces;


function do_wait_for_db_close(db) {
  const ProfD = do_get_profile().QueryInterface(_Ci.nsILocalFile);

  let dbfile = ProfD.clone();
  dbfile.append(db);

  
  if (!dbfile.exists())
    return;

  let thr = _Cc["@mozilla.org/thread-manager;1"].
            getService(_Ci.nsIThreadManager).
            mainThread;

  
  let db = null;
  while (!db) {
    
    try {
      db = Services.storage.openUnsharedDatabase(dbfile);
    }
    catch (e) {
      if (thr.hasPendingEvents())
        thr.processNextEvent(false);
    }
  }

  
  while (db) {
    
    try {
      db.executeSimpleSQL("CREATE TABLE moz_test (id INTEGER PRIMARY KEY)");
      db.executeSimpleSQL("DROP TABLE moz_test");
      db.close();
      db = null;
    }
    catch (e) {
      if (thr.hasPendingEvents())
        thr.processNextEvent(false);
    }
  }
}


function do_reload_profile(observer, cleanse) {
  observer.observe(null, "profile-before-change", cleanse ? cleanse : "");
  do_wait_for_db_close("cookies.sqlite");
  observer.observe(null, "profile-do-change", "");
}



function do_set_cookies(uri, channel, session, expected) {
  const cs = _Cc["@mozilla.org/cookieService;1"].getService(_Ci.nsICookieService);

  var suffix = session ? "" : "; max-age=1000";

  
  cs.setCookieString(uri, null, "oh=hai" + suffix, null);
  do_check_eq(cs.countCookiesFromHost(uri.host), expected[0]);
  
  cs.setCookieString(uri, null, "can=has" + suffix, channel);
  do_check_eq(cs.countCookiesFromHost(uri.host), expected[1]);
  
  cs.setCookieStringFromHttp(uri, null, null, "cheez=burger" + suffix, null, null);
  do_check_eq(cs.countCookiesFromHost(uri.host), expected[2]);
  
  cs.setCookieStringFromHttp(uri, null, null, "hot=dog" + suffix, null, channel);
  do_check_eq(cs.countCookiesFromHost(uri.host), expected[3]);
}

