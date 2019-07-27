



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
const ONRESULT_CHUNK_SIZE = 300;




XPCOMUtils.defineLazyGetter(this, "operationsBarrier", () =>
  new AsyncShutdown.Barrier("History.jsm: wait until all connections are closed")
);




 XPCOMUtils.defineLazyGetter(this, "DBConnPromised", () =>
  Task.spawn(function*() {
    let db = yield PlacesUtils.promiseWrappedConnection();
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
        })
      );
    } catch (ex) {
      
      
      db.close();
      throw ex;
    }
    return db;
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

  




























  removeVisitsByFilter: function(filter, onResult = null) {
    ensureModuleIsOpen();

    if (!filter || typeof filter != "object") {
      throw new TypeError("Expected a filter");
    }

    let hasBeginDate = "beginDate" in filter;
    let hasEndDate = "endDate" in filter;
    if (hasBeginDate) {
      ensureDate(filter.beginDate);
    }
    if (hasEndDate) {
      ensureDate(filter.endDate);
    }
    if (hasBeginDate && hasEndDate && filter.beginDate > filter.endDate) {
      throw new TypeError("`beginDate` should be at least as old as `endDate`");
    }
    if (!hasBeginDate && !hasEndDate) {
      throw new TypeError("Expected a non-empty filter");
    }

    if (onResult && typeof onResult != "function") {
      throw new TypeError("Invalid function: " + onResult);
    }

    return Task.spawn(function*() {
      let promise = removeVisitsByFilter(filter, onResult);

      operationsBarrier.client.addBlocker(
        "History.removeVisitsByFilter",
        promise
      );

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




function ensureDate(arg) {
  if (!arg || typeof arg != "object" || arg.constructor.name != "Date") {
    throw new TypeError("Expected a Date, got " + arg);
  }
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










let removePagesById = Task.async(function*(db, idList) {
  if (idList.length == 0) {
    return;
  }
  yield db.execute(`DELETE FROM moz_places
                    WHERE id IN ( ${ sqlList(idList) } )`);
});






















let cleanupPages = Task.async(function*(db, pages) {
  yield invalidateFrecencies(db, [p.id for (p of pages) if (p.hasForeign || p.hasVisits)]);
  yield removePagesById(db, [p.id for (p of pages) if (!p.hasForeign && !p.hasVisits)]);
});


















let notifyCleanup = Task.async(function*(db, pages) {
  let notifiedCount = 0;
  let observers = PlacesUtils.history.getObservers();

  let reason = Ci.nsINavHistoryObserver.REASON_DELETED;

  for (let page of pages) {
    let uri = NetUtil.newURI(page.url.href);
    let guid = page.guid;
    if (page.hasVisits) {
      
      
      continue;
    }
    if (page.hasForeign) {
      
      
      notify(observers, "onDeleteVisits",
        [uri, 0, guid, reason, -1]);
    } else {
      
      notify(observers, "onDeleteURI",
        [uri, guid, reason]);
    }
    if (++notifiedCount % NOTIFICATION_CHUNK_SIZE == 0) {
      
      
      yield Promise.resolve();
    }
  }
});











let notifyOnResult = Task.async(function*(data, onResult) {
  if (!onResult) {
    return;
  }
  let notifiedCount = 0;
  for (let info of data) {
    try {
      onResult(info);
    } catch (ex) {
      
      Promise.reject(ex);
    }
    if (++notifiedCount % ONRESULT_CHUNK_SIZE == 0) {
      
      
      yield Promise.resolve();
    }
  }
});


let removeVisitsByFilter = Task.async(function*(filter, onResult = null) {
  let db = yield DBConnPromised;

  
  
  
  let dates = {
    conditions: [],
    args: {},
  };
  if ("beginDate" in filter) {
    dates.conditions.push("visit_date >= :begin * 1000");
    dates.args.begin = Number(filter.beginDate);
  }
  if ("endDate" in filter) {
    dates.conditions.push("visit_date <= :end * 1000");
    dates.args.end = Number(filter.endDate);
  }

  let visitsToRemove = [];
  let pagesToInspect = new Set();
  let onResultData = onResult ? [] : null;

  yield db.executeCached(
    `SELECT id, place_id, visit_date / 1000 AS date, visit_type FROM moz_historyvisits
     WHERE ${ dates.conditions.join(" AND ") }`,
     dates.args,
     row => {
       let id = row.getResultByName("id");
       let place_id = row.getResultByName("place_id");
       visitsToRemove.push(id);
       pagesToInspect.add(place_id);

       if (onResult) {
         onResultData.push({
           date: new Date(row.getResultByName("date")),
           transition: row.getResultByName("visit_type")
         });
       }
     }
  );

  try {
    if (visitsToRemove.length == 0) {
      
      return false;
    }

    let pages = [];
    yield db.executeTransaction(function*() {
      
      yield db.execute(`DELETE FROM moz_historyvisits
                        WHERE id IN (${ sqlList(visitsToRemove) } )`);

      
      yield db.execute(
        `SELECT id, url, guid,
          (foreign_count != 0) AS has_foreign,
          (last_visit_date NOTNULL) as has_visits
         FROM moz_places
         WHERE id IN (${ sqlList([...pagesToInspect]) })`,
         null,
         row => {
           let page = {
             id:  row.getResultByName("id"),
             guid: row.getResultByName("guid"),
             hasForeign: row.getResultByName("has_foreign"),
             hasVisits: row.getResultByName("has_visits"),
             url: new URL(row.getResultByName("url")),
           };
           pages.push(page);
         });

      
      yield cleanupPages(db, pages);
    });

    notifyCleanup(db, pages);
    notifyOnResult(onResultData, onResult); 
  } finally {
    
    PlacesUtils.history.clearEmbedVisits();
  }

  return visitsToRemove.length != 0;
});



let remove = Task.async(function*({guids, urls}, onResult = null) {
  let db = yield DBConnPromised;
  
  let query =
    `SELECT id, url, guid, foreign_count, title, frecency FROM moz_places
     WHERE guid IN (${ sqlList(guids) })
        OR url  IN (${ sqlList(urls)  })
     `;

  let onResultData = onResult ? [] : null;
  let pages = [];
  let hasPagesToKeep = false;
  let hasPagesToRemove = false;
  yield db.execute(query, null, Task.async(function*(row) {
    let hasForeign = row.getResultByName("foreign_count") != 0;
    if (hasForeign) {
      hasPagesToKeep = true;
    } else {
      hasPagesToRemove = true;
    }
    let id = row.getResultByName("id");
    let guid = row.getResultByName("guid");
    let url = row.getResultByName("url");
    let page = {
      id,
      guid,
      hasForeign,
      hasVisits: false,
      url: new URL(url),
    };
    pages.push(page);
    if (onResult) {
      onResultData.push({
        guid: guid,
        title: row.getResultByName("title"),
        frecency: row.getResultByName("frecency"),
        url: new URL(url)
      });
    }
  }));

  try {
    if (pages.length == 0) {
      
      return false;
    }

    yield db.executeTransaction(function*() {
      
      yield db.execute(`DELETE FROM moz_historyvisits
                        WHERE place_id IN (${ sqlList([p.id for (p of pages)]) })
                       `);

      
      yield cleanupPages(db, pages);
    });

    notifyCleanup(db, pages);
    notifyOnResult(onResultData, onResult); 
  } finally {
    
    PlacesUtils.history.clearEmbedVisits();
  }

  return hasPagesToRemove;
});
