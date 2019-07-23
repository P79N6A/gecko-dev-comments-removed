














































let pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
         getService(Ci.nsIPrivateBrowsingService);

const COOKIE_EXPIRY = Math.round(Date.now() / 1000) + 60;
const COOKIE_NAME = "testcookie";
const COOKIE_PATH = "/";

const LOGIN_USERNAME = "username";
const LOGIN_PASSWORD = "password";
const LOGIN_USERNAME_FIELD = "username_field";
const LOGIN_PASSWORD_FIELD = "password_field";

const PERMISSION_TYPE = "test-perm";
const PERMISSION_VALUE = Ci.nsIPermissionManager.ALLOW_ACTION;

const PREFERENCE_NAME = "test-pref";











function uri(aURIString)
{
  return Cc["@mozilla.org/network/io-service;1"].
         getService(Ci.nsIIOService).
         newURI(aURIString, null, null);
}







function add_visit(aURI)
{
  check_visited(aURI, false);
  let bh = Cc["@mozilla.org/browser/global-history;2"].
           getService(Ci.nsIBrowserHistory);
  bh.addPageWithDetails(aURI, aURI.spec, Date.now() * 1000);
  check_visited(aURI, true);
}









function check_visited(aURI, aIsVisited)
{
  let gh = Cc["@mozilla.org/browser/global-history;2"].
           getService(Ci.nsIGlobalHistory2);
  let checker = aIsVisited ? do_check_true : do_check_false;
  checker(gh.isVisited(aURI));
}






function add_cookie(aDomain)
{
  check_cookie_exists(aDomain, false);
  let cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  cm.add(aDomain, COOKIE_PATH, COOKIE_NAME, "", false, false, false,
         COOKIE_EXPIRY);
  check_cookie_exists(aDomain, true);
}









function check_cookie_exists(aDomain, aExists)
{
  let cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  let cookie = {
    host: aDomain,
    name: COOKIE_NAME,
    path: COOKIE_PATH
  }
  let checker = aExists ? do_check_true : do_check_false;
  checker(cm.cookieExists(cookie));
}










function add_download(aURIString, aIsActive)
{
  check_downloaded(aURIString, false);
  let db = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager).
           DBConnection;
  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (source, state) " +
    "VALUES (:source, :state)"
  );
  stmt.params.source = aURIString;
  stmt.params.state = aIsActive ? Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING :
                                  Ci.nsIDownloadManager.DOWNLOAD_FINISHED;
  try {
    stmt.execute();
  }
  finally {
    stmt.finalize();
  }
  check_downloaded(aURIString, true);
}









function check_downloaded(aURIString, aIsDownloaded)
{
  let db = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager).
           DBConnection;
  let stmt = db.createStatement(
    "SELECT * " +
    "FROM moz_downloads " +
    "WHERE source = :source"
  );
  stmt.params.source = aURIString;

  let checker = aIsDownloaded ? do_check_true : do_check_false;
  try {
    checker(stmt.executeStep());
  }
  finally {
    stmt.finalize();
  }
}







function add_disabled_host(aHost)
{
  check_disabled_host(aHost, false);
  let lm = Cc["@mozilla.org/login-manager;1"].
           getService(Ci.nsILoginManager);
  lm.setLoginSavingEnabled(aHost, false);
  check_disabled_host(aHost, true);
}









function check_disabled_host(aHost, aIsDisabled)
{
  let lm = Cc["@mozilla.org/login-manager;1"].
           getService(Ci.nsILoginManager);
  let checker = aIsDisabled ? do_check_false : do_check_true;
  checker(lm.getLoginSavingEnabled(aHost));
}







function add_login(aHost)
{
  check_login_exists(aHost, false);
  let login = Cc["@mozilla.org/login-manager/loginInfo;1"].
              createInstance(Ci.nsILoginInfo);
  login.init(aHost, "", null, LOGIN_USERNAME, LOGIN_PASSWORD,
             LOGIN_USERNAME_FIELD, LOGIN_PASSWORD_FIELD);
  let lm = Cc["@mozilla.org/login-manager;1"].
           getService(Ci.nsILoginManager);
  lm.addLogin(login);
  check_login_exists(aHost, true);
}









function check_login_exists(aHost, aExists)
{
  let lm = Cc["@mozilla.org/login-manager;1"].
           getService(Ci.nsILoginManager);
  let count = { value: 0 };
  lm.findLogins(count, aHost, "", null);
  do_check_eq(count.value, aExists ? 1 : 0);
}







function add_permission(aURI)
{
  check_permission_exists(aURI, false);
  let pm = Cc["@mozilla.org/permissionmanager;1"].
           getService(Ci.nsIPermissionManager);
  pm.add(aURI, PERMISSION_TYPE, PERMISSION_VALUE);
  check_permission_exists(aURI, true);
}









