
















































let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
let as = Cc["@mozilla.org/browser/annotation-service;1"].
         getService(Ci.nsIAnnotationService);











let now = Date.now();
function add_old_anno(aIdentifier, aName, aValue, aExpirePolicy,
                      aAgeInDays, aLastModifiedAgeInDays) {
  let expireDate = (now - (aAgeInDays * 86400 * 1000)) * 1000;
  let lastModifiedDate = 0;
  if (aLastModifiedAgeInDays)
    lastModifiedDate = (now - (aLastModifiedAgeInDays * 86400 * 1000)) * 1000;

  let sql;
  if (typeof(aIdentifier) == "number") {
    
    as.setItemAnnotation(aIdentifier, aName, aValue, 0, aExpirePolicy);
    
    sql = "UPDATE moz_items_annos SET dateAdded = :expire_date, lastModified = :last_modified " +
          "WHERE id = (SELECT id FROM moz_items_annos " +
                      "WHERE item_id = :id " +
                      "ORDER BY dateAdded DESC LIMIT 1)";
  }
  else if (aIdentifier instanceof Ci.nsIURI){
    
    as.setPageAnnotation(aIdentifier, aName, aValue, 0, aExpirePolicy);
    
    sql = "UPDATE moz_annos SET dateAdded = :expire_date, lastModified = :last_modified " +
          "WHERE id = (SELECT a.id FROM moz_annos a " +
                      "LEFT JOIN moz_places h on h.id = a.place_id " +
                      "WHERE h.url = :id " +
                      "ORDER BY a.dateAdded DESC LIMIT 1)";
  }
  else
    do_throw("Wrong identifier type");

  let stmt = DBConn().createStatement(sql);
  stmt.params.id = (typeof(aIdentifier) == "number") ? aIdentifier
                                                     : aIdentifier.spec;
  stmt.params.expire_date = expireDate;
  stmt.params.last_modified = lastModifiedDate;
  try {
    stmt.executeStep();
  }
  finally {
    stmt.finalize();
  }
}

function run_test() {
  
  setInterval(3600); 

  
  setMaxPages(0);

  let now = getExpirablePRTime();
  
  for (let i = 0; i < 5; i++) {
    let pageURI = uri("http://item_anno." + i + ".mozilla.org/");
    hs.addVisit(pageURI, now++, null, hs.TRANSITION_TYPED, false, 0);
    let id = bs.insertBookmark(bs.unfiledBookmarksFolder, pageURI,
                               bs.DEFAULT_INDEX, null);
    
    add_old_anno(id, "persist_days", "test", as.EXPIRE_DAYS, 6);
    
    add_old_anno(id, "persist_lm_days", "test", as.EXPIRE_DAYS, 8, 6);
    
    add_old_anno(id, "expire_days", "test", as.EXPIRE_DAYS, 8);

    
    add_old_anno(id, "persist_weeks", "test", as.EXPIRE_WEEKS, 29);
    
    add_old_anno(id, "persist_lm_weeks", "test", as.EXPIRE_WEEKS, 31, 29);
    
    add_old_anno(id, "expire_weeks", "test", as.EXPIRE_WEEKS, 31);

    
    add_old_anno(id, "persist_months", "test", as.EXPIRE_MONTHS, 179);
    
    add_old_anno(id, "persist_lm_months", "test", as.EXPIRE_MONTHS, 181, 179);
    
    add_old_anno(id, "expire_months", "test", as.EXPIRE_MONTHS, 181);

    
    add_old_anno(pageURI, "persist_days", "test", as.EXPIRE_DAYS, 6);
    
    add_old_anno(pageURI, "persist_lm_days", "test", as.EXPIRE_DAYS, 8, 6);
    
    add_old_anno(pageURI, "expire_days", "test", as.EXPIRE_DAYS, 8);

    
    add_old_anno(pageURI, "persist_weeks", "test", as.EXPIRE_WEEKS, 29);
    
    add_old_anno(pageURI, "persist_lm_weeks", "test", as.EXPIRE_WEEKS, 31, 29);
    
    add_old_anno(pageURI, "expire_weeks", "test", as.EXPIRE_WEEKS, 31);

    
    add_old_anno(pageURI, "persist_months", "test", as.EXPIRE_MONTHS, 179);
    
    add_old_anno(pageURI, "persist_lm_months", "test", as.EXPIRE_MONTHS, 181, 179);
    
    add_old_anno(pageURI, "expire_months", "test", as.EXPIRE_MONTHS, 181);
  }

  
  for (let i = 0; i < 5; i++) {
    let pageURI = uri("http://page_anno." + i + ".mozilla.org/");
    hs.addVisit(pageURI, now++, null, hs.TRANSITION_TYPED, false, 0);
    
    add_old_anno(pageURI, "persist_days", "test", as.EXPIRE_DAYS, 6);
    
    add_old_anno(pageURI, "persist_lm_days", "test", as.EXPIRE_DAYS, 8, 6);
    
    add_old_anno(pageURI, "expire_days", "test", as.EXPIRE_DAYS, 8);

    
    add_old_anno(pageURI, "persist_weeks", "test", as.EXPIRE_WEEKS, 29);
    
    add_old_anno(pageURI, "persist_lm_weeks", "test", as.EXPIRE_WEEKS, 31, 29);
    
    add_old_anno(pageURI, "expire_weeks", "test", as.EXPIRE_WEEKS, 31);

    
    add_old_anno(pageURI, "persist_months", "test", as.EXPIRE_MONTHS, 179);
    
    add_old_anno(pageURI, "persist_lm_months", "test", as.EXPIRE_MONTHS, 181, 179);
    
    add_old_anno(pageURI, "expire_months", "test", as.EXPIRE_MONTHS, 181);
  }

  
  observer = {
    observe: function(aSubject, aTopic, aData) {
      os.removeObserver(observer, PlacesUtils.TOPIC_EXPIRATION_FINISHED);

      ["expire_days", "expire_weeks", "expire_months"].forEach(function(aAnno) {
        let pages = as.getPagesWithAnnotation(aAnno);
        do_check_eq(pages.length, 0);
      });

      ["expire_days", "expire_weeks", "expire_months"].forEach(function(aAnno) {
        let items = as.getItemsWithAnnotation(aAnno);
        do_check_eq(items.length, 0);
      });

      ["persist_days", "persist_lm_days", "persist_weeks", "persist_lm_weeks",
       "persist_months", "persist_lm_months"].forEach(function(aAnno) {
        let pages = as.getPagesWithAnnotation(aAnno);
        do_check_eq(pages.length, 10);
      });

      ["persist_days", "persist_lm_days", "persist_weeks", "persist_lm_weeks",
       "persist_months", "persist_lm_months"].forEach(function(aAnno) {
        let items = as.getItemsWithAnnotation(aAnno);
        do_check_eq(items.length, 5);
      });

      do_test_finished();
    }
  };
  os.addObserver(observer, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  
  force_expiration_step(5);
  do_test_pending();
}
