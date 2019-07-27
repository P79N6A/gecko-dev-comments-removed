


Components.utils.import("resource://gre/modules/NetUtil.jsm");

const ABOUT_PERMISSIONS_SPEC = "about:permissions";

const TEST_URI_1 = NetUtil.newURI("http://mozilla.com/");
const TEST_URI_2 = NetUtil.newURI("http://mozilla.org/");

const TEST_PRINCIPAL_1 = Services.scriptSecurityManager.getNoAppCodebasePrincipal(TEST_URI_1);
const TEST_PRINCIPAL_2 = Services.scriptSecurityManager.getNoAppCodebasePrincipal(TEST_URI_2);


const PERM_UNKNOWN = 0;
const PERM_ALLOW = 1;
const PERM_DENY = 2;

const PERM_FIRST_PARTY_ONLY = 9;


const TEST_PERMS = {
  "password": PERM_ALLOW,
  "cookie": PERM_ALLOW,
  "geo": PERM_UNKNOWN,
  "push": PERM_DENY,
  "indexedDB": PERM_UNKNOWN,
  "popup": PERM_DENY,
  "fullscreen" : PERM_UNKNOWN,
  "camera": PERM_UNKNOWN,
  "microphone": PERM_UNKNOWN
};

const NO_GLOBAL_ALLOW = [
  "geo",
  "indexedDB",
  "fullscreen"
];


const TEST_PERMS_COUNT = 9;

function test() {
  waitForExplicitFinish();
  registerCleanupFunction(cleanUp);

  
  PlacesTestUtils.addVisits(TEST_URI_1).then(() => {
    
    
    for (let type in TEST_PERMS) {
      if (type == "password") {
        Services.logins.setLoginSavingEnabled(TEST_URI_2.prePath, true);
      } else {
        
        Services.perms.addFromPrincipal(TEST_PRINCIPAL_2, type, TEST_PERMS[type]);
      }
    }

    
    gBrowser.selectedTab = gBrowser.addTab("about:permissions");
  });

  function observer() {
    Services.obs.removeObserver(observer, "browser-permissions-initialized");
    runNextTest();
  }
  Services.obs.addObserver(observer, "browser-permissions-initialized", false);
}

function cleanUp() {
  for (let type in TEST_PERMS) {
    if (type != "password") {
      Services.perms.removeFromPrincipal(TEST_PRINCIPAL_1, type);
      Services.perms.removeFromPrincipal(TEST_PRINCIPAL_2, type);
    }
  }

  gBrowser.removeTab(gBrowser.selectedTab);
}

function runNextTest() {
  if (gTestIndex == tests.length) {
    PlacesTestUtils.clearHistory().then(finish);
    return;
  }

  let nextTest = tests[gTestIndex++];
  info("[" + nextTest.name + "] running test");
  nextTest();
}

var gSitesList;
var gHeaderDeck;
var gSiteLabel;

