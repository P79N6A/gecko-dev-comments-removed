









































start_sync();


var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
var ghist = Cc["@mozilla.org/browser/global-history;2"].
            getService(Ci.nsIGlobalHistory2);
var annosvc = Cc["@mozilla.org/browser/annotation-service;1"].
              getService(Ci.nsIAnnotationService);
var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);
var prefs = Cc["@mozilla.org/preferences-service;1"].
            getService(Ci.nsIPrefBranch);


var observer = {
  onBeginUpdateBatch: function() {
  },
  onEndUpdateBatch: function() {
  },
  onVisit: function(aURI, aVisitID, aTime, aSessionID, aReferringID, aTransitionType) {
  },
  onTitleChanged: function(aURI, aPageTitle) {
  },
  onBeforeDeleteURI: function(aURI) {
  },
  onDeleteURI: function(aURI) {
  },
  onClearHistory: function() {
    this.historyCleared = true;
  },
  onPageChanged: function(aURI, aWhat, aValue) {
  },
  expiredURI: null,
  onPageExpired: function(aURI, aVisitTime, aWholeEntry) {
    this.expiredURI = aURI.spec;
  },
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavBookmarkObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};
histsvc.addObserver(observer, false);


var dbConnection = histsvc.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;


var testURI = uri("http://mozilla.com");
var testAnnoName = "tests/expiration/history";
var testAnnoVal = "foo";
var bookmark = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, testURI, bmsvc.DEFAULT_INDEX, "foo");
var triggerURI = uri("http://foobar.com");


