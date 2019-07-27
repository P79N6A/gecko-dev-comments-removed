



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

  




   rootGuid:    "root________",
   menuGuid:    "menu________",
   toolbarGuid: "toolbar_____",
   unfiledGuid: "unfiled_____",

   
   
   tagsGuid:    "tags________",

  























  insert(info) {
    
    
    let time = (info && info.dateAdded) || new Date();
    let insertInfo = validateBookmarkObject(info,
      { type: { defaultValue: this.TYPE_BOOKMARK }
      , index: { defaultValue: this.DEFAULT_INDEX }
      , url: { requiredIf: b => b.type == this.TYPE_BOOKMARK
             , validIf: b => b.type == this.TYPE_BOOKMARK }
      , parentGuid: { required: true }
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
               , validIf: b => b.index >= 0 || b.index == this.DEFAULT_INDEX }
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
        , title: { validIf: () => [ this.TYPE_BOOKMARK
                                  , this.TYPE_FOLDER ].indexOf(item.type) != -1 }
        , lastModified: { defaultValue: new Date()
                        , validIf: b => b.lastModified >= item.dateAdded }
        });

      let db = yield PlacesUtils.promiseWrappedConnection();
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

        if (updateInfo.index >= parent._childCount ||
            updateInfo.index == this.DEFAULT_INDEX) {
           updateInfo.index = parent._childCount;

          
          if (parent.guid == item.parentGuid)
             updateInfo.index--;
        }
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
      
      if (item.parentGuid != updatedItem.parentGuid ||
          item.index != updatedItem.index) {
        notify(observers, "onItemMoved", [ updatedItem._id, item._parentId,
                                           item.index, updatedItem._parentId,
                                           updatedItem.index, updatedItem.type,
                                           updatedItem.guid, item.parentGuid,
                                           updatedItem.parentGuid ]);
      }

      
      return Object.assign({}, updatedItem);
    }.bind(this));
  },

  











  remove(guidOrInfo) {
    let info = guidOrInfo;
    if (!info)
      throw new Error("Input should be a valid object");
    if (typeof(guidOrInfo) != "object")
      info = { guid: guidOrInfo };

    
    if ([this.rootGuid, this.menuGuid, this.toolbarGuid, this.unfiledGuid,
         this.tagsGuid].indexOf(info.guid) != -1) {
      throw new Error("It's not possible to remove Places root folders.");
    }

    
    
    let removeInfo = validateBookmarkObject(info);

    return Task.spawn(function* () {
      let item = yield fetchBookmark(removeInfo);
      if (!item)
        throw new Error("No bookmarks found for the provided GUID.");

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
    let db = yield PlacesUtils.promiseWrappedConnection();
    yield db.executeTransaction(function* () {
      const folderGuids = [this.toolbarGuid, this.menuGuid, this.unfiledGuid];
      yield removeFoldersContents(db, folderGuids);
      const time = toPRTime(new Date());
      for (let folderGuid of folderGuids) {
        yield db.executeCached(
          `UPDATE moz_bookmarks SET lastModified = :time
           WHERE id IN (SELECT id FROM moz_bookmarks WHERE guid = :folderGuid )
          `, { folderGuid, time });
      }
    }.bind(this));
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
      v => v.hasOwnProperty("url")
    ].reduce((old, fn) => old + fn(info)|0, 0);
    if (conditionsCount != 1)
      throw new Error(`Unexpected number of conditions provided: ${conditionsCount}`);

    
    
    let fetchInfo = validateBookmarkObject(info,
      { parentGuid: { requiredIf: b => b.hasOwnProperty("index") }
      , index: { requiredIf: b => b.hasOwnProperty("parentGuid")
               , validIf: b => typeof(b.index) == "number" &&
                               b.index >= 0 || b.index == this.DEFAULT_INDEX }
      });

    return Task.spawn(function* () {
      let results;
      if (fetchInfo.hasOwnProperty("guid"))
        results = yield fetchBookmark(fetchInfo);
      else if (fetchInfo.hasOwnProperty("parentGuid") && fetchInfo.hasOwnProperty("index"))
        results = yield fetchBookmarkByPosition(fetchInfo);
      else if (fetchInfo.hasOwnProperty("url"))
        results = yield fetchBookmarksByURL(fetchInfo);

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
    let info = { guid: parentGuid };
    info = validateBookmarkObject(info, { guid: { required: true } });

    if (!Array.isArray(orderedChildrenGuids) || !orderedChildrenGuids.length)
      throw new Error("Must provide a sorted array of children GUIDs.");
    try {
      orderedChildrenGuids.forEach(VALIDATORS.guid);
    } catch (ex) {
      throw new Error("Invalid GUID found in the sorted children array.");
    }

    return Task.spawn(function* () {
      let parent = yield fetchBookmark(info);
      if (!parent || parent.type != this.TYPE_FOLDER)
        throw new Error("No folder found for the provided GUID.");

      let sortedChildren = yield reorderChildren(parent, orderedChildrenGuids);

      let observers = PlacesUtils.bookmarks.getObservers();
      
      for (let i = 0; i < sortedChildren.length; ++i) {
        let child = sortedChildren[i];
        notify(observers, "onItemMoved", [ child._id, child._parentId,
                                           child.index, child._parentId,
                                           i, child.type,
                                           child.guid, child.parentGuid,
                                           child.parentGuid ]);
      }
    }.bind(this));
  }
});














function notify(observers, notification, args) {
  for (let observer of observers) {
    try {
      observer[notification](...args);
    } catch (ex) {}
  }
}




function* updateBookmark(info, item, newParent) {
  let db = yield PlacesUtils.promiseWrappedConnection();

  let tuples = new Map();
  if (info.hasOwnProperty("lastModified"))
    tuples.set("lastModified", { value: toPRTime(info.lastModified) });
  if (info.hasOwnProperty("title"))
    tuples.set("title", { value: info.title });

  yield db.executeTransaction(function* () {
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

  return updatedItem;
}




function* insertBookmark(item, parent) {
  let db = yield PlacesUtils.promiseWrappedConnection();

  
  
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

    
    yield db.executeCached(
      `INSERT INTO moz_bookmarks (fk, type, parent, position, title,
                                  dateAdded, lastModified, guid)
       VALUES ((SELECT id FROM moz_places WHERE url = :url), :type, :parent,
               :index, :title, :date_added, :last_modified, :guid)
      `, { url: item.hasOwnProperty("url") ? item.url.href : "nonexistent",
           type: item.type, parent: parent._id, index: item.index,
           title: item.title, date_added: toPRTime(item.dateAdded),
           last_modified: toPRTime(item.lastModified), guid: item.guid });

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
  let db = yield PlacesUtils.promiseWrappedConnection();

  let rows = yield db.executeCached(
    `SELECT b.guid, IFNULL(p.guid, "") AS parentGuid, b.position AS 'index',
            b.dateAdded, b.lastModified, b.type, b.title, h.url AS url,
            b.id AS _id, b.parent AS _parentId,
            (SELECT count(*) FROM moz_bookmarks WHERE parent = b.id) AS _childCount,
            p.parent AS _grandParentId
     FROM moz_bookmarks b
     LEFT JOIN moz_bookmarks p ON p.id = b.parent
     LEFT JOIN moz_places h ON h.id = b.fk
     WHERE b.guid = :guid
    `, { guid: info.guid });

  return rows.length ? rowsToItemsArray(rows)[0] : null;
}

function* fetchBookmarkByPosition(info) {
  let db = yield PlacesUtils.promiseWrappedConnection();
  let index = info.index == Bookmarks.DEFAULT_INDEX ? null : info.index;

  let rows = yield db.executeCached(
    `SELECT b.guid, IFNULL(p.guid, "") AS parentGuid, b.position AS 'index',
            b.dateAdded, b.lastModified, b.type, b.title, h.url AS url,
            b.id AS _id, b.parent AS _parentId,
            (SELECT count(*) FROM moz_bookmarks WHERE parent = b.id) AS _childCount,
            p.parent AS _grandParentId
     FROM moz_bookmarks b
     LEFT JOIN moz_bookmarks p ON p.id = b.parent
     LEFT JOIN moz_places h ON h.id = b.fk
     WHERE p.guid = :parentGuid
     AND b.position = IFNULL(:index, (SELECT count(*) - 1
                                      FROM moz_bookmarks
                                      WHERE parent = p.id))
    `, { parentGuid: info.parentGuid, index });

  return rows.length ? rowsToItemsArray(rows)[0] : null;
}

function* fetchBookmarksByURL(info) {
  let db = yield PlacesUtils.promiseWrappedConnection();

  let rows = yield db.executeCached(
    `SELECT b.guid, IFNULL(p.guid, "") AS parentGuid, b.position AS 'index',
            b.dateAdded, b.lastModified, b.type, b.title, h.url AS url,
            b.id AS _id, b.parent AS _parentId,
            (SELECT count(*) FROM moz_bookmarks WHERE parent = b.id) AS _childCount,
            p.parent AS _grandParentId
     FROM moz_bookmarks b
     LEFT JOIN moz_bookmarks p ON p.id = b.parent
     LEFT JOIN moz_places h ON h.id = b.fk
     WHERE h.url = :url
     AND _grandParentId <> :tags_folder
     ORDER BY b.lastModified DESC
    `, { url: info.url.href,
         tags_folder: PlacesUtils.tagsFolderId });

  return rows.length ? rowsToItemsArray(rows) : null;
}

function* fetchBookmarksByParent(info) {
  let db = yield PlacesUtils.promiseWrappedConnection();

  let rows = yield db.executeCached(
    `SELECT b.guid, IFNULL(p.guid, "") AS parentGuid, b.position AS 'index',
            b.dateAdded, b.lastModified, b.type, b.title, h.url AS url,
            b.id AS _id, b.parent AS _parentId,
            (SELECT count(*) FROM moz_bookmarks WHERE parent = b.id) AS _childCount,
            p.parent AS _grandParentId
     FROM moz_bookmarks b
     LEFT JOIN moz_bookmarks p ON p.id = b.parent
     LEFT JOIN moz_places h ON h.id = b.fk
     WHERE p.guid = :parentGuid
     ORDER BY b.position ASC
    `, { parentGuid: info.parentGuid });

  return rowsToItemsArray(rows);
}




function* removeBookmark(item) {
  let db = yield PlacesUtils.promiseWrappedConnection();

  let isUntagging = item._grandParentId == PlacesUtils.tagsFolderId;

  yield db.executeTransaction(function* transaction() {
    
    if (item.type == Bookmarks.TYPE_FOLDER)
      yield removeFoldersContents(db, [item.guid]);

    
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
  });

  
  if (item.type == Bookmarks.TYPE_BOOKMARK && !isUntagging) {
    
    updateFrecency(db, [item.url]).then(null, Cu.reportError);
  }

  return item;
}




function* reorderChildren(parent, orderedChildrenGuids) {
  let db = yield PlacesUtils.promiseWrappedConnection();

  return db.executeTransaction(function* () {
    
    let children = yield fetchBookmarksByParent({ parentGuid: parent.guid });
    if (!children.length)
      return;

    
    
    children.sort((a, b) => {
      let i = orderedChildrenGuids.indexOf(a.guid);
      let j = orderedChildrenGuids.indexOf(b.guid);
      
      return (i == -1 && j == -1) ? 0 :
               (i != -1 && j != -1 && i < j) || (i != -1 && j == -1) ? -1 : 1;
     });

    
    
    
    
    
    
    let valuesTable = children.map((child, i) => `("${child.guid}", ${i})`)
                              .join();
    yield db.execute(
      `WITH sorting(g, p) AS (
         VALUES ${valuesTable}
       )
       UPDATE moz_bookmarks SET position = (
         SELECT CASE count(a.g) WHEN 0 THEN -position
                                ELSE count(a.g) - 1
                END
         FROM sorting a
         JOIN sorting b ON b.p <= a.p
         WHERE a.g = guid
           AND parent = :parentId
      )`, { parentId: parent._id});

    
    
    
    yield db.executeCached(
      `CREATE TEMP TRIGGER moz_bookmarks_reorder_trigger
         AFTER UPDATE OF position ON moz_bookmarks
         WHEN NEW.position = -1
       BEGIN
         UPDATE moz_bookmarks
         SET position = (SELECT MAX(position) FROM moz_bookmarks
                         WHERE parent = NEW.parent) +
                        (SELECT count(*) FROM moz_bookmarks
                         WHERE parent = NEW.parent
                           AND position BETWEEN OLD.position AND -1)
         WHERE guid = NEW.guid;
       END
      `);

    yield db.executeCached(
      `UPDATE moz_bookmarks SET position = -1 WHERE position < 0`);

    yield db.executeCached(`DROP TRIGGER moz_bookmarks_reorder_trigger`);

    return children;
  }.bind(this));
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
      default:
        remove = dest[prop] == src[prop];
    }
    if (remove && prop != "guid")
      delete dest[prop];
  }
}








function toURI(url) {
  return NetUtil.newURI(url.href);
}








function toPRTime(date) {
  return date * 1000;
}








function toDate(time) {
  return new Date(parseInt(time / 1000));
}








function rowsToItemsArray(rows) {
  return rows.map(row => {
    let item = {};
    for (let prop of ["guid", "index", "type"]) {
      item[prop] = row.getResultByName(prop);
    }
    for (let prop of ["dateAdded", "lastModified"]) {
      item[prop] = toDate(row.getResultByName(prop));
    }
    for (let prop of ["title", "parentGuid", "url" ]) {
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
  }
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
      input[prop] = behavior[prop].defaultValue;
    }
  }

  for (let prop in input) {
    if (required.has(prop)) {
      required.delete(prop);
    } else if (input[prop] === undefined) {
      
      continue;
    }
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
                  LEFT JOIN moz_annos a1 ON a1.anno_attribute_id = n.id
                  LEFT JOIN moz_items_annos a2 ON a2.anno_attribute_id = n.id
                  WHERE a1.id ISNULL AND a2.id ISNULL)
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









let removeFoldersContents =
Task.async(function* (db, folderGuids) {
  let itemsRemoved = [];
  for (let folderGuid of folderGuids) {
    let rows = yield db.executeCached(
      `WITH RECURSIVE
       descendants(did) AS (
         SELECT b.id FROM moz_bookmarks b
         JOIN moz_bookmarks p ON b.parent = p.id
         WHERE p.guid = :folderGuid
         UNION ALL
         SELECT id FROM moz_bookmarks
         JOIN descendants ON parent = did
       )
       SELECT b.id AS _id, b.parent AS _parentId, b.position AS 'index',
              b.type, url, b.guid, p.guid AS parentGuid, b.dateAdded,
              b.lastModified, b.title, p.parent AS _grandParentId,
              NULL AS _childCount
       FROM moz_bookmarks b
       JOIN moz_bookmarks p ON p.id = b.parent
       LEFT JOIN moz_places h ON b.fk = h.id
       WHERE b.id IN descendants`, { folderGuid });

    itemsRemoved = itemsRemoved.concat(rowsToItemsArray(rows));

    yield db.executeCached(
      `WITH RECURSIVE
       descendants(did) AS (
         SELECT b.id FROM moz_bookmarks b
         JOIN moz_bookmarks p ON b.parent = p.id
         WHERE p.guid = :folderGuid
         UNION ALL
         SELECT id FROM moz_bookmarks
         JOIN descendants ON parent = did
       )
       DELETE FROM moz_bookmarks WHERE id IN descendants`, { folderGuid });
  }

  
  yield removeOrphanAnnotations(db);

  

  let urls = [for (item of itemsRemoved) if (item.url) item.url];
  updateFrecency(db, urls).then(null, Cu.reportError);

  
  
  
  

  
  let observers = PlacesUtils.bookmarks.getObservers();
  for (let item of itemsRemoved.reverse()) {
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
