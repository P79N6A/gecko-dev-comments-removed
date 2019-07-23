








































try {
  var bhist = Cc["@mozilla.org/browser/global-history;2"].getService(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


try {
  var annosvc= Cc["@mozilla.org/browser/annotation-service;1"].getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
} 


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


var observer = {
  onBeginUpdateBatch: function() {
  },
  onEndUpdateBatch: function() {
  },
  onVisit: function(aURI, aVisitID, aTime, aSessionID, aReferringID, aTransitionType) {
  },
  onTitleChanged: function(aURI, aPageTitle) {
  },
  onDeleteURI: function(aURI) {
  },
  onClearHistory: function() {
    this.historyCleared = true;
  },
  onPageChanged: function(aURI, aWhat, aValue) {
  },
  expiredURIs: [],
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


var dirService = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
var dbFile = dirService.get("ProfD", Ci.nsIFile);
dbFile.append("places.sqlite");

var dbService = Cc["@mozilla.org/storage/service;1"].getService(Ci.mozIStorageService);
var dbConnection = dbService.openDatabase(dbFile);
  

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
    do_throw("nsIBrowserHistory.removePagesFromHost() didn't remove an EXPIRE_WITH_HISTORY annotation");
  } catch(ex) {}
  do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName + "Never"), testAnnoVal);
  annosvc.removePageAnnotation(testURI, testAnnoName + "Never");

  



  var removeAllTestURI = uri("http://removeallpages.com");
  var removeAllTestURINever = uri("http://removeallpagesnever.com");
  histsvc.addVisit(removeAllTestURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  var bmURI = uri("http://bookmarked");
  bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, bmURI, bmsvc.DEFAULT_INDEX, "foo");
  
  var placeURI = uri("place:folder=23");
  bhist.addPageWithDetails(placeURI, "place uri", Date.now() * 1000);
  annosvc.setPageAnnotation(removeAllTestURI, testAnnoName + "Hist", testAnnoVal, 0, annosvc.EXPIRE_WITH_HISTORY);
  annosvc.setPageAnnotation(removeAllTestURINever, testAnnoName + "Never", testAnnoVal, 0, annosvc.EXPIRE_NEVER);
  bhist.removeAllPages();
  try {
    annosvc.getPageAnnotation(removeAllTestURI, testAnnoName + "Hist");
    do_throw("nsIBrowserHistory.removeAllPages() didn't remove an EXPIRE_WITH_HISTORY annotation");
  } catch(ex) {}
  
  do_check_eq(histsvc.getPageTitle(removeAllTestURI), null);
  try {
    do_check_eq(annosvc.getPageAnnotation(removeAllTestURINever, testAnnoName + "Never"), testAnnoVal);
    annosvc.removePageAnnotation(removeAllTestURINever, testAnnoName + "Never");
  } catch(ex) {
    do_throw("nsIBrowserHistory.removeAllPages deleted EXPIRE_NEVER annos!");
  }
  
  do_check_neq(histsvc.getPageTitle(removeAllTestURINever), null);
  
  do_check_neq(histsvc.getPageTitle(placeURI), null);
  
  do_check_neq(histsvc.getPageTitle(bmURI), null);

  


  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);
  annosvc.setItemAnnotation(bookmark, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_NEVER);
  histsvc.removeAllPages();
  
  do_check_eq(annosvc.getPageAnnotation(testURI, testAnnoName), testAnnoVal);
  do_check_eq(annosvc.getItemAnnotation(bookmark, testAnnoName), testAnnoVal);
  annosvc.removeItemAnnotation(bookmark, testAnnoName);

  


  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WITH_HISTORY);
  histsvc.removeAllPages();
  try {
    annosvc.getPageAnnotation(testURI, testAnnoName);
    do_throw("page still had expire_with_history anno");
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

  startIncrementalExpirationTests();
}




function startIncrementalExpirationTests() {
  startExpireNeither();
}

var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
var ghist = Cc["@mozilla.org/browser/global-history;2"].getService(Ci.nsIGlobalHistory2);






















function startExpireNeither() {
  dump("startExpireNeither()\n");
  
  histsvc.removeAllPages();
  observer.expiredURI = null;

  
  histsvc.addVisit(testURI, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WITH_HISTORY);

  
  prefs.setIntPref("browser.history_expire_sites", 2);
  
  prefs.setIntPref("browser.history_expire_days_min", 2);
  
  prefs.setIntPref("browser.history_expire_days", 3);

  
  ghist.addURI(triggerURI, false, true, null); 

  
  do_test_pending();
  do_timeout(3600, "checkExpireNeither();"); 
}

function checkExpireNeither() {
  dump("checkExpireNeither()\n");
  try {
    do_check_eq(observer.expiredURI, null);
    do_check_eq(annosvc.getPageAnnotationNames(testURI, {}).length, 1);
  } catch(ex) {
    do_throw(ex);
  }
  dump("done incremental expiration test 1\n");
  startExpireDaysOnly();
}




















function startExpireDaysOnly() {
  
  histsvc.removeAllPages();
  observer.expiredURI = null;

  dump("startExpireDaysOnly()\n");

  
  histsvc.addVisit(testURI, (Date.now() - (86400 * 2 * 1000)) * 1000, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WITH_HISTORY);

  
  histsvc.addVisit(uri("http://unexpirable.com"), (Date.now() - (86400 * 1000)) * 1000, null, histsvc.TRANSITION_TYPED, false, 0);

  
  prefs.setIntPref("browser.history_expire_sites", 2);
  
  prefs.setIntPref("browser.history_expire_days_min", 2);
  
  prefs.setIntPref("browser.history_expire_days", 3);

  
  ghist.addURI(triggerURI, false, true, null); 

  
  do_timeout(3600, "checkExpireDaysOnly();"); 
}

function checkExpireDaysOnly() {
  try {
    
    do_check_eq(observer.expiredURI, null);
    do_check_eq(annosvc.getPageAnnotationNames(testURI, {}).length, 1);
    
    do_check_neq(histsvc.getPageTitle(uri("http://unexpirable.com")), null);
  } catch(ex) {}
  dump("done expiration test 2\n");
  startExpireBoth();
}






















function startExpireBoth() {
  
  histsvc.removeAllPages();
  observer.expiredURI = null;
  dump("starting expiration test 3: both criteria met\n");

  
  
  var age = (Date.now() - (86400 * 2 * 1000)) * 1000;
  dump("AGE: " + age + "\n");
  histsvc.addVisit(testURI, age, null, histsvc.TRANSITION_TYPED, false, 0);
  histsvc.addVisit(testURI, age, null, histsvc.TRANSITION_TYPED, false, 0);
  annosvc.setPageAnnotation(testURI, testAnnoName, testAnnoVal, 0, annosvc.EXPIRE_WITH_HISTORY);

  
  prefs.setIntPref("browser.history_expire_sites", 1);
  
  prefs.setIntPref("browser.history_expire_days", 3);
  
  prefs.setIntPref("browser.history_expire_days_min", 1);

  
  ghist.addURI(triggerURI, false, true, null);

  
  do_timeout(3600, "checkExpireBoth();"); 
}

function checkExpireBoth() {
  try {
    do_check_eq(observer.expiredURI, testURI.spec);
    do_check_eq(annosvc.getPageAnnotationNames(testURI, {}).length, 0);
  } catch(ex) {}
  dump("done expiration test 3\n");
  do_test_finished();
}
