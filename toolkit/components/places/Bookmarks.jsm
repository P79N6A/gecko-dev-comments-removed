



"use strict";

























































this.EXPORTED_SYMBOLS = [ "Bookmarks" ];

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.importGlobalProperties(["URL"]);

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
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");


const DB_URL_LENGTH_MAX = 65536;
const DB_TITLE_LENGTH_MAX = 4096;

let Bookmarks = Object.freeze({
  



  TYPE_BOOKMARK: 1,
  TYPE_FOLDER: 2,
  TYPE_SEPARATOR: 3,

  



  DEFAULT_INDEX: -1,

  























  insert(info) {
    
    
    let time = (info && info.dateAdded) || new Date();
    let insertInfo = validateBookmarkObject(info,
      { type: { required: true }
      , index: { defaultValue: this.DEFAULT_INDEX }
      , url: { requiredIf: b => b.type == this.TYPE_BOOKMARK
             , validIf: b => b.type == this.TYPE_BOOKMARK }
      , parentGuid: { required: true }
      , keyword: { validIf: b => b.keyword &&
                            b.keyword.length > 0 &&
                            b.type == this.TYPE_BOOKMARK }
      , title: { validIf: b => [ this.TYPE_BOOKMARK
                               , this.TYPE_FOLDER ].indexOf(b.type) != -1 }
      , dateAdded: { defaultValue: time
                   , validIf: b => !b.lastModified ||
                                    b.dateAdded <= b.lastModified }
      , lastModified: { defaultValue: time,
                        validIf: b => (!b.dateAdded && b.lastModified >= time) ||
                                      (b.dateAdded && b.lastModified >= b.dateAdded) }
      });

    return Task.spawn(function* () {
      
      let parent = yield fetchBookmark({ guid: insertInfo.parentGuid });
      if (!parent)
        throw new Error("parentGuid must be valid");

      
      if (insertInfo.index == this.DEFAULT_INDEX ||
          insertInfo.index > parent._childCount) {
        insertInfo.index = parent._childCount;
      }

      let item = yield insertBookmark(insertInfo, parent);

      
      let observers = PlacesUtils.bookmarks.getObservers();
      
      
      let uri = item.hasOwnProperty("url") ? toURI(item.url) : null;
      let itemId = yield PlacesUtils.promiseItemId(item.guid);
      notify(observers, "onItemAdded", [ itemId, parent._id, item.index,
                                         item.type, uri, item.title || null,
                                         toPRTime(item.dateAdded), item.guid,
                                         item.parentGuid ]);

      
      if (item.keyword) {
        notify(observers, "onItemChanged", [ itemId, "keyword", false,
                                             item.keyword,
                                             toPRTime(item.lastModified),
                                             item.type, parent._id, item.guid,
                                             item.parentGuid ]);
      }

      
      let isTagging = parent._parentId == PlacesUtils.tagsFolderId;
      if (isTagging) {
        for (let entry of (yield fetchBookmarksByURL(item))) {
          notify(observers, "onItemChanged", [ entry._id, "tags", false, "",
                                               toPRTime(entry.lastModified),
                                               entry.type, entry._parentId,
                                               entry.guid, entry.parentGuid ]);
        }
      }

      
      return Object.assign({}, item);
    }.bind(this));
  },

  




















  update(info) {
    
    
    
    let updateInfo = validateBookmarkObject(info,
      { guid: { required: true }
      , index: { requiredIf: b => b.hasOwnProperty("parentGuid")
               , validIf: b => b.index >= 0 }
      , parentGuid: { requiredIf: b => b.hasOwnProperty("index") }
      });

    
    if (Object.keys(updateInfo).length < 2)
      throw new Error("Not enough properties to update");

    return Task.spawn(function* () {
      
      let item = yield fetchBookmark(updateInfo);
      if (!item)
        throw new Error("No bookmarks found for the provided GUID");
      if (updateInfo.hasOwnProperty("type") && updateInfo.type != item.type)
        throw new Error("The bookmark type cannot be changed");
      if (updateInfo.hasOwnProperty("dateAdded") &&
          updateInfo.dateAdded.getTime() != item.dateAdded.getTime())
        throw new Error("The bookmark dateAdded cannot be changed");

      
      removeSameValueProperties(updateInfo, item);
      
      if (Object.keys(updateInfo).length < 2) {
        
        return Object.assign({}, item);
      }

      let time = (updateInfo && updateInfo.dateAdded) || new Date();
      updateInfo = validateBookmarkObject(updateInfo,
        { url: { validIf: () => item.type == this.TYPE_BOOKMARK }
        , keyword: { validIf: () => item.type == this.TYPE_BOOKMARK }
        , title: { validIf: () => [ this.TYPE_BOOKMARK
                                  , this.TYPE_FOLDER ].indexOf(item.type) != -1 }
        , lastModified: { defaultValue: new Date()
                        , validIf: b => b.lastModified >= item.dateAdded }
        });

      let db = yield DBConnPromised;
      let parent;
      if (updateInfo.hasOwnProperty("parentGuid")) {
        if (item.type == this.TYPE_FOLDER) {
          
          
          let rows = yield db.executeCached(
            `WITH RECURSIVE
             descendants(did) AS (
               VALUES(:id)
               UNION ALL
               SELECT id FROM moz_bookmarks
               JOIN descendants ON parent = did
               WHERE type = :type
             )
             SELECT guid FROM moz_bookmarks
             WHERE id IN descendants
            `, { id: item._id, type: this.TYPE_FOLDER });
          if ([r.getResultByName("guid") for (r of rows)].indexOf(updateInfo.parentGuid) != -1)
            throw new Error("Cannot insert a folder into itself or one of its descendants");
        }

        parent = yield fetchBookmark({ guid: updateInfo.parentGuid });
        if (!parent)
          throw new Error("No bookmarks found for the provided parentGuid");
      }

      if (updateInfo.hasOwnProperty("index")) {
        
        
        if (!parent)
          parent = yield fetchBookmark({ guid: item.parentGuid });
        
        if (updateInfo.index > parent._childCount)
          updateInfo.index = parent._childCount;
      }

      let updatedItem = yield updateBookmark(updateInfo, item, parent);

      if (item.type == this.TYPE_BOOKMARK &&
          item.url.href != updatedItem.url.href) {
        
        updateFrecency(db, [item.url]).then(null, Cu.reportError);
        updateFrecency(db, [updatedItem.url]).then(null, Cu.reportError);
      }

      
      let observers = PlacesUtils.bookmarks.getObservers();
      
      
      if (info.hasOwnProperty("lastModified") &&
          updateInfo.hasOwnProperty("lastModified") &&
          item.lastModified != updatedItem.lastModified) {
        notify(observers, "onItemChanged", [ updatedItem._id, "lastModified",
                                             false,
                                             `${toPRTime(updatedItem.lastModified)}`,
                                             toPRTime(updatedItem.lastModified),
                                             updatedItem.type,
                                             updatedItem._parentId,
                                             updatedItem.guid,
                                             updatedItem.parentGuid ]);
      }
      if (updateInfo.hasOwnProperty("title")) {
        notify(observers, "onItemChanged", [ updatedItem._id, "title",
                                             false, updatedItem.title,
                                             toPRTime(updatedItem.lastModified),
                                             updatedItem.type,
                                             updatedItem._parentId,
                                             updatedItem.guid,
                                             updatedItem.parentGuid ]);
      }
      if (updateInfo.hasOwnProperty("url")) {
        notify(observers, "onItemChanged", [ updatedItem._id, "uri",
                                             false, updatedItem.url.href,
                                             toPRTime(updatedItem.lastModified),
                                             updatedItem.type,
                                             updatedItem._parentId,
                                             updatedItem.guid,
                                             updatedItem.parentGuid ]);
      }
      if (updateInfo.hasOwnProperty("keyword")) {
        notify(observers, "onItemChanged", [ updatedItem._id, "keyword",
                                             false, updatedItem.keyword,
                                             toPRTime(updatedItem.lastModified),
                                             updatedItem.type,
                                             updatedItem._parentId,
                                             updatedItem.guid,
                                             updatedItem.parentGuid ]);
      }
      
      if (item.parentGuid != updatedItem.parentGuid ||
          item.index != updatedItem.index) {
        notify(observers, "onItemMoved", [ updatedItem._id, item._parentId,
                                           item.index, updatedItem._parentId,
                                           updatedItem.index, updatedItem.type,
                                           updatedItem.guid, item.parentGuid,
                                           updatedItem.newParentGuid ]);
      }

      
      return Object.assign({}, updatedItem);
    }.bind(this));
  },

  











  remove(guidOrInfo) {
    let info = guidOrInfo;
    if (!info)
      throw new Error("Input should be a valid object");
    if (typeof(guidOrInfo) != "object") {
      info = { guid: guidOrInfo };
    }

    
    
    let removeInfo = validateBookmarkObject(info);

    return Task.spawn(function* () {
      let item = yield fetchBookmark(removeInfo);
      if (!item)
        throw new Error("No bookmarks found for the provided GUID.");

      
      if (!item._parentId || item._parentId == PlacesUtils.placesRootId)
        throw new Error("It's not possible to remove Places root folders.");

      item = yield removeBookmark(item);

      
      let observers = PlacesUtils.bookmarks.getObservers();
      let uri = item.hasOwnProperty("url") ? toURI(item.url) : null;
      notify(observers, "onItemRemoved", [ item._id, item._parentId, item.index,
                                           item.type, uri, item.guid,
                                           item.parentGuid ]);

      let isUntagging = item._grandParentId == PlacesUtils.tagsFolderId;
      if (isUntagging) {
        for (let entry of (yield fetchBookmarksByURL(item))) {
          notify(observers, "onItemChanged", [ entry._id, "tags", false, "",
                                               toPRTime(entry.lastModified),
                                               entry.type, entry._parentId,
                                               entry.guid, entry.parentGuid ]);
        }
      }

      
      return Object.assign({}, item);
    });
  },

  







  eraseEverything: Task.async(function* () {
    let db = yield DBConnPromised;

    yield db.executeTransaction(function* () {
      let rows = yield db.executeCached(
        `WITH RECURSIVE
         descendants(did) AS (
           SELECT id FROM moz_bookmarks
           WHERE parent IN (SELECT folder_id FROM moz_bookmarks_roots
                            WHERE root_name IN ("toolbar", "menu", "unfiled"))
           UNION ALL
           SELECT id FROM moz_bookmarks
           JOIN descendants ON parent = did
         )
         SELECT b.id AS _id, b.parent AS _parentId, b.position AS 'index',
                b.type, url, b.guid, p.guid AS parentGuid, b.dateAdded,
                b.lastModified, b.title, p.parent AS _grandParentId,
                NULL AS _childCount, NULL AS keyword
         FROM moz_bookmarks b
         JOIN moz_bookmarks p ON p.id = b.parent
         LEFT JOIN moz_places h ON b.fk = h.id
         WHERE b.id IN descendants
        `);
      let items = rowsToItemsArray(rows);

      yield db.executeCached(
        `WITH RECURSIVE
         descendants(did) AS (
           SELECT id FROM moz_bookmarks
           WHERE parent IN (SELECT folder_id FROM moz_bookmarks_roots
                            WHERE root_name IN ("toolbar", "menu", "unfiled"))
           UNION ALL
           SELECT id FROM moz_bookmarks
           JOIN descendants ON parent = did
         )
         DELETE FROM moz_bookmarks WHERE id IN descendants
        `);

      
      yield removeOrphanAnnotations(db);
      yield removeOrphanKeywords(db);

      

      
      yield db.executeCached(
        `UPDATE moz_bookmarks SET lastModified = :time
         WHERE id IN (SELECT folder_id FROM moz_bookmarks_roots
                      WHERE root_name IN ("places", "toolbar", "menu", "unfiled"));
        `, { time: toPRTime(new Date()) });

      let urls = [for (item of items) if (item.url) item.url];
      updateFrecency(db, urls).then(null, Cu.reportError);

      
      
      

      
      let observers = PlacesUtils.bookmarks.getObservers();
      for (let item of items.reverse()) {
        let uri = item.hasOwnProperty("url") ? toURI(item.url) : null;
        notify(observers, "onItemRemoved", [ item._id, item._parentId,
                                             item.index, item.type, uri,
                                             item.guid, item.parentGuid ]);

        let isUntagging = item._grandParentId == PlacesUtils.tagsFolderId;
        if (isUntagging) {
          for (let entry of (yield fetchBookmarksByURL(item))) {
            notify(observers, "onItemChanged", [ entry._id, "tags", false, "",
                                                 toPRTime(entry.lastModified),
                                                 entry.type, entry._parentId,
                                                 entry.guid, entry.parentGuid ]);
          }
        }
      }
    });
  }),

  






































  fetch(guidOrInfo, onResult=null) {
    if (onResult && typeof onResult != "function")
      throw new Error("onResult callback must be a valid function");
    let info = guidOrInfo;
    if (!info)
      throw new Error("Input should be a valid object");
    if (typeof(info) != "object")
      info = { guid: guidOrInfo };

    
    let conditionsCount = [
      v => v.hasOwnProperty("guid"),
      v => v.hasOwnProperty("parentGuid") && v.hasOwnProperty("index"),
      v => v.hasOwnProperty("url"),
      v => v.hasOwnProperty("keyword")
    ].reduce((old, fn) => old + fn(info)|0, 0);
    if (conditionsCount != 1)
      throw new Error(`Unexpected number of conditions provided: ${conditionsCount}`);

    
    
    let fetchInfo = validateBookmarkObject(info,
      { parentGuid: { requiredIf: b => b.hasOwnProperty("index") }
      , index: { requiredIf: b => b.hasOwnProperty("parentGuid")
               , validIf: b => typeof(b.index) == "number" &&
                               b.index >= 0 }
      , keyword: { validIf: b => typeof(b.keyword) == "string" &&
                                 b.keyword.length > 0 }
      });

    return Task.spawn(function* () {
      let results;
      if (fetchInfo.hasOwnProperty("guid"))
        results = yield fetchBookmark(fetchInfo);
      else if (fetchInfo.hasOwnProperty("parentGuid") && fetchInfo.hasOwnProperty("index"))
        results = yield fetchBookmarkByPosition(fetchInfo);
      else if (fetchInfo.hasOwnProperty("url"))
        results = yield fetchBookmarksByURL(fetchInfo);
      else if (fetchInfo.hasOwnProperty("keyword"))
        results = yield fetchBookmarksByKeyword(fetchInfo);

      if (!results)
        return null;

      if (!Array.isArray(results))
        results = [results];
      
      results = results.map(r => Object.assign({}, r));

      
      
      
      if (onResult) {
        for (let result of results) {
          try {
            onResult(result);
          } catch (ex) {
            Cu.reportError(ex);
          }
        }
      }

      return results[0];
    });
  },

  


























































  
  
  fetchTree(guid = "", options = {}) {
    throw new Error("Not yet implemented");
  },

  














  
  
  reorder(parentGuid, orderedChildrenGuids) {
    throw new Error("Not yet implemented");
  }
});