function run_test() {
  



  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName + "Hist", testAnnoVal, 0, annosvc.EXPIRE_WITH_HISTORY);
  annosvc.setPageAnnotation(testURI, testAnnoName + "Never", testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  bhist.removePagesFromHost("mozilla.com", false);

  do_check_eq(bmsvc.getBookmarkURI(bookmark).spec, testURI.spec);
  
  try {
    annosvc.getPageAnnotation(testAnnoName + "Hist");
    do_throw("removePagesFromHost() didn't remove an EXPIRE_WITH_HISTORY annotation");
  } catch(ex) {}
  
  do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName + "Never"), testAnnoVal);
  
  do_check_eq(histsvc.getPageTitle(testURI), "mozilla.com");

  
  annosvc.removePageAnnotation(testURI, testAnnoName + "Never");

  



  var removeAllTestURI = uri("http://removeallpages.com");
  var removeAllTestURINever = uri("http://removeallpagesnever.com");
  histsvc.addVisit(removeAllTestURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  var bmURI = uri("http://bookmarked");
  var bookmark2 = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, bmURI, bmsvc.DEFAULT_INDEX, "foo");
  var placeURI = uri("place:folder=23");
  bhist.addPageWithDetails(placeURI, "place uri", Date.now() * 1000);
  annosvc.setPageAnnotation(removeAllTestURI, testAnnoName + "Hist", testAnnoVal, 0, annosvc.EXPIRE_WITH_HISTORY);
  annosvc.setPageAnnotation(removeAllTestURINever, testAnnoName + "Never", testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  bhist.removeAllPages();

  
  try {
    annosvc.getPageAnnotation(removeAllTestURI, testAnnoName + "Hist");
    do_throw("nsIBrowserHistory.removeAllPages() didn't remove an EXPIRE_WITH_HISTORY annotation");
  } catch(ex) {}
  try {
    annosvc.getPageAnnotation(removeAllTestURINever, testAnnoName + "Never");
    do_throw("nsIBrowserHistory.removePagesFromHost() didn't remove an EXPIRE_NEVER annotation");
  } catch(ex) {}
  
  do_check_neq(histsvc.getPageTitle(placeURI), null);
  
  do_check_neq(histsvc.getPageTitle(bmURI), null);

  
  bmsvc.removeItem(bookmark2);

  





  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);
  annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);
  
  var expireNeverURI = uri("http://expiremenever.com");
  histsvc.addVisit(expireNeverURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(expireNeverURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  histsvc.removeAllPages();  

  
  do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName), testAnnoVal);
  do_check_eq(annosvc.getItemAnnotation(bookmark, testAnnoName), testAnnoVal);
  
    try {
    annosvc.getPageAnnotation(expireNeverURI, testAnnoName);
    do_throw("nsIBrowserHistory.removeAllPages() didn't remove an EXPIRE_NEVER annotation");
  } catch(ex) {}

  
  annosvc.removeItemAnnotation(bookmark, testAnnoName);
  annosvc.removePageAnnotation(testURI, testAnnoName);

  






  
  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WITH_HISTORY);
  
  try {
    annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WITH_HISTORY);
    do_throw("I was able to set an EXPIRE_WITH_HISTORY anno on a bookmark");
  } catch(ex) {}

  histsvc.removeAllPages();

  
  try {
    annosvc.getPageAnnotation(testURI, testAnnoName);
    do_throw("page still had expire_with_history page anno");
  } catch(ex) {}

  







  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);

  
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_DAYS);
  annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_DAYS);

  
  var expirationDate = (Date.now() - (8 * 86400 * 1000)) * 1000;
  dbConnection.executeSimpleSQL("UPDATE moz_annos SET dateAdded = " + expirationDate);
  dbConnection.executeSimpleSQL("UPDATE moz_items_annos SET dateAdded = " + expirationDate);

  
  annosvc.setPageAnnotation(testURI, testAnnoName + "NotExpired", testAnnoVal, 0, annosvc.EXPIRE_DAYS);
  annosvc.setItemAnnotation(bookmark, testAnnoName + "NotExpired", testAnnoVal, 0, annosvc.EXPIRE_DAYS);

  
  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  bhist.removePage(triggerURI);

  
  try {
    do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName + "NotExpired"), testAnnoVal);
  } catch(ex) {
    do_throw("anno < 7 days old was expired!");
  }
  annosvc.removePageAnnotation(testURI, testAnnoName + "NotExpired");
  try {
    do_check_eq(annosvc.getItemAnnotation(bookmark, testAnnoName + "NotExpired"), testAnnoVal);
  } catch(ex) {
    do_throw("item anno < 7 days old was expired!");
  }
  annosvc.removeItemAnnotation(bookmark, testAnnoName + "NotExpired");

  
  try {
    annosvc.getPageAnnotation(testURI, testAnnoName);
    do_throw("page still had days anno");
  } catch(ex) {}
  try {
    annosvc.getItemAnnotation(bookmark, testAnnoName);
    do_throw("bookmark still had days anno");
  } catch(ex) {}

  
  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_DAYS);
  annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_DAYS);
  
  var expirationDate = (Date.now() - (6 * 86400 * 1000)) * 1000;
  dbConnection.executeSimpleSQL("UPDATE moz_annos SET dateAdded = " + expirationDate);
  dbConnection.executeSimpleSQL("UPDATE moz_items_annos SET dateAdded = " + expirationDate);

  
  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  bhist.removePage(triggerURI);

  
  try {
    do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName), testAnnoVal);
  } catch(ex) {
    do_throw("anno < 7 days old was expired!");
  }
  annosvc.removePageAnnotation(testURI, testAnnoName);
  try {
    do_check_eq(annosvc.getItemAnnotation(bookmark, testAnnoName), testAnnoVal);
  } catch(ex) {
    do_throw("item anno < 7 days old was expired!");
  }
  annosvc.removeItemAnnotation(bookmark, testAnnoName);


  
  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WEEKS);
  annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WEEKS);
  
  var expirationDate = (Date.now() - (31 * 86400 * 1000)) * 1000;
  dbConnection.executeSimpleSQL("UPDATE moz_annos SET dateAdded = " + expirationDate);
  dbConnection.executeSimpleSQL("UPDATE moz_items_annos SET dateAdded = " + expirationDate);
  
  annosvc.setPageAnnotation(testURI, testAnnoName + "NotExpired", testAnnoVal, 0, annosvc.EXPIRE_WEEKS);
  annosvc.setItemAnnotation(bookmark, testAnnoName + "NotExpired", testAnnoVal, 0, annosvc.EXPIRE_WEEKS);

  
  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  bhist.removePage(triggerURI);

  
  try {
    do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName + "NotExpired"), testAnnoVal);
  } catch(ex) {
    do_throw("anno < 30 days old was expired!");
  }
  annosvc.removePageAnnotation(testURI, testAnnoName + "NotExpired");
  try {
    do_check_eq(annosvc.getItemAnnotation(bookmark, testAnnoName + "NotExpired"), testAnnoVal);
  } catch(ex) {
    do_throw("item anno < 30 days old was expired!");
  }
  annosvc.removeItemAnnotation(bookmark, testAnnoName + "NotExpired");

  try {
    annosvc.getPageAnnotation(testURI, testAnnoName);
    do_throw("page still had weeks anno");
  } catch(ex) {}
  try {
    annosvc.getItemAnnotation(bookmark, testAnnoName);
    do_throw("bookmark still had weeks anno");
  } catch(ex) {}

  
  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WEEKS);
  annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WEEKS);
  
  var expirationDate = (Date.now() - (29 * 86400 * 1000)) * 1000;
  dbConnection.executeSimpleSQL("UPDATE moz_annos SET dateAdded = " + expirationDate);
  dbConnection.executeSimpleSQL("UPDATE moz_items_annos SET dateAdded = " + expirationDate);

  
  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  bhist.removePage(triggerURI);

  
  try {
    do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName), testAnnoVal);
  } catch(ex) {
    do_throw("anno < 30 days old was expired!");
  }
  annosvc.removePageAnnotation(testURI, testAnnoName);
  try {
    do_check_eq(annosvc.getItemAnnotation(bookmark, testAnnoName), testAnnoVal);
  } catch(ex) {
    do_throw("item anno < 30 days old was expired!");
  }
  annosvc.removeItemAnnotation(bookmark, testAnnoName);

  
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_MONTHS);
  annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_MONTHS);
  var expirationDate = (Date.now() - (181 * 86400 * 1000)) * 1000;
  dbConnection.executeSimpleSQL("UPDATE moz_annos SET dateAdded = " + expirationDate);
  dbConnection.executeSimpleSQL("UPDATE moz_items_annos SET dateAdded = " + expirationDate);
  
  annosvc.setPageAnnotation(testURI, testAnnoName + "NotExpired", testAnnoVal, 0, annosvc.EXPIRE_MONTHS);
  annosvc.setItemAnnotation(bookmark, testAnnoName + "NotExpired", testAnnoVal, 0, annosvc.EXPIRE_MONTHS);

  
  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  bhist.removePage(triggerURI);

  
  try {
    do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName + "NotExpired"), testAnnoVal);
  } catch(ex) {
    do_throw("anno < 180 days old was expired!");
  }
  annosvc.removePageAnnotation(testURI, testAnnoName + "NotExpired");
  try {
    do_check_eq(annosvc.getItemAnnotation(bookmark, testAnnoName + "NotExpired"), testAnnoVal);
  } catch(ex) {
    do_throw("item anno < 180 days old was expired!");
  }
  annosvc.removeItemAnnotation(bookmark, testAnnoName + "NotExpired");

  try {
    var annoVal = annosvc.getPageAnnotation(testURI, testAnnoName);
    do_throw("page still had months anno");
  } catch(ex) {}
  try {
    annosvc.getItemAnnotation(bookmark, testAnnoName);
    do_throw("bookmark still had months anno");
  } catch(ex) {}

  
  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_MONTHS);
  annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_MONTHS);
  
  var expirationDate = (Date.now() - (179 * 86400 * 1000)) * 1000;
  dbConnection.executeSimpleSQL("UPDATE moz_annos SET dateAdded = " + expirationDate);
  dbConnection.executeSimpleSQL("UPDATE moz_items_annos SET dateAdded = " + expirationDate);

  
  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  bhist.removePage(triggerURI);

  
  try {
    do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName), testAnnoVal);
  } catch(ex) {
    do_throw("anno < 180 days old was expired!");
  }
  annosvc.removePageAnnotation(testURI, testAnnoName);
  try {
    do_check_eq(annosvc.getItemAnnotation(bookmark, testAnnoName), testAnnoVal);
  } catch(ex) {
    do_throw("item anno < 180 days old was expired!");
  }
  annosvc.removeItemAnnotation(bookmark, testAnnoName);

  
  

  
  
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_DAYS);
  annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_DAYS);
  
  var expirationDate = (Date.now() - (8 * 86400 * 1000)) * 1000;
  dbConnection.executeSimpleSQL("UPDATE moz_annos SET dateAdded = " + expirationDate);
  dbConnection.executeSimpleSQL("UPDATE moz_items_annos SET dateAdded = " + expirationDate);
  
  annosvc.setPageAnnotation(testURI, testAnnoName, "mod", 0, annosvc.EXPIRE_DAYS);
  annosvc.setItemAnnotation(bookmark, testAnnoName, "mod", 0, annosvc.EXPIRE_DAYS);
  
  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  bhist.removePage(triggerURI);
  
  try {
    var annoVal = annosvc.getPageAnnotation(testURI, testAnnoName);
  } catch(ex) {
    do_throw("page lost a days anno that was 8 days old, but was modified today");
  }
  try {
    annosvc.getItemAnnotation(bookmark, testAnnoName);
  } catch(ex) {
    do_throw("bookmark lost a days anno that was 8 days old, but was modified today");
  }

  
  var expirationDate = (Date.now() - (8 * 86400 * 1000)) * 1000;
  dbConnection.executeSimpleSQL("UPDATE moz_annos SET lastModified = " + expirationDate);
  dbConnection.executeSimpleSQL("UPDATE moz_items_annos SET lastModified = " + expirationDate);
  
  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  bhist.removePage(triggerURI);
  
  try {
    var annoVal = annosvc.getPageAnnotation(testURI, testAnnoName);
    do_throw("page still had a days anno that was modified 8 days ago");
  } catch(ex) {}
  try {
    annosvc.getItemAnnotation(bookmark, testAnnoName);
    do_throw("bookmark lost a days anno that was modified 8 days ago");
  } catch(ex) {}

  
  bmsvc.removeItem(bookmark);
  annosvc.removePageAnnotations(testURI);
  annosvc.removePageAnnotations(triggerURI);

  startIncrementalExpirationTests();
}




