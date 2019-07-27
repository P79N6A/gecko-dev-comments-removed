



"use strict";

this.EXPORTED_SYMBOLS = [
  "Sync",
];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Preferences",
  "resource://gre/modules/Preferences.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReadingList",
  "resource:///modules/readinglist/ReadingList.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ServerClient",
  "resource:///modules/readinglist/ServerClient.jsm");



const BATCH_REQUEST_LIMIT = 25;


const SERVER_LAST_MODIFIED_HEADER_PREF = "readinglist.sync.serverLastModified";


const SERVER_PROPERTIES_BY_LOCAL_PROPERTIES = {
  guid: "id",
  serverLastModified: "last_modified",
  url: "url",
  preview: "preview",
  title: "title",
  resolvedURL: "resolved_url",
  resolvedTitle: "resolved_title",
  excerpt: "excerpt",
  archived: "archived",
  deleted: "deleted",
  favorite: "favorite",
  isArticle: "is_article",
  wordCount: "word_count",
  unread: "unread",
  addedBy: "added_by",
  addedOn: "added_on",
  storedOn: "stored_on",
  markedReadBy: "marked_read_by",
  markedReadOn: "marked_read_on",
  readPosition: "read_position",
};


const NEW_RECORD_PROPERTIES = `
  url
  title
  resolvedURL
  resolvedTitle
  excerpt
  favorite
  isArticle
  wordCount
  unread
  addedBy
  addedOn
  markedReadBy
  markedReadOn
  readPosition
  preview
`.trim().split(/\s+/);


const MUTABLE_RECORD_PROPERTIES = `
  title
  resolvedURL
  resolvedTitle
  excerpt
  favorite
  isArticle
  wordCount
  unread
  markedReadBy
  markedReadOn
  readPosition
  preview
`.trim().split(/\s+/);

let log = Log.repository.getLogger("readinglist.sync");








function SyncImpl(readingList) {
  this.list = readingList;
  this._client = new ServerClient();
}