function notify(observers, notification, args) {
  for (let observer of observers) {
    try {
      observer[notification](...args);
    } catch (ex) {}
  }
}

XPCOMUtils.defineLazyGetter(this, "DBConnPromised",
  () => new Promise((resolve, reject) => {
    Sqlite.wrapStorageConnection({ connection: PlacesUtils.history.DBConnection } )
          .then(db => {
      try {
        Sqlite.shutdown.addBlocker("Places Bookmarks.jsm wrapper closing",
                                   db.close.bind(db));
      }
      catch (ex) {
        
        db.close();
        reject(ex);
      }
      resolve(db);
    });
  })
);




function* updateBookmark(info, item, newParent) {
  let db = yield DBConnPromised;

  let tuples = new Map();
  if (info.hasOwnProperty("lastModified"))
    tuples.set("lastModified", { value: toPRTime(info.lastModified) });
  if (info.hasOwnProperty("title"))
    tuples.set("title", { value: info.title });

  yield db.executeTransaction(function* () {
    if (info.hasOwnProperty("keyword")) {
      if (info.keyword.length > 0)
        yield maybeCreateKeyword(db, info.keyword);
      tuples.set("keyword", { value: info.keyword
                            , fragment: "keyword_id = (SELECT id FROM moz_keywords WHERE keyword = :keyword)" });
    }

    if (info.hasOwnProperty("url")) {
      
      yield db.executeCached(
        `INSERT OR IGNORE INTO moz_places (url, rev_host, hidden, frecency, guid) 
         VALUES (:url, :rev_host, 0, :frecency, GENERATE_GUID())
        `, { url: info.url ? info.url.href : null,
             rev_host: PlacesUtils.getReversedHost(info.url),
             frecency: info.url.protocol == "place:" ? 0 : -1 });
      tuples.set("url", { value: info.url.href
                        , fragment: "fk = (SELECT id FROM moz_places WHERE url = :url)" });
    }

    if (newParent) {
      
      let newIndex = info.hasOwnProperty("index") ? info.index : item.index;
      tuples.set("position", { value: newIndex });

      if (newParent.guid == item.parentGuid) {
        
        
        
        let sign = newIndex < item.index ? +1 : -1;
        yield db.executeCached(
          `UPDATE moz_bookmarks SET position = position + :sign
           WHERE parent = :newParentId
             AND position BETWEEN :lowIndex AND :highIndex
          `, { sign: sign, newParentId: newParent._id,
               lowIndex: Math.min(item.index, newIndex),
               highIndex: Math.max(item.index, newIndex) });
      } else {
        
        tuples.set("parent", { value: newParent._id} );
        yield db.executeCached(
          `UPDATE moz_bookmarks SET position = position + :sign
           WHERE parent = :oldParentId
             AND position >= :oldIndex
          `, { sign: -1, oldParentId: item._parentId, oldIndex: item.index });
        yield db.executeCached(
          `UPDATE moz_bookmarks SET position = position + :sign
           WHERE parent = :newParentId
             AND position >= :newIndex
          `, { sign: +1, newParentId: newParent._id, newIndex: newIndex });

        yield setAncestorsLastModified(db, item.parentGuid, info.lastModified);
      }
      yield setAncestorsLastModified(db, newParent.guid, info.lastModified);
    }

    yield db.executeCached(
      `UPDATE moz_bookmarks
       SET ${[tuples.get(v).fragment || `${v} = :${v}` for (v of tuples.keys())].join(", ")}
       WHERE guid = :guid
      `, Object.assign({ guid: info.guid },
                       [...tuples.entries()].reduce((p, c) => { p[c[0]] = c[1].value; return p; }, {})));


    if (info.hasOwnProperty("keyword") && info.keyword === "")
      yield removeOrphanKeywords(db);
  });

  
  let additionalParentInfo = {};
  if (newParent) {
    Object.defineProperty(additionalParentInfo, "_parentId",
                          { value: newParent._id, enumerable: false });
    Object.defineProperty(additionalParentInfo, "_grandParentId",
                          { value: newParent._parentId, enumerable: false });
  }

  let updatedItem = mergeIntoNewObject(item, info, additionalParentInfo);

  
  if (updatedItem.hasOwnProperty("title") && updatedItem.title === null)
    delete updatedItem.title;
  if (updatedItem.hasOwnProperty("keyword") && updatedItem.keyword === "")
    delete updatedItem.keyword;

  return updatedItem;
}




