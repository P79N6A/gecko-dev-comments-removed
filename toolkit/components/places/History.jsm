



"use strict";

























































this.EXPORTED_SYMBOLS = [ "History" ];

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");
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
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
Cu.importGlobalProperties(["URL"]);







const NOTIFICATION_CHUNK_SIZE = 300;




XPCOMUtils.defineLazyGetter(this, "operationsBarrier", () =>
  new AsyncShutdown.Barrier("Sqlite.jsm: wait until all connections are closed")
);




XPCOMUtils.defineLazyGetter(this, "DBConnPromised",
  () => new Promise((resolve) => {
    Sqlite.wrapStorageConnection({ connection: PlacesUtils.history.DBConnection } )
          .then(db => {
      try {
        Sqlite.shutdown.addBlocker(
          "Places History.jsm: Closing database wrapper",
          Task.async(function*() {
            yield operationsBarrier.wait();
            gIsClosed = true;
            yield db.close();
          }),
          () => ({
            fetchState: () => ({
              isClosed: gIsClosed,
              operations: operationsBarrier.state,
            })
          }));
      } catch (ex) {
        
        
        db.close();
        throw ex;
      }
      resolve(db);
    });
  })
);




let gIsClosed = false;
function ensureModuleIsOpen() {
  if (gIsClosed) {
    throw new Error("History.jsm has been shutdown");
  }
}











function notify(observers, notification, args = []) {
  for (let observer of observers) {
    try {
      observer[notification](...args);
    } catch (ex) {}
  }
}

this.History = Object.freeze({
  

















  fetch: function (guidOrURI) {
    throw new Error("Method not implemented");
  },

  





















































  update: function (infos, onResult) {
    throw new Error("Method not implemented");
  },

  























  remove: function (pages, onResult = null) {
    ensureModuleIsOpen();

    
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
    let normalizedPages = {guids: guids, urls: urls};

    
    

    if (onResult && typeof onResult != "function") {
      throw new TypeError("Invalid function: " + onResult);
    }

    return Task.spawn(function*() {
      let promise = remove(normalizedPages, onResult);

      operationsBarrier.client.addBlocker(
        "History.remove",
        promise,
        {
          
          
          
          fetchState: () => ({
            guids: guids.length,
            urls: normalizedPages.urls.map(u => u.protocol),
          })
        });

      try {
        return (yield promise);
      } finally {
        
        operationsBarrier.client.removeBlocker(promise);
      }
    });
  },

  















  hasVisits: function(page, onResult) {
    throw new Error("Method not implemented");
  },

  





  clear() {
    ensureModuleIsOpen();

    return Task.spawn(function* () {
      let promise = clear();
      operationsBarrier.client.addBlocker("History.clear", promise);

      try {
        return (yield promise);
      } finally {
        
        operationsBarrier.client.removeBlocker(promise);
      }
    });
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


let clear = Task.async(function* () {
  let db = yield DBConnPromised;

  
  yield db.execute("DELETE FROM moz_historyvisits");

  
  PlacesUtils.history.clearEmbedVisits();

  
  let observers = PlacesUtils.history.getObservers();
  notify(observers, "onClearHistory");

  
  
  yield db.execute(
    `UPDATE moz_places SET frecency =
     (CASE
      WHEN url BETWEEN 'place:' AND 'place;'
      THEN 0
      ELSE -1
      END)
     WHERE frecency > 0`);

  
  notify(observers, "onManyFrecenciesChanged");
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

    
    let observers = PlacesUtils.history.getObservers();
    let reason = Ci.nsINavHistoryObserver.REASON_DELETED;
    for (let {guid, uri, toRemove} of pages) {
      if (toRemove) {
        notify(observers, "onDeleteURI", [uri, guid, reason]);
      } else {
        notify(observers, "onDeleteVisits", [uri, 0, guid, reason, 0]);
      }
    }
  });

  PlacesUtils.history.clearEmbedVisits();

  return hasPagesToRemove;
});
