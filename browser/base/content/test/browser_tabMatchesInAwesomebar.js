





































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const TEST_URL_BASES = [
  "http://example.org/browser/browser/base/content/test/dummy_page.html#tabmatch",
  "http://example.org/browser/browser/base/content/test/moz.png#tabmatch"
];

var gPrivateBrowsing = Cc["@mozilla.org/privatebrowsing;1"].
                         getService(Ci.nsIPrivateBrowsingService);


var gTabWaitCount = 0;
var gTabCounter = 0;

var gTestSteps = [
  function() {
    info("Running step 1");
    for (let i = 0; i < 10; i++) {
      let tab = gBrowser.addTab();
      loadTab(tab, TEST_URL_BASES[0] + (++gTabCounter));
    }
  },
  function() {
    info("Running step 2");
    gBrowser.selectTabAtIndex(1);
    gBrowser.removeCurrentTab();
    gBrowser.selectTabAtIndex(1);
    gBrowser.removeCurrentTab();
    for (let i = 1; i < gBrowser.tabs.length; i++)
      loadTab(gBrowser.tabs[i], TEST_URL_BASES[1] + (++gTabCounter));
  },
  function() {
    info("Running step 3");
    for (let i = 1; i < gBrowser.tabs.length; i++)
      loadTab(gBrowser.tabs[i], TEST_URL_BASES[0] + gTabCounter);
  },
  function() {
    info("Running step 4");
    let ps = Services.prefs;
    ps.setBoolPref("browser.privatebrowsing.keep_current_session", true);
    ps.setBoolPref("browser.tabs.warnOnClose", false);

    gPrivateBrowsing.privateBrowsingEnabled = true;

    executeSoon(function() {
      ensure_opentabs_match_db();
      nextStep();
    });
  },
  function() {
    info("Running step 5");
    gPrivateBrowsing.privateBrowsingEnabled = false;

    executeSoon(function() {
      let ps = Services.prefs;
      try {
        ps.clearUserPref("browser.privatebrowsing.keep_current_session");
      } catch (ex) {}
      try {
        ps.clearUserPref("browser.tabs.warnOnClose");
      } catch (ex) {}

      ensure_opentabs_match_db();
      nextStep()
    });
  },
  function() {
    info("Running step 6 - ensure we don't register subframes as open pages");
    let tab = gBrowser.addTab();
    tab.linkedBrowser.addEventListener("load", function () {
      tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
      
      executeSoon(function () {
        tab.linkedBrowser.addEventListener("load", function (e) {
          tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
            ensure_opentabs_match_db();
            nextStep()
        }, true);
        tab.linkedBrowser.contentDocument.querySelector("iframe").src = "http://test2.example.org/";
      });
    }, true);
    tab.linkedBrowser.loadURI('data:text/html,<body><iframe src=""></iframe></body>');
  },
  function() {
    info("Running step 7 - remove tab immediately");
    let tab = gBrowser.addTab("about:blank");
    gBrowser.removeTab(tab);
    ensure_opentabs_match_db();
    nextStep();
  },
];



function test() {
  waitForExplicitFinish();
  nextStep();
}

function loadTab(tab, url) {
  
  let visited = false;

  tab.linkedBrowser.addEventListener("load", function (event) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    Services.obs.addObserver(
      function (aSubject, aTopic, aData) {
        if (url != aSubject.QueryInterface(Ci.nsIURI).spec)
          return;
        Services.obs.removeObserver(arguments.callee, aTopic);
        if (--gTabWaitCount > 0)
          return;
        is(gTabWaitCount, 0,
           "sanity check, gTabWaitCount should not be decremented below 0");

        try {
          ensure_opentabs_match_db();
        } catch (e) {
          ok(false, "exception from ensure_openpages_match_db: " + e);
        }

        executeSoon(nextStep);
      }, "uri-visit-saved", false);
  }, true);

  gTabWaitCount++;
  tab.linkedBrowser.loadURI(url);
}

function nextStep() {
  if (gTestSteps.length == 0) {
    while (gBrowser.tabs.length > 1) {
      gBrowser.selectTabAtIndex(1);
      gBrowser.removeCurrentTab();
    }

    waitForClearHistory(finish);

    return;
  }

  var stepFunc = gTestSteps.shift();
  stepFunc();
}

function ensure_opentabs_match_db() {
  var tabs = {};

  var winEnum = Services.wm.getEnumerator("navigator:browser");
  while (winEnum.hasMoreElements()) {
    let browserWin = winEnum.getNext();
    
    if (browserWin.closed)
      continue;

    for (let i = 0; i < browserWin.gBrowser.tabContainer.childElementCount; i++) {
      let browser = browserWin.gBrowser.getBrowserAtIndex(i);
      let url = browser.currentURI.spec;
      if (!(url in tabs))
        tabs[url] = 1;
      else
        tabs[url]++;
    }
  }

  var db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                              .DBConnection;

  try {
    var stmt = db.createStatement(
                          "SELECT t.url, open_count, IFNULL(p_t.id, p.id) " +
                          "FROM moz_openpages_temp t " +
                          "LEFT JOIN moz_places p ON p.url = t.url " +
                          "LEFT JOIN moz_places_temp p_t ON p_t.url = t.url");
  } catch (e) {
    ok(false, "error creating db statement: " + e);
    return;
  }

  var dbtabs = [];
  try {
    while (stmt.executeStep()) {
      ok(stmt.row.url in tabs,
         "url is in db, should be in tab: " + stmt.row.url);
      is(tabs[stmt.row.url], stmt.row.open_count,
         "db count (" + stmt.row.open_count + ") " +
         "should match actual open tab count " +
         "(" + tabs[stmt.row.url] + "): " + stmt.row.url);
      dbtabs.push(stmt.row.url);
    }
  } finally {
    stmt.finalize();
  }

  for (let url in tabs) {
    ok(dbtabs.indexOf(url) > -1,
       "tab is open (" + tabs[url] + " times) and should recorded in db: " + url);
  }
}




function waitForClearHistory(aCallback) {
  const TOPIC_EXPIRATION_FINISHED = "places-expiration-finished";
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      Services.obs.removeObserver(this, TOPIC_EXPIRATION_FINISHED);
      aCallback();
    }
  };
  Services.obs.addObserver(observer, TOPIC_EXPIRATION_FINISHED, false);

  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  hs.QueryInterface(Ci.nsIBrowserHistory).removeAllPages();
}