function check_permission_exists(aURI, aExists)
{
  let pm = Cc["@mozilla.org/permissionmanager;1"].
           getService(Ci.nsIPermissionManager);
  let perm = pm.testExactPermission(aURI, PERMISSION_TYPE);
  let checker = aExists ? do_check_eq : do_check_neq;
  checker(perm, PERMISSION_VALUE);
}







function add_preference(aURI)
{
  check_preference_exists(aURI, false);
  let cp = Cc["@mozilla.org/content-pref/service;1"].
           getService(Ci.nsIContentPrefService);
  cp.setPref(aURI, PREFERENCE_NAME, "foo");
  check_preference_exists(aURI, true);
}









function check_preference_exists(aURI, aExists)
{
  let cp = Cc["@mozilla.org/content-pref/service;1"].
           getService(Ci.nsIContentPrefService);
  let checker = aExists ? do_check_true : do_check_false;
  checker(cp.hasPref(aURI, PREFERENCE_NAME));
}





function test_history_cleared_with_direct_match()
{
  const TEST_URI = uri("http://mozilla.org/foo");
  add_visit(TEST_URI);
  pb.removeDataFromDomain("mozilla.org");
  check_visited(TEST_URI, false);
}

function test_history_cleared_with_subdomain()
{
  const TEST_URI = uri("http://www.mozilla.org/foo");
  add_visit(TEST_URI);
  pb.removeDataFromDomain("mozilla.org");
  check_visited(TEST_URI, false);
}

function test_history_not_cleared_with_uri_contains_domain()
{
  const TEST_URI = uri("http://ilovemozilla.org/foo");
  add_visit(TEST_URI);
  pb.removeDataFromDomain("mozilla.org");
  check_visited(TEST_URI, true);

  
  let bh = Cc["@mozilla.org/browser/global-history;2"].
           getService(Ci.nsIBrowserHistory);
  bh.removeAllPages();
}


function test_cookie_cleared_with_direct_match()
{
  const TEST_DOMAIN = "mozilla.org";
  add_cookie(TEST_DOMAIN);
  pb.removeDataFromDomain("mozilla.org");
  check_cookie_exists(TEST_DOMAIN, false);
}

function test_cookie_cleared_with_subdomain()
{
  const TEST_DOMAIN = "www.mozilla.org";
  add_cookie(TEST_DOMAIN);
  pb.removeDataFromDomain("mozilla.org");
  check_cookie_exists(TEST_DOMAIN, false);
}

function test_cookie_not_cleared_with_uri_contains_domain()
{
  const TEST_DOMAIN = "ilovemozilla.org";
  add_cookie(TEST_DOMAIN);
  pb.removeDataFromDomain("mozilla.org");
  check_cookie_exists(TEST_DOMAIN, true);
}


function test_download_history_cleared_with_direct_match()
{
  const TEST_URI = "http://mozilla.org/foo";
  add_download(TEST_URI, false);
  pb.removeDataFromDomain("mozilla.org");
  check_downloaded(TEST_URI, false);
}

function test_download_history_cleared_with_subdomain()
{
  const TEST_URI = "http://www.mozilla.org/foo";
  add_download(TEST_URI, false);
  pb.removeDataFromDomain("mozilla.org");
  check_downloaded(TEST_URI, false);
}

function test_download_history_not_cleared_with_active_direct_match()
{
  
  const TEST_URI = "http://mozilla.org/foo";
  add_download(TEST_URI, true);
  pb.removeDataFromDomain("mozilla.org");
  check_downloaded(TEST_URI, true);

  
  let db = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager).
           DBConnection;
  db.executeSimpleSQL("DELETE FROM moz_downloads");
  check_downloaded(TEST_URI, false);
}


function test_login_manager_disabled_hosts_cleared_with_direct_match()
{
  const TEST_HOST = "http://mozilla.org";
  add_disabled_host(TEST_HOST);
  pb.removeDataFromDomain("mozilla.org");
  check_disabled_host(TEST_HOST, false);
}

function test_login_manager_disabled_hosts_cleared_with_subdomain()
{
  const TEST_HOST = "http://www.mozilla.org";
  add_disabled_host(TEST_HOST);
  pb.removeDataFromDomain("mozilla.org");
  check_disabled_host(TEST_HOST, false);
}

function test_login_manager_disabled_hosts_not_cleared_with_uri_contains_domain()
{
  const TEST_HOST = "http://ilovemozilla.org";
  add_disabled_host(TEST_HOST);
  pb.removeDataFromDomain("mozilla.org");
  check_disabled_host(TEST_HOST, true);

  
  let lm = Cc["@mozilla.org/login-manager;1"].
           getService(Ci.nsILoginManager);
  lm.setLoginSavingEnabled(TEST_HOST, true);
  check_disabled_host(TEST_HOST, false);
}