function* insertBookmark(item, parent) {
  let db = yield DBConnPromised;

  
  
  if (!item.hasOwnProperty("guid"))
    item.guid = (yield db.executeCached("SELECT GENERATE_GUID() AS guid"))[0].getResultByName("guid");

  yield db.executeTransaction(function* transaction() {
    if (item.type == Bookmarks.TYPE_BOOKMARK) {
      
      yield db.executeCached(
        `INSERT OR IGNORE INTO moz_places (url, rev_host, hidden, frecency, guid) 
         VALUES (:url, :rev_host, 0, :frecency, GENERATE_GUID())
        `, { url: item.url.href, rev_host: PlacesUtils.getReversedHost(item.url),
             frecency: item.url.protocol == "place:" ? 0 : -1 });
    }

    
    yield db.executeCached(
      `UPDATE moz_bookmarks SET position = position + 1
       WHERE parent = :parent
       AND position >= :index
      `, { parent: parent._id, index: item.index });

    
    if (item.hasOwnProperty("keyword"))
      yield maybeCreateKeyword(db, item.keyword);

    
    yield db.executeCached(
      `INSERT INTO moz_bookmarks (fk, type, parent, position, title,
                                  dateAdded, lastModified, guid, keyword_id)
       VALUES ((SELECT id FROM moz_places WHERE url = :url), :type, :parent,
               :index, :title, :date_added, :last_modified, :guid,
               (SELECT id FROM moz_keywords WHERE keyword = :keyword))
      `, { url: item.hasOwnProperty("url") ? item.url.href : "nonexistent",
           type: item.type, parent: parent._id, index: item.index,
           title: item.title, date_added: toPRTime(item.dateAdded),
           last_modified: toPRTime(item.lastModified), guid: item.guid,
           keyword: item.keyword || "" });

    yield setAncestorsLastModified(db, item.parentGuid, item.dateAdded);
  });

  
  let isTagging = parent._parentId == PlacesUtils.tagsFolderId;
  if (item.type == Bookmarks.TYPE_BOOKMARK && !isTagging) {
    
    updateFrecency(db, [item.url]).then(null, Cu.reportError);
  }

  
  if (item.hasOwnProperty("title") && item.title === null)
    delete item.title;
  return item;
}




