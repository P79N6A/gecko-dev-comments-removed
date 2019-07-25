



Components.utils.import("resource://gre/modules/Services.jsm");

const _Cc = Components.classes;
const _Ci = Components.interfaces;


function do_wait_for_db_close(dbfile) {
  
  if (!dbfile.exists())
    return;

  let thr = _Cc["@mozilla.org/thread-manager;1"].
            getService(_Ci.nsIThreadManager).
            mainThread;

  
  while (true) {
    
    try {
      db = Services.storage.openUnsharedDatabase(dbfile);
      db.schemaVersion = 0;
      db.schemaVersion = 2;
      db.close();
      db = null;
      break;
    }
    catch (e) {
      if (thr.hasPendingEvents())
        thr.processNextEvent(false);
    }
  }
}


function do_reload_profile(profile, observer, cleanse) {
  let dbfile = profile.QueryInterface(_Ci.nsILocalFile).clone();
  dbfile.append("cookies.sqlite");

  observer.observe(null, "profile-before-change", cleanse ? cleanse : "");
  do_wait_for_db_close(dbfile);
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

