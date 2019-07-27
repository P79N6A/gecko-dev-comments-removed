












let hs = PlacesUtils.history;
let bs = PlacesUtils.bookmarks;
let as = PlacesUtils.annotations;











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
          "WHERE id = ( " +
            "SELECT a.id FROM moz_items_annos a " +
            "JOIN moz_anno_attributes n ON n.id = a.anno_attribute_id " +
            "WHERE a.item_id = :id " +
              "AND n.name = :anno_name " +
            "ORDER BY a.dateAdded DESC LIMIT 1 " +
          ")";
  }
  else if (aIdentifier instanceof Ci.nsIURI){
    
    as.setPageAnnotation(aIdentifier, aName, aValue, 0, aExpirePolicy);
    
    sql = "UPDATE moz_annos SET dateAdded = :expire_date, lastModified = :last_modified " +
          "WHERE id = ( " +
            "SELECT a.id FROM moz_annos a " +
            "JOIN moz_anno_attributes n ON n.id = a.anno_attribute_id " +
            "JOIN moz_places h on h.id = a.place_id " +
            "WHERE h.url = :id " +
            "AND n.name = :anno_name " +
            "ORDER BY a.dateAdded DESC LIMIT 1 " +
          ")";
  }
  else
    do_throw("Wrong identifier type");

  let stmt = DBConn().createStatement(sql);
  stmt.params.id = (typeof(aIdentifier) == "number") ? aIdentifier
                                                     : aIdentifier.spec;
  stmt.params.expire_date = expireDate;
  stmt.params.last_modified = lastModifiedDate;
  stmt.params.anno_name = aName;
  try {
    stmt.executeStep();
  }
  finally {
    stmt.finalize();
  }
}

function run_test() {
  run_next_test();
}

add_task(function test_removeAllPages() {
  
  setInterval(3600); 

  
  setMaxPages(0);

  
  for (let i = 0; i < 5; i++) {
    let pageURI = uri("http://item_anno." + i + ".mozilla.org/");
    
    yield promiseAddVisits({ uri: pageURI });
    let id = bs.insertBookmark(bs.unfiledBookmarksFolder, pageURI,
                               bs.DEFAULT_INDEX, null);
    
    as.setItemAnnotation(id, "persist", "test", 0, as.EXPIRE_NEVER);
    
    as.setPageAnnotation(pageURI, "persist", "test", 0, as.EXPIRE_NEVER);
    
    as.setItemAnnotation(id, "expire_session", "test", 0, as.EXPIRE_SESSION);
    as.setPageAnnotation(pageURI, "expire_session", "test", 0, as.EXPIRE_SESSION);
    
    add_old_anno(id, "expire_days", "test", as.EXPIRE_DAYS, 8);
    add_old_anno(id, "expire_weeks", "test", as.EXPIRE_WEEKS, 31);
    add_old_anno(id, "expire_months", "test", as.EXPIRE_MONTHS, 181);
    add_old_anno(pageURI, "expire_days", "test", as.EXPIRE_DAYS, 8);
    add_old_anno(pageURI, "expire_weeks", "test", as.EXPIRE_WEEKS, 31);
    add_old_anno(pageURI, "expire_months", "test", as.EXPIRE_MONTHS, 181);
  }

  
  for (let i = 0; i < 5; i++) {
    
    
    let pageURI = uri("http://page_anno." + i + ".mozilla.org/");
    yield promiseAddVisits({ uri: pageURI });
    as.setPageAnnotation(pageURI, "expire", "test", 0, as.EXPIRE_NEVER);
    as.setPageAnnotation(pageURI, "expire_session", "test", 0, as.EXPIRE_SESSION);
    add_old_anno(pageURI, "expire_days", "test", as.EXPIRE_DAYS, 8);
    add_old_anno(pageURI, "expire_weeks", "test", as.EXPIRE_WEEKS, 31);
    add_old_anno(pageURI, "expire_months", "test", as.EXPIRE_MONTHS, 181);
  }

  
  
  
  let promise =
      promiseTopicObserved(PlacesUtils.TOPIC_EXPIRATION_FINISHED);
  hs.QueryInterface(Ci.nsIBrowserHistory).removeAllPages();
  yield promise;

  ["expire_days", "expire_weeks", "expire_months", "expire_session",
   "expire"].forEach(function(aAnno) {
    let pages = as.getPagesWithAnnotation(aAnno);
    do_check_eq(pages.length, 0);
  });

  ["expire_days", "expire_weeks", "expire_months", "expire_session",
   "expire"].forEach(function(aAnno) {
    let items = as.getItemsWithAnnotation(aAnno);
    do_check_eq(items.length, 0);
  });

  ["persist"].forEach(function(aAnno) {
    let pages = as.getPagesWithAnnotation(aAnno);
    do_check_eq(pages.length, 5);
  });

  ["persist"].forEach(function(aAnno) {
    let items = as.getItemsWithAnnotation(aAnno);
    do_check_eq(items.length, 5);
    items.forEach(function(aItemId) {
      
      bs.getItemIndex(aItemId);
    });
  });
});