function startIncrementalExpirationTests() {
  do_test_pending();
  startExpireNeither();
}






















function startExpireNeither() {
  dump("startExpireNeither()\n");
  
  histsvc.removeAllPages();
  observer.expiredURI = null;

  
  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);
  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(triggerURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  
  prefs.setIntPref("browser.history_expire_sites", 2);
  
  prefs.setIntPref("browser.history_expire_days_min", 2);
  
  prefs.setIntPref("browser.history_expire_days", 3);

  

  
  do_timeout(600, "checkExpireNeither();");
}

function checkExpireNeither() {
  dump("checkExpireNeither()\n");
  try {
    do_check_eq(observer.expiredURI, null);
    do_check_eq(annosvc.getPageAnnotationNames(testURI, {}).length, 1);
    do_check_eq(annosvc.getPageAnnotationNames(triggerURI, {}).length, 1);
  } catch(ex) {
    do_throw(ex);
  }
  dump("done incremental expiration test 1\n");
  startExpireDaysOnly();
}





















function startExpireDaysOnly() {
  dump("startExpireDaysOnly()\n");
  
  histsvc.removeAllPages();
  observer.expiredURI = null;

  
  histsvc.addVisit(testURI, (Date.now() - (86400 * 4 * 1000)) * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  
  var unexpirableURI = uri("http://unexpirable.com");
  histsvc.addVisit(unexpirableURI, (Date.now() - (86400 * 1000)) * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(unexpirableURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  
  prefs.setIntPref("browser.history_expire_sites", 20);
  
  prefs.setIntPref("browser.history_expire_days_min", 2);
  
  prefs.setIntPref("browser.history_expire_days", 3);

  

  
  do_timeout(600, "checkExpireDaysOnly();");
}

function checkExpireDaysOnly() {
  try {
    
    do_check_eq(observer.expiredURI, testURI.spec);
    do_check_eq(annosvc.getPageAnnotationNames(testURI, {}).length, 0);

    
    do_check_neq(histsvc.getPageTitle(unexpirableURI), null);
    do_check_eq(annosvc.getPageAnnotationNames(unexpirableURI, {}).length, 1);
  } catch(ex) {}
  dump("done expiration test 2\n");
  startExpireBoth();
}






















function startExpireBoth() {
  dump("starting expiration test 3: both criteria met\n");
  
  histsvc.removeAllPages();
  observer.expiredURI = null;

  
  
  
  var bmId = bmsvc.insertBookmark(bmsvc.toolbarFolder, testURI, bmsvc.DEFAULT_INDEX, "foo");
  bmsvc.removeItem(bmId);

  
  
  var age = (Date.now() - (86400 * 2 * 1000)) * 1000;
  dump("AGE: " + age + "\n");
  histsvc.addVisit(testURI, age, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(triggerURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  
  prefs.setIntPref("browser.history_expire_sites", 1);
  
  prefs.setIntPref("browser.history_expire_days", 3);
  
  prefs.setIntPref("browser.history_expire_days_min", 1);

  

  
  do_timeout(600, "checkExpireBoth();"); 
}

function checkExpireBoth() {
  try {
    do_check_eq(observer.expiredURI, testURI.spec);
    do_check_eq(annosvc.getPageAnnotationNames(testURI, {}).length, 0);
    do_check_eq(annosvc.getPageAnnotationNames(triggerURI, {}).length, 1);
  } catch(ex) {}
  dump("done expiration test 3\n");
  startExpireNeitherOver()
}























function startExpireNeitherOver() {
  dump("startExpireNeitherOver()\n");
  
  histsvc.removeAllPages();
  observer.expiredURI = null;

  
  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  histsvc.addVisit(triggerURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(triggerURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  
  prefs.setIntPref("browser.history_expire_sites", 1);
  
  prefs.setIntPref("browser.history_expire_days_min", 2);
  
  prefs.setIntPref("browser.history_expire_days", 3);

  

  
  do_timeout(600, "checkExpireNeitherOver();");
}

function checkExpireNeitherOver() {
  dump("checkExpireNeitherOver()\n");
  try {
    do_check_eq(observer.expiredURI, null);
    do_check_eq(annosvc.getPageAnnotationNames(testURI, {}).length, 1);
    do_check_eq(annosvc.getPageAnnotationNames(triggerURI, {}).length, 1);
  } catch(ex) {
    do_throw(ex);
  }
  dump("done incremental expiration test 4\n");
  startExpireHistoryDisabled();
}


















function startExpireHistoryDisabled() {
  dump("startExpireHistoryDisabled()\n");
  
  histsvc.removeAllPages();
  observer.expiredURI = null;

  
  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  
  prefs.setIntPref("browser.history_expire_days", 0);

  

  
  do_timeout(600, "checkExpireHistoryDisabled();");
}

function checkExpireHistoryDisabled() {
  dump("checkExpireHistoryDisabled()\n");
  try {
    do_check_eq(observer.expiredURI, testURI.spec);
    do_check_eq(annosvc.getPageAnnotationNames(testURI, {}).length, 0);
  } catch(ex) {
    do_throw(ex);
  }
  dump("done incremental expiration test 5\n");
  startExpireBadPrefs();
}



















function startExpireBadPrefs() {
  dump("startExpireBadPrefs()\n");
  
  histsvc.removeAllPages();
  observer.expiredURI = null;

  
  var age = (Date.now() - (86400 * 10 * 1000)) * 1000;
  histsvc.addVisit(testURI, age, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);

  
  prefs.setIntPref("browser.history_expire_days_min", 20);
  
  prefs.setIntPref("browser.history_expire_days", 1);

  

  
  do_timeout(600, "checkExpireBadPrefs();");
}

function checkExpireBadPrefs() {
  dump("checkExpireBadPrefs()\n");
  try {
    do_check_eq(observer.expiredURI, null);
    do_check_eq(annosvc.getPageAnnotationNames(testURI, {}).length, 1);
  } catch(ex) {
    do_throw(ex);
  }
  dump("done incremental expiration test 6\n");
  finish_test();
}