function* fetchBookmark(info) {
  let db = yield DBConnPromised;

  let rows = yield db.executeCached(
    `SELECT b.guid, IFNULL(p.guid, "") AS parentGuid, b.position AS 'index',
            b.dateAdded, b.lastModified, b.type, b.title, h.url AS url,
            keyword, b.id AS _id, b.parent AS _parentId,
            (SELECT count(*) FROM moz_bookmarks WHERE parent = b.id) AS _childCount,
            p.parent AS _grandParentId
     FROM moz_bookmarks b
     LEFT JOIN moz_bookmarks p ON p.id = b.parent
     LEFT JOIN moz_keywords k ON k.id = b.keyword_id
     LEFT JOIN moz_places h ON h.id = b.fk
     WHERE b.guid = :guid
    `, { guid: info.guid });

  return rows.length ? rowsToItemsArray(rows)[0] : null;
}

function* fetchBookmarkByPosition(info) {
  let db = yield DBConnPromised;

  let rows = yield db.executeCached(
    `SELECT b.guid, IFNULL(p.guid, "") AS parentGuid, b.position AS 'index',
            b.dateAdded, b.lastModified, b.type, b.title, h.url AS url,
            keyword, b.id AS _id, b.parent AS _parentId,
            (SELECT count(*) FROM moz_bookmarks WHERE parent = b.id) AS _childCount,
            p.parent AS _grandParentId
     FROM moz_bookmarks b
     LEFT JOIN moz_bookmarks p ON p.id = b.parent
     LEFT JOIN moz_keywords k ON k.id = b.keyword_id
     LEFT JOIN moz_places h ON h.id = b.fk
     WHERE p.guid = :parentGuid AND b.position = :index
    `, { parentGuid: info.parentGuid, index: info.index });

  return rows.length ? rowsToItemsArray(rows)[0] : null;
}