SyncImpl.prototype = {

  





  start() {
    if (!this.promise) {
      this.promise = Task.spawn(function* () {
        try {
          yield this._start();
        } finally {
          delete this.promise;
        }
      }.bind(this));
    }
    return this.promise;
  },

  



  promise: null,

  


  _start: Task.async(function* () {
    log.info("Starting sync");
    yield this._uploadStatusChanges();
    yield this._uploadNewItems();
    yield this._uploadDeletedItems();
    yield this._downloadModifiedItems();

    
    yield this._uploadMaterialChanges();

    log.info("Sync done");
  }),

  





  _uploadStatusChanges: Task.async(function* () {
    log.debug("Phase 1 part 1: Uploading status changes");
    yield this._uploadChanges(ReadingList.SyncStatus.CHANGED_STATUS,
                              ReadingList.SyncStatusProperties.STATUS);
  }),

  








  _uploadChanges: Task.async(function* (syncStatus, localProperties) {
    
    let requests = [];
    yield this.list.forEachItem(localItem => {
      requests.push({
        path: "/articles/" + localItem.guid,
        body: serverRecordFromLocalItem(localItem, localProperties),
      });
    }, { syncStatus: syncStatus });
    if (!requests.length) {
      log.debug("No local changes to upload");
      return;
    }

    
    let request = {
      body: {
        defaults: {
          method: "PATCH",
        },
        requests: requests,
      },
    };
    let batchResponse = yield this._postBatch(request);
    if (batchResponse.status != 200) {
      this._handleUnexpectedResponse("uploading changes", batchResponse);
      return;
    }

    
    for (let response of batchResponse.body.responses) {
      if (response.status == 404) {
        
        yield this._deleteItemForGUID(response.body.id);
        continue;
      }
      if (response.status == 409) {
        
        
        
        
        continue;
      }
      if (response.status != 200) {
        this._handleUnexpectedResponse("uploading a change", response);
        continue;
      }
      
      
      

      let item = yield this._itemForGUID(response.body.id);
      yield this._updateItemWithServerRecord(item, response.body);
    }
  }),

  




  _uploadNewItems: Task.async(function* () {
    log.debug("Phase 1 part 2: Uploading new items");

    
    let requests = [];
    yield this.list.forEachItem(localItem => {
      requests.push({
        body: serverRecordFromLocalItem(localItem, NEW_RECORD_PROPERTIES),
      });
    }, { syncStatus: ReadingList.SyncStatus.NEW });
    if (!requests.length) {
      log.debug("No new local items to upload");
      return;
    }

    
    let request = {
      body: {
        defaults: {
          method: "POST",
          path: "/articles",
        },
        requests: requests,
      },
    };
    let batchResponse = yield this._postBatch(request);
    if (batchResponse.status != 200) {
      this._handleUnexpectedResponse("uploading new items", batchResponse);
      return;
    }

    
    for (let response of batchResponse.body.responses) {
      if (response.status == 303) {
        
        
        
        
        continue;
      }
      
      
      
      
      
      if (response.status == 200) {
        log.debug("Attempting to upload a new item found the server already had it", response);
        
      } else if (response.status != 201) {
        this._handleUnexpectedResponse("uploading a new item", response);
        continue;
      }
      let item = yield this.list.itemForURL(response.body.url);
      yield this._updateItemWithServerRecord(item, response.body);
    }
  }),

  




  _uploadDeletedItems: Task.async(function* () {
    log.debug("Phase 1 part 3: Uploading deleted items");

    
    let requests = [];
    yield this.list.forEachSyncedDeletedGUID(guid => {
      requests.push({
        path: "/articles/" + guid,
      });
    });
    if (!requests.length) {
      log.debug("No local deleted synced items to upload");
      return;
    }

    
    let request = {
      body: {
        defaults: {
          method: "DELETE",
        },
        requests: requests,
      },
    };
    let batchResponse = yield this._postBatch(request);
    if (batchResponse.status != 200) {
      this._handleUnexpectedResponse("uploading deleted items", batchResponse);
      return;
    }

    
    for (let response of batchResponse.body.responses) {
      
      
      if (response.status != 200 && response.status != 404) {
        this._handleUnexpectedResponse("uploading a deleted item", response);
        continue;
      }
      yield this._deleteItemForGUID(response.body.id);
    }
  }),

  




  _downloadModifiedItems: Task.async(function* () {
    log.debug("Phase 2: Downloading modified items");

    
    let path = "/articles";
    if (this._serverLastModifiedHeader) {
      path += "?_since=" + this._serverLastModifiedHeader;
    }
    let request = {
      method: "GET",
      path: path,
    };
    let response = yield this._sendRequest(request);
    if (response.status != 200) {
      this._handleUnexpectedResponse("downloading modified items", response);
      return;
    }

    
    for (let serverRecord of response.body.items) {
      if (serverRecord.deleted) {
        
        yield this._deleteItemForGUID(serverRecord.id);
        continue;
      }
      let localItem = yield this._itemForGUID(serverRecord.id);
      if (localItem) {
        if (localItem.serverLastModified == serverRecord.last_modified) {
          
          continue;
        }
        
        
        
        
        

        yield this._updateItemWithServerRecord(localItem, serverRecord);
        continue;
      }
      
      
      
      let localRecord = localRecordFromServerRecord(serverRecord);
      try {
        yield this.list.addItem(localRecord);
      } catch (ex) {
        if (ex instanceof ReadingList.Error.Exists) {
          log.debug("Tried to add an item that already exists.");
        } else {
          log.error("Error adding an item from server record ${serverRecord} ${ex}",
                    { serverRecord, ex });
        }
      }
    }

    
    
    
    if (response.headers && "last-modified" in response.headers) {
      this._serverLastModifiedHeader = response.headers["last-modified"];
    }
  }),

  




  _uploadMaterialChanges: Task.async(function* () {
    log.debug("Phase 3: Uploading material changes");
    yield this._uploadChanges(ReadingList.SyncStatus.CHANGED_MATERIAL,
                              MUTABLE_RECORD_PROPERTIES);
  }),

  





  _itemForGUID: Task.async(function* (guid) {
    return (yield this.list.item({ guid: guid }));
  }),

  







  _updateItemWithServerRecord: Task.async(function* (localItem, serverRecord) {
    if (!localItem) {
      
      
      log.debug("Tried to update a null local item from server record",
                serverRecord);
      return;
    }
    localItem._record = localRecordFromServerRecord(serverRecord);
    try {
      yield this.list.updateItem(localItem);
    } catch (ex) {
      
      if (ex instanceof ReadingList.Error.Deleted) {
        log.debug("Tried to update an item that was deleted from server record",
                  serverRecord);
      } else {
        log.error("Error updating an item from server record ${serverRecord} ${ex}",
                  { serverRecord, ex });
      }
    }
  }),

  




  _deleteItemForGUID: Task.async(function* (guid) {
    let item = yield this._itemForGUID(guid);
    if (item) {
      
      
      
      
      item._record.syncStatus = ReadingList.SyncStatus.NEW;
      try {
        yield this.list.deleteItem(item);
      } catch (ex) {
        log.error("Failed delete local item with id ${guid} ${ex}",
                  { guid, ex });
      }
      return;
    }
    
    
    
    
    log.debug("Item not present in list, deleting it by GUID instead");
    try {
      this.list._store.deleteItemByGUID(guid);
    } catch (ex) {
      log.error("Failed to delete local item with id ${guid} ${ex}",
                { guid, ex });
    }
  }),

  






  _sendRequest: Task.async(function* (req) {
    log.debug("Sending request", req);
    let response = yield this._client.request(req);
    log.debug("Received response", response);
    return response;
  }),

  

















  _postBatch: Task.async(function* (bigRequest) {
    log.debug("Sending batch requests");
    let allSubResponses = [];
    let remainingSubRequests = bigRequest.body.requests;
    while (remainingSubRequests.length) {
      let request = Object.assign({}, bigRequest);
      request.method = "POST";
      request.path = "/batch";
      request.body.requests =
        remainingSubRequests.splice(0, BATCH_REQUEST_LIMIT);
      let response = yield this._sendRequest(request);
      if (response.status != 200) {
        return response;
      }
      allSubResponses = allSubResponses.concat(response.body.responses);
    }
    let bigResponse = {
      status: 200,
      body: {
        responses: allSubResponses,
      },
    };
    log.debug("All batch requests successfully sent");
    return bigResponse;
  }),

  _handleUnexpectedResponse(contextMsgFragment, response) {
    log.error(`Unexpected response ${contextMsgFragment}`, response);
  },

  
  get _serverLastModifiedHeader() {
    if (!("__serverLastModifiedHeader" in this)) {
      this.__serverLastModifiedHeader =
        Preferences.get(SERVER_LAST_MODIFIED_HEADER_PREF, undefined);
    }
    return this.__serverLastModifiedHeader;
  },
  set _serverLastModifiedHeader(val) {
    this.__serverLastModifiedHeader = val;
    Preferences.set(SERVER_LAST_MODIFIED_HEADER_PREF, val);
  },
};










function serverRecordFromLocalItem(localItem, localProperties) {
  let serverRecord = {};
  for (let localProp of localProperties) {
    let serverProp = SERVER_PROPERTIES_BY_LOCAL_PROPERTIES[localProp];
    if (localProp in localItem._record) {
      serverRecord[serverProp] = localItem._record[localProp];
    }
  }
  return serverRecord;
}








function localRecordFromServerRecord(serverRecord) {
  let localRecord = {
    
    syncStatus: ReadingList.SyncStatus.SYNCED,
  };
  for (let localProp in SERVER_PROPERTIES_BY_LOCAL_PROPERTIES) {
    let serverProp = SERVER_PROPERTIES_BY_LOCAL_PROPERTIES[localProp];
    if (serverProp in serverRecord) {
      localRecord[localProp] = serverRecord[serverProp];
    }
  }
  return localRecord;
}

Object.defineProperty(this, "Sync", {
  get() {
    if (!this._singleton) {
      this._singleton = new SyncImpl(ReadingList);
    }
    return this._singleton;
  },
});