function test_login_manager_logins_cleared_with_direct_match()
{
  const TEST_HOST = "http://mozilla.org";
  add_login(TEST_HOST);
  pb.removeDataFromDomain("mozilla.org");
  check_login_exists(TEST_HOST, false);
}

function test_login_manager_logins_cleared_with_subdomain()
{
  const TEST_HOST = "http://www.mozilla.org";
  add_login(TEST_HOST);
  pb.removeDataFromDomain("mozilla.org");
  check_login_exists(TEST_HOST, false);
}

function tets_login_manager_logins_not_cleared_with_uri_contains_domain()
{
  const TEST_HOST = "http://ilovemozilla.org";
  add_login(TEST_HOST);
  pb.removeDataFromDomain("mozilla.org");
  check_login_exists(TEST_HOST, true);

  let lm = Cc["@mozilla.org/login-manager;1"].
           getService(Ci.nsILoginManager);
  lm.removeAllLogins();
  check_login_exists(TEST_HOST, false);
}


function test_permission_manager_cleared_with_direct_match()
{
  const TEST_URI = uri("http://mozilla.org");
  add_permission(TEST_URI);
  pb.removeDataFromDomain("mozilla.org");
  check_permission_exists(TEST_URI, false);
}

function test_permission_manager_cleared_with_subdomain()
{
  const TEST_URI = uri("http://www.mozilla.org");
  add_permission(TEST_URI);
  pb.removeDataFromDomain("mozilla.org");
  check_permission_exists(TEST_URI, false);
}

function test_permission_manager_not_cleared_with_uri_contains_domain()
{
  const TEST_URI = uri("http://ilovemozilla.org");
  add_permission(TEST_URI);
  pb.removeDataFromDomain("mozilla.org");
  check_permission_exists(TEST_URI, true);

  
  let pm = Cc["@mozilla.org/permissionmanager;1"].
           getService(Ci.nsIPermissionManager);
  pm.removeAll();
  check_permission_exists(TEST_URI, false);
}


function test_content_preferences_cleared_with_direct_match()
{
  const TEST_URI = uri("http://mozilla.org");
  add_preference(TEST_URI);
  pb.removeDataFromDomain("mozilla.org");
  check_preference_exists(TEST_URI, false);
}

function test_content_preferences_cleared_with_subdomain()
{
  const TEST_URI = uri("http://www.mozilla.org");
  add_preference(TEST_URI);
  pb.removeDataFromDomain("mozilla.org");
  check_preference_exists(TEST_URI, false);
}

function test_content_preferecnes_not_cleared_with_uri_contains_domain()
{
  const TEST_URI = uri("http://ilovemozilla.org");
  add_preference(TEST_URI);
  pb.removeDataFromDomain("mozilla.org");
  check_preference_exists(TEST_URI, true);

  
  let cp = Cc["@mozilla.org/content-pref/service;1"].
           getService(Ci.nsIContentPrefService);
  cp.removePref(TEST_URI, PREFERENCE_NAME);
  check_preference_exists(TEST_URI, false);
}


function test_cache_cleared()
{
  
  do_check_eq(tests[tests.length - 1], arguments.callee);

  
  
  
  
  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let observer = {
    observe: function(aSubject, aTopic, aData)
    {
      os.removeObserver(observer, "cacheservice:empty-cache");
      shutdownPlaces();
      do_test_finished();
    }
  };
  os.addObserver(observer, "cacheservice:empty-cache", false);
  pb.removeDataFromDomain("mozilla.org");
  do_test_pending();
}

let tests = [
  
  test_history_cleared_with_direct_match,
  test_history_cleared_with_subdomain,
  test_history_not_cleared_with_uri_contains_domain,

  
  test_cookie_cleared_with_direct_match,
  test_cookie_cleared_with_subdomain,
  test_cookie_not_cleared_with_uri_contains_domain,

  
  
  test_download_history_cleared_with_direct_match,
  test_download_history_cleared_with_subdomain,
  test_download_history_not_cleared_with_active_direct_match,

  
  test_login_manager_disabled_hosts_cleared_with_direct_match,
  test_login_manager_disabled_hosts_cleared_with_subdomain,
  test_login_manager_disabled_hosts_not_cleared_with_uri_contains_domain,
  test_login_manager_logins_cleared_with_direct_match,
  test_login_manager_logins_cleared_with_subdomain,
  tets_login_manager_logins_not_cleared_with_uri_contains_domain,

  
  test_permission_manager_cleared_with_direct_match,
  test_permission_manager_cleared_with_subdomain,
  test_permission_manager_not_cleared_with_uri_contains_domain,

  
  test_content_preferences_cleared_with_direct_match,
  test_content_preferences_cleared_with_subdomain,
  test_content_preferecnes_not_cleared_with_uri_contains_domain,

  
  test_cache_cleared,
];

function do_test()
{
  for (let i = 0; i < tests.length; i++)
    tests[i]();
}