function* fetchBookmarksByURL(info) {
  let db = yield DBConnPromised;

  let rows = yield db.executeCached(
    `SELECT b.guid, IFNULL(p.guid, "") AS parentGuid, b.position AS 'index',
            b.dateAdded, b.lastModified, b.type, b.title, h.url AS url,
            keyword, b.id AS _id, b.parent AS _parentId,
            (SELECT count(*) FROM moz_bookmarks WHERE parent = b.id) AS _childCount,
            p.parent AS _grandParentId
     FROM moz_bookmarks b
     LEFT JOIN moz_bookmarks p ON p.id = b.parent
     LEFT JOIN moz_keywords k ON k.id = b.keyword_id
     LEFT JOIN moz_places h ON h.id = b.fk
     WHERE h.url = :url
     ORDER BY b.lastModified DESC
    `, { url: info.url.href });

  return rows.length ? rowsToItemsArray(rows) : null;
}

function* fetchBookmarksByKeyword(info) {
  let db = yield DBConnPromised;

  let rows = yield db.executeCached(
    `SELECT b.guid, IFNULL(p.guid, "") AS parentGuid, b.position AS 'index',
            b.dateAdded, b.lastModified, b.type, b.title, h.url AS url,
            keyword, b.id AS _id, b.parent AS _parentId,
            (SELECT count(*) FROM moz_bookmarks WHERE parent = b.id) AS _childCount,
            p.parent AS _grandParentId
     FROM moz_bookmarks b
     LEFT JOIN moz_bookmarks p ON p.id = b.parent
     LEFT JOIN moz_keywords k ON k.id = b.keyword_id
     LEFT JOIN moz_places h ON h.id = b.fk
     WHERE keyword = :keyword
     ORDER BY b.lastModified DESC
    `, { keyword: info.keyword });

  return rows.length ? rowsToItemsArray(rows) : null;
}