var gTestIndex = 0;
var tests = [
  function test_page_load() {
    is(gBrowser.currentURI.spec, ABOUT_PERMISSIONS_SPEC, "about:permissions loaded");

    gSitesList = gBrowser.contentDocument.getElementById("sites-list");
    ok(gSitesList, "got sites list");

    gHeaderDeck = gBrowser.contentDocument.getElementById("header-deck");
    ok(gHeaderDeck, "got header deck");

    gSiteLabel = gBrowser.contentDocument.getElementById("site-label");
    ok(gSiteLabel, "got site label");

    runNextTest();
  },

  function test_sites_list() {
    is(gSitesList.firstChild.id, "all-sites-item",
       "all sites is the first item in the sites list");

    ok(getSiteItem(TEST_URI_1.host), "site item from places db exists");
    ok(getSiteItem(TEST_URI_2.host), "site item from enumerating services exists");

    runNextTest();
  },

  function test_filter_sites_list() {
    
    let sitesFilter = gBrowser.contentDocument.getElementById("sites-filter");
    sitesFilter.value = TEST_URI_1.host;
    sitesFilter.doCommand();

    
    let testSite1 = getSiteItem(TEST_URI_1.host);
    ok(!testSite1.collapsed, "test site 1 is not collapsed");
    let testSite2 = getSiteItem(TEST_URI_2.host);
    ok(testSite2.collapsed, "test site 2 is collapsed");

    
    sitesFilter.value = "";
    sitesFilter.doCommand();

    runNextTest();
  },

  function test_all_sites() {
    
    is(gSitesList.selectedItem, gBrowser.contentDocument.getElementById("all-sites-item"),
       "all sites item is selected");

    let defaultsHeader = gBrowser.contentDocument.getElementById("defaults-header");
    is(defaultsHeader, gHeaderDeck.selectedPanel,
       "correct header shown for all sites");

    ok(gBrowser.contentDocument.getElementById("passwords-count").hidden,
       "passwords count is hidden");
    ok(gBrowser.contentDocument.getElementById("cookies-count").hidden,
       "cookies count is hidden");

    
    NO_GLOBAL_ALLOW.forEach(function(aType) {
      let menuitem = gBrowser.contentDocument.getElementById(aType + "-" + PERM_ALLOW);
      ok(menuitem.hidden, aType + " allow menuitem hidden for all sites");
    });

    runNextTest();
  },

  function test_all_sites_permission() {
    
    Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

    
    is(Services.prefs.getIntPref("network.cookie.cookieBehavior"), PERM_UNKNOWN,
       "network.cookie.cookieBehavior is expected default");

    
    let cookieMenulist = getPermissionMenulist("cookie");
    is(cookieMenulist.value, PERM_ALLOW,
       "menulist correctly shows that cookies are allowed");

    
    Services.prefs.setIntPref("network.cookie.cookieBehavior", PERM_DENY);
    
    is(cookieMenulist.value, PERM_DENY, "menulist correctly shows that cookies are blocked");

    
    Services.prefs.clearUserPref("network.cookie.cookieBehavior");

    runNextTest();
  },

  function test_manage_all_passwords() {
    
    addWindowListener("chrome://passwordmgr/content/passwordManager.xul", runNextTest);
    gBrowser.contentDocument.getElementById("passwords-manage-all-button").doCommand();

  },

  function test_manage_all_cookies() {
    
    addWindowListener("chrome://browser/content/preferences/cookies.xul", runNextTest);
    gBrowser.contentDocument.getElementById("cookies-manage-all-button").doCommand();
  },

  function test_select_site() {
    
    let testSiteItem = getSiteItem(TEST_URI_2.host);
    gSitesList.selectedItem = testSiteItem;

    let siteHeader = gBrowser.contentDocument.getElementById("site-header");
    is(siteHeader, gHeaderDeck.selectedPanel,
       "correct header shown for a specific site");
    is(gSiteLabel.value, TEST_URI_2.host, "header updated for selected site");

    ok(!gBrowser.contentDocument.getElementById("passwords-count").hidden,
       "passwords count is not hidden");
    ok(!gBrowser.contentDocument.getElementById("cookies-count").hidden,
       "cookies count is not hidden");

    
    NO_GLOBAL_ALLOW.forEach(function(aType) {
      let menuitem = gBrowser.contentDocument.getElementById(aType + "-" + PERM_ALLOW);
      ok(!menuitem.hidden, aType  + " allow menuitem not hidden for single site");
    });

    runNextTest();
  },

  function test_permissions() {
    let menulists = gBrowser.contentDocument.getElementsByClassName("pref-menulist");
    is(menulists.length, TEST_PERMS_COUNT, "got expected number of managed permissions");

    for (let i = 0; i < menulists.length; i++) {
      let permissionMenulist = menulists.item(i);
      let permissionType = permissionMenulist.getAttribute("type");

      
      is(permissionMenulist.value, TEST_PERMS[permissionType],
        "got expected value for " + permissionType + " permission");
    }

    runNextTest();
  },

  function test_permission_change() {
    let geoMenulist = getPermissionMenulist("geo");
    is(geoMenulist.value, PERM_UNKNOWN, "menulist correctly shows that geolocation permission is unspecified");

    
    Services.perms.addFromPrincipal(TEST_PRINCIPAL_2, "geo", PERM_DENY);
    
    is(geoMenulist.value, PERM_DENY, "menulist shows that geolocation is blocked");

    
    let geoAllowItem = gBrowser.contentDocument.getElementById("geo-" + PERM_ALLOW);
    geoMenulist.selectedItem = geoAllowItem;
    geoMenulist.doCommand();
    
    is(Services.perms.testPermissionFromPrincipal(TEST_PRINCIPAL_2, "geo"), PERM_ALLOW,
       "permission manager shows that geolocation is allowed");


    
    let cookieMenuList = getPermissionMenulist("cookie");
    let cookieItem = gBrowser.contentDocument.getElementById("cookie-" + PERM_FIRST_PARTY_ONLY);
    cookieMenuList.selectedItem = cookieItem;
    cookieMenuList.doCommand();
    is(cookieMenuList.value, PERM_FIRST_PARTY_ONLY, "menulist correctly shows that " +
       "first party only cookies are allowed");
    is(Services.perms.testPermissionFromPrincipal(TEST_PRINCIPAL_2, "cookie"),
       PERM_FIRST_PARTY_ONLY, "permission manager shows that first party cookies " +
       "are allowed");

    runNextTest();
  },

  function test_forget_site() {
    
    gBrowser.contentDocument.getElementById("forget-site-button").doCommand();
    PlacesTestUtils.clearHistory().then(() => {
      is(gSiteLabel.value, "", "site label cleared");

      let allSitesItem = gBrowser.contentDocument.getElementById("all-sites-item");
      is(gSitesList.selectedItem, allSitesItem,
         "all sites item selected after forgetting selected site");

      
      let testSiteItem = getSiteItem(TEST_URI_2.host);
      ok(!testSiteItem, "site removed from sites list");

      
      for (let type in TEST_PERMS) {
        if (type == "password") {
          ok(Services.logins.getLoginSavingEnabled(TEST_URI_2.prePath),
             "password saving should be enabled by default");
        } else {
          is(Services.perms.testPermissionFromPrincipal(TEST_PRINCIPAL_2, type), PERM_UNKNOWN,
             type + " permission should not be set for test site 2");
        }
      }

      runNextTest();
    });
  }
];

function getPermissionMenulist(aType) {
  return gBrowser.contentDocument.getElementById(aType + "-menulist");
}

function getSiteItem(aHost) {
  return gBrowser.contentDocument.
                  querySelector(".site[value='" + aHost + "']");
}

function addWindowListener(aURL, aCallback) {
  Services.wm.addListener({
    onOpenWindow: function(aXULWindow) {
      info("window opened, waiting for focus");
      Services.wm.removeListener(this);

      var domwindow = aXULWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindow);
      waitForFocus(function() {
        is(domwindow.document.location.href, aURL, "should have seen the right window open");
        domwindow.close();
        aCallback();
      }, domwindow);
    },
    onCloseWindow: function(aXULWindow) { },
    onWindowTitleChange: function(aXULWindow, aNewTitle) { }
  });
}
