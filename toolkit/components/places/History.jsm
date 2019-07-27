



"use strict";

























































this.EXPORTED_SYMBOLS = [ "History" ];

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Sqlite",
                                  "resource://gre/modules/Sqlite.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "gNotifier",
                                   "@mozilla.org/browser/nav-history-service;1",
                                   Ci.nsPIPlacesHistoryListenersNotifier);
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
Cu.importGlobalProperties(["URL"]);




XPCOMUtils.defineLazyGetter(this, "DBConnPromised",
  () => new Promise((resolve) => {
    Sqlite.wrapStorageConnection({ connection: PlacesUtils.history.DBConnection } )
          .then(db => {
      try {
        Sqlite.shutdown.addBlocker("Places History.jsm: Closing database wrapper",
                                   () => db.close());
      } catch (ex) {
        
        
        db.close();
        throw ex;
      }
      resolve(db);
    });
  })
);

this.History = Object.freeze({
  

















  fetch: function (guidOrURI) {
    throw new Error("Method not implemented");
  },

  





















































  update: function (infos, onResult) {
    throw new Error("Method not implemented");
  },

  























  remove: function (pages, onResult = null) {
    
    if (Array.isArray(pages)) {
      if (pages.length == 0) {
        throw new TypeError("Expected at least one page");
      }
    } else {
      pages = [pages];
    }

    let guids = [];
    let urls = [];
    for (let page of pages) {
      
      
      let normalized = normalizeToURLOrGUID(page);
      if (typeof normalized === "string") {
        guids.push(normalized);
      } else {
        urls.push(normalized.href);
      }
    }
    
    

    if (onResult && typeof onResult != "function") {
      throw new TypeError("Invalid function: " + onResult);
    }

    
    return remove({guids: guids, urls: urls}, onResult);
  },

  















  hasVisits: function(page, onResult) {
    throw new Error("Method not implemented");
  },

  




  


  TRANSITION_LINK: Ci.nsINavHistoryService.TRANSITION_LINK,

  





  TRANSITION_TYPED: Ci.nsINavHistoryService.TRANSITION_TYPED,

  


  TRANSITION_BOOKMARK: Ci.nsINavHistoryService.TRANSITION_BOOKMARK,

  





  TRANSITION_EMBED: Ci.nsINavHistoryService.TRANSITION_EMBED,

  


  TRANSITION_REDIRECT_PERMANENT: Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT,

  


  TRANSITION_REDIRECT_TEMPORARY: Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY,

  


  TRANSITION_DOWNLOAD: Ci.nsINavHistoryService.TRANSITION_REDIRECT_DOWNLOAD,

  


  TRANSITION_FRAMED_LINK: Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK,
});










function normalizeToURLOrGUID(key) {
  if (typeof key === "string") {
    
    if (/^[a-zA-Z0-9\-_]{12}$/.test(key)) {
      return key;
    }
    return new URL(key);
  }
  if (key instanceof URL) {
    return key;
  }
  if (key instanceof Ci.nsIURI) {
    return new URL(key.spec);
  }
  throw new TypeError("Invalid url or guid: " + key);
}





function sqlList(list) {
  return list.map(JSON.stringify).join();
}










let invalidateFrecencies = Task.async(function*(db, idList) {
  if (idList.length == 0) {
    return;
  }
  let ids = sqlList(idList);
  yield db.execute(
    `UPDATE moz_places
     SET frecency = NOTIFY_FRECENCY(
       CALCULATE_FRECENCY(id), url, guid, hidden, last_visit_date
     ) WHERE id in (${ ids })`
  );
  yield db.execute(
    `UPDATE moz_places
     SET hidden = 0
     WHERE id in (${ ids })
     AND frecency <> 0`
  );
});



let remove = Task.async(function*({guids, urls}, onResult = null) {
  let db = yield DBConnPromised;

  
  let query =
    `SELECT id, url, guid, foreign_count, title, frecency FROM moz_places
     WHERE guid IN (${ sqlList(guids) })
        OR url  IN (${ sqlList(urls)  })
     `;

  let pages = [];
  let hasPagesToKeep = false;
  let hasPagesToRemove = false;
  yield db.execute(query, null, Task.async(function*(row) {
    let toRemove = row.getResultByName("foreign_count") == 0;
    if (toRemove) {
      hasPagesToRemove = true;
    } else {
      hasPagesToKeep = true;
    }
    let id = row.getResultByName("id");
    let guid = row.getResultByName("guid");
    let url = row.getResultByName("url");
    let page = {
      id: id,
      guid: guid,
      toRemove: toRemove,
      uri: NetUtil.newURI(url),
    };
    pages.push(page);
    if (onResult) {
      let pageInfo = {
        guid: guid,
        title: row.getResultByName("title"),
        frecency: row.getResultByName("frecency"),
        url: new URL(url)
      };
      try {
        yield onResult(pageInfo);
      } catch (ex) {
        
        Promise.reject(ex);
      }
    }
  }));

  if (pages.length == 0) {
    
    return false;
  }

  yield db.executeTransaction(function*() {
    
    yield db.execute(`DELETE FROM moz_historyvisits
                      WHERE place_id IN (${ sqlList([p.id for (p of pages)]) })
                     `);

     
    if (hasPagesToKeep) {
      yield invalidateFrecencies(db, [p.id for (p of pages) if (!p.toRemove)]);
    }

    
    if (hasPagesToRemove) {
      let ids = [p.id for (p of pages) if (p.toRemove)];
      yield db.execute(`DELETE FROM moz_places
                        WHERE id IN (${ sqlList(ids) })
                       `);
    }

    
    for (let {guid, uri, toRemove} of pages) {
      gNotifier.notifyOnPageExpired(
        uri, 
        0, 
        toRemove, 
        guid, 
        Ci.nsINavHistoryObserver.REASON_DELETED, 
        -1 
      );
    }
  });

  PlacesUtils.history.clearEmbedVisits();

  return hasPagesToRemove;
});