function* removeBookmark(item) {
  let db = yield DBConnPromised;

  let isUntagging = item._grandParentId == PlacesUtils.tagsFolderId;

  yield db.executeTransaction(function* transaction() {
    
    if (!isUntagging) {
      
      
      
      yield removeAnnotationsForItem(db, item._id);
    }

    
    yield db.executeCached(
      `DELETE FROM moz_bookmarks WHERE guid = :guid`, { guid: item.guid });

    
    yield db.executeCached(
      `UPDATE moz_bookmarks SET position = position - 1 WHERE
       parent = :parentId AND position > :index
      `, { parentId: item._parentId, index: item.index });

    yield setAncestorsLastModified(db, item.parentGuid, new Date());

    
    if (item.keyword)
      removeOrphanKeywords(db);
  });

  
  if (item.type == Bookmarks.TYPE_BOOKMARK && !isUntagging) {
    
    updateFrecency(db, [item.url]).then(null, Cu.reportError);
  }

  return item;
}











function mergeIntoNewObject(...sources) {
  let dest = {};
  for (let src of sources) {
    for (let prop of Object.getOwnPropertyNames(src)) {
      Object.defineProperty(dest, prop, Object.getOwnPropertyDescriptor(src, prop));
    }
  }
  return dest;
}











function removeSameValueProperties(dest, src) {
  for (let prop in dest) {
    let remove = false;
    switch (prop) {
      case "lastModified":
      case "dateAdded":
        remove = src.hasOwnProperty(prop) && dest[prop].getTime() == src[prop].getTime();
        break;
      case "url":
        remove = src.hasOwnProperty(prop) && dest[prop].href == src[prop].href;
        break;
      case "keyword":
        remove = (dest.keyword == "" && !src.hasOwnProperty("keyword")) ||
                 dest[prop] == src[prop];
        break;
      default:
        remove = dest[prop] == src[prop];
    }
    if (remove && prop != "guid")
      delete dest[prop];
  }
}








function toURI(url) NetUtil.newURI(url.href);








function toPRTime(date) date * 1000;








function toDate(time) new Date(parseInt(time / 1000));








function rowsToItemsArray(rows) {
  return rows.map(row => {
    let item = {};
    for (let prop of ["guid", "index", "type"]) {
      item[prop] = row.getResultByName(prop);
    }
    for (let prop of ["dateAdded", "lastModified"]) {
      item[prop] = toDate(row.getResultByName(prop));
    }
    for (let prop of ["title", "keyword", "parentGuid", "url" ]) {
      let val = row.getResultByName(prop);
      if (val)
        item[prop] = prop === "url" ? new URL(val) : val;
    }
    for (let prop of ["_id", "_parentId", "_childCount", "_grandParentId"]) {
      let val = row.getResultByName(prop);
      if (val !== null) {
        
        
        
        
        Object.defineProperty(item, prop, { value: val, enumerable: false,
                                                        configurable: true });
      }
    }

    return item;
  });  
}









function simpleValidateFunc(boolValidateFn) {
  return (v, input) => {
    if (!boolValidateFn(v, input))
      throw new Error("Invalid value");
    return v;
  };
}






const VALIDATORS = Object.freeze({
  guid: simpleValidateFunc(v => typeof(v) == "string" &&
                                /^[a-zA-Z0-9\-_]{12}$/.test(v)),
  parentGuid: simpleValidateFunc(v => typeof(v) == "string" &&
                                      /^[a-zA-Z0-9\-_]{12}$/.test(v)),
  index: simpleValidateFunc(v => Number.isInteger(v) &&
                                 v >= Bookmarks.DEFAULT_INDEX),
  dateAdded: simpleValidateFunc(v => v.constructor.name == "Date"),
  lastModified: simpleValidateFunc(v => v.constructor.name == "Date"),
  type: simpleValidateFunc(v => Number.isInteger(v) &&
                                [ Bookmarks.TYPE_BOOKMARK
                                , Bookmarks.TYPE_FOLDER
                                , Bookmarks.TYPE_SEPARATOR ].indexOf(v) != -1),
  title: v => {
    simpleValidateFunc(val => val === null || typeof(val) == "string").call(this, v);
    if (!v)
      return null;
    return v.slice(0, DB_TITLE_LENGTH_MAX);
  },
  url: v => {
    simpleValidateFunc(val => (typeof(val) == "string" && val.length <= DB_URL_LENGTH_MAX) ||
                              (val instanceof Ci.nsIURI && val.spec.length <= DB_URL_LENGTH_MAX) ||
                              (val instanceof URL && val.href.length <= DB_URL_LENGTH_MAX)
                      ).call(this, v);
    if (typeof(v) === "string")
      return new URL(v);
    if (v instanceof Ci.nsIURI)
      return new URL(v.spec);
    return v;
  },
  keyword: simpleValidateFunc(v => typeof(v) == "string" && /^\S*$/.test(v)),
});




















function validateBookmarkObject(input, behavior={}) {
  if (!input)
    throw new Error("Input should be a valid object");
  let normalizedInput = {};
  let required = new Set();
  for (let prop in behavior) {
    if (behavior[prop].hasOwnProperty("required") && behavior[prop].required) {
      required.add(prop);
    }
    if (behavior[prop].hasOwnProperty("requiredIf") && behavior[prop].requiredIf(input)) {
      required.add(prop);
    }
    if (behavior[prop].hasOwnProperty("validIf") && input[prop] !== undefined &&
        !behavior[prop].validIf(input)) {
      throw new Error(`Invalid value for property '${prop}': ${input[prop]}`);
    }
    if (behavior[prop].hasOwnProperty("defaultValue") && input[prop] === undefined) {
      normalizedInput[prop] = behavior[prop].defaultValue;
    }
  }

  for (let prop in input) {
    if (required.has(prop))
      required.delete(prop);
    if (VALIDATORS.hasOwnProperty(prop)) {
      try {
        normalizedInput[prop] = VALIDATORS[prop](input[prop], input);
      } catch(ex) {
        throw new Error(`Invalid value for property '${prop}': ${input[prop]}`);
      }
    }
  }
  if (required.size > 0)
    throw new Error(`The following properties were expected: ${[...required].join(", ")}`); 
  return normalizedInput;
}









let updateFrecency = Task.async(function* (db, urls) {
  yield db.execute(
    `UPDATE moz_places
     SET frecency = NOTIFY_FRECENCY(
       CALCULATE_FRECENCY(id), url, guid, hidden, last_visit_date
     ) WHERE url IN ( ${urls.map(url => JSON.stringify(url.href)).join(", ")} )
    `);

  yield db.execute(
    `UPDATE moz_places
     SET hidden = 0
     WHERE url IN ( ${urls.map(url => JSON.stringify(url.href)).join(", ")} )
       AND frecency <> 0
    `);
});









let maybeCreateKeyword = Task.async(function* (db, keyword) {
  yield db.executeCached(
    `INSERT OR IGNORE INTO moz_keywords (keyword)
     VALUES (:keyword)
    `, { keyword: keyword });
});







let removeOrphanKeywords = Task.async(function* (db) {
  yield db.executeCached(
    `DELETE FROM moz_keywords
     WHERE NOT EXISTS(SELECT 1 FROM moz_bookmarks
                      WHERE keyword_id = moz_keywords.id)`);
});







let removeOrphanAnnotations = Task.async(function* (db) {
  yield db.executeCached(
    `DELETE FROM moz_items_annos
     WHERE id IN (SELECT a.id from moz_items_annos a 
                  LEFT JOIN moz_bookmarks b ON a.item_id = b.id 
                  WHERE b.id ISNULL)
    `);
  yield db.executeCached(
    `DELETE FROM moz_anno_attributes
     WHERE id IN (SELECT n.id from moz_anno_attributes n
                  LEFT JOIN moz_annos a1 ON a1.anno_attribute_id = n.id
                  LEFT JOIN moz_items_annos a2 ON a2.anno_attribute_id = n.id
                  WHERE a1.id ISNULL AND a2.id ISNULL)
    `);
});









let removeAnnotationsForItem = Task.async(function* (db, itemId) {
  yield db.executeCached(
    `DELETE FROM moz_items_annos
     WHERE item_id = :id
    `, { id: itemId });
  yield db.executeCached(
    `DELETE FROM moz_anno_attributes
     WHERE id IN (SELECT n.id from moz_anno_attributes n
                  LEFT JOIN moz_items_annos a ON a.anno_attribute_id = n.id
                  WHERE a.id ISNULL)
    `);
});













let setAncestorsLastModified = Task.async(function* (db, folderGuid, time) {
  yield db.executeCached(
    `WITH RECURSIVE
     ancestors(aid) AS (
       SELECT id FROM moz_bookmarks WHERE guid = :guid
       UNION ALL
       SELECT parent FROM moz_bookmarks
       JOIN ancestors ON id = aid
       WHERE type = :type
     )
     UPDATE moz_bookmarks SET lastModified = :time
     WHERE id IN ancestors
    `, { guid: folderGuid, type: Bookmarks.TYPE_FOLDER,
         time: toPRTime(time) });
});
