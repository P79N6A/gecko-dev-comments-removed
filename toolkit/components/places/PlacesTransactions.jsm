



"use strict";

this.EXPORTED_SYMBOLS = ["PlacesTransactions"];









































































































































































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");

Components.utils.importGlobalProperties(["URL"]);

let TransactionsHistory = [];
TransactionsHistory.__proto__ = {
  __proto__: Array.prototype,

  
  
  _undoPosition: 0,
  get undoPosition() this._undoPosition,

  
  get topUndoEntry() {
    return this.undoPosition < this.length ? this[this.undoPosition] : null;
  },
  get topRedoEntry() {
    return this.undoPosition > 0 ? this[this.undoPosition - 1] : null;
  },

  
  
  
  
  proxifiedToRaw: new WeakMap(),

  






  proxifyTransaction: function (aRawTransaction) {
    let proxy = Object.freeze({
      transact() {
        return TransactionsManager.transact(this);
      }
    });
    this.proxifiedToRaw.set(proxy, aRawTransaction);
    return proxy;
  },

  






  isProxifiedTransactionObject(aValue) {
    return this.proxifiedToRaw.has(aValue);
  },

  






  getRawTransaction(aProxy) {
    return this.proxifiedToRaw.get(aProxy);
  },

  











  add(aProxifiedTransaction, aForceNewEntry = false) {
    if (!this.isProxifiedTransactionObject(aProxifiedTransaction))
      throw new Error("aProxifiedTransaction is not a proxified transaction");

    if (this.length == 0 || aForceNewEntry) {
      this.clearRedoEntries();
      this.unshift([aProxifiedTransaction]);
    }
    else {
      this[this.undoPosition].unshift(aProxifiedTransaction);
    }
  },

  


  clearUndoEntries() {
    if (this.undoPosition < this.length)
      this.splice(this.undoPosition);
  },

  


  clearRedoEntries() {
    if (this.undoPosition > 0) {
      this.splice(0, this.undoPosition);
      this._undoPosition = 0;
    }
  },

  


  clearAllEntries() {
    if (this.length > 0) {
      this.splice(0);
      this._undoPosition = 0;
    }
  }
};


let PlacesTransactions = {
  


  batch(aToBatch) {
    let batchFunc;
    if (Array.isArray(aToBatch)) {
      if (aToBatch.length == 0)
        throw new Error("aToBatch must not be an empty array");

      if (aToBatch.some(
           o => !TransactionsHistory.isProxifiedTransactionObject(o))) {
        throw new Error("aToBatch contains non-transaction element");
      }
      return TransactionsManager.batch(function* () {
        for (let txn of aToBatch) {
          try {
            yield txn.transact();
          }
          catch(ex) {
            console.error(ex);
          }
        }
      });
    }
    if (typeof(aToBatch) == "function") {
      return TransactionsManager.batch(aToBatch);
    }

    throw new Error("aToBatch must be either a function or a transactions array");
  },

  








  undo() {
    return TransactionsManager.undo();
  },

  








  redo() {
    return TransactionsManager.redo();
  },

  













  clearTransactionsHistory(aUndoEntries = true, aRedoEntries = true) {
    return TransactionsManager.clearTransactionsHistory(aUndoEntries, aRedoEntries);
  },

  


  get length() {
    return TransactionsHistory.length;
  },

  











  entry(aIndex) {
    if (!Number.isInteger(aIndex) || aIndex < 0 || aIndex >= this.length)
      throw new Error("Invalid index");

    return TransactionsHistory[aIndex];
  },

  





  get undoPosition() {
    return TransactionsHistory.undoPosition;
  },

  


  get topUndoEntry() {
    return TransactionsHistory.topUndoEntry;
  },

  


  get topRedoEntry() {
    return TransactionsHistory.topRedoEntry;
  }
};










function Enqueuer() {
  this._promise = Promise.resolve();
}
Enqueuer.prototype = {
  








  enqueue(aFunc) {
    let promise = this._promise.then(Task.async(aFunc));

    
    this._promise = promise.catch(console.error);
    return promise;
  },

  







  alsoWaitFor(aPromise) {
    
    
    let promise = aPromise.catch(console.error);
    this._promise = Promise.all([this._promise, promise]);
  },

  


  get promise() {
    return this._promise;
  }
};

let TransactionsManager = {
  
  
  _mainEnqueuer: new Enqueuer(),
  _transactEnqueuer: new Enqueuer(),

  
  
  _batching: false,

  
  
  
  _createdBatchEntry: false,

  
  
  
  _executedTransactions: new WeakSet(),

  transact(aTxnProxy) {
    let rawTxn = TransactionsHistory.getRawTransaction(aTxnProxy);
    if (!rawTxn)
      throw new Error("|transact| was called with an unexpected object");

    if (this._executedTransactions.has(rawTxn))
      throw new Error("Transactions objects may not be recycled.");

    
    
    this._executedTransactions.add(rawTxn);

    let promise = this._transactEnqueuer.enqueue(function* () {
      
      
      let retval = yield rawTxn.execute();

      let forceNewEntry = !this._batching || !this._createdBatchEntry;
      TransactionsHistory.add(aTxnProxy, forceNewEntry);
      if (this._batching)
        this._createdBatchEntry = true;

      this._updateCommandsOnActiveWindow();
      return retval;
    }.bind(this));
    this._mainEnqueuer.alsoWaitFor(promise);
    return promise;
  },

  batch(aTask) {
    return this._mainEnqueuer.enqueue(function* () {
      this._batching = true;
      this._createdBatchEntry = false;
      let rv;
      try {
        
        rv = (yield Task.spawn(aTask));
      }
      finally {
        this._batching = false;
        this._createdBatchEntry = false;
      }
      return rv;
    }.bind(this));
  },

  


  undo() {
    let promise = this._mainEnqueuer.enqueue(function* () {
      let entry = TransactionsHistory.topUndoEntry;
      if (!entry)
        return;

      for (let txnProxy of entry) {
        try {
          yield TransactionsHistory.getRawTransaction(txnProxy).undo();
        }
        catch(ex) {
          
          
          console.error(ex,
                        "Couldn't undo a transaction, clearing all undo entries.");
          TransactionsHistory.clearUndoEntries();
          return;
        }
      }
      TransactionsHistory._undoPosition++;
      this._updateCommandsOnActiveWindow();
    }.bind(this));
    this._transactEnqueuer.alsoWaitFor(promise);
    return promise;
  },

  


  redo() {
    let promise = this._mainEnqueuer.enqueue(function* () {
      let entry = TransactionsHistory.topRedoEntry;
      if (!entry)
        return;

      for (let i = entry.length - 1; i >= 0; i--) {
        let transaction = TransactionsHistory.getRawTransaction(entry[i]);
        try {
          if (transaction.redo)
            yield transaction.redo();
          else
            yield transaction.execute();
        }
        catch(ex) {
          
          
          console.error(ex,
                        "Couldn't redo a transaction, clearing all redo entries.");
          TransactionsHistory.clearRedoEntries();
          return;
        }
      }
      TransactionsHistory._undoPosition--;
      this._updateCommandsOnActiveWindow();
    }.bind(this));

    this._transactEnqueuer.alsoWaitFor(promise);
    return promise;
  },

  clearTransactionsHistory(aUndoEntries, aRedoEntries) {
    let promise = this._mainEnqueuer.enqueue(function* () {
      if (aUndoEntries && aRedoEntries)
        TransactionsHistory.clearAllEntries();
      else if (aUndoEntries)
        TransactionsHistory.clearUndoEntries();
      else if (aRedoEntries)
        TransactionsHistory.clearRedoEntries();
      else
        throw new Error("either aUndoEntries or aRedoEntries should be true");
    }.bind(this));

    this._transactEnqueuer.alsoWaitFor(promise);
    return promise;
  },

  
  
  _updateCommandsOnActiveWindow() {
    
    try {
      let win = Services.focus.activeWindow;
      if (win)
        win.updateCommands("undo");
    }
    catch(ex) { console.error(ex, "Couldn't update undo commands"); }
  }
};

















function DefineTransaction(aRequiredProps = [], aOptionalProps = []) {
  for (let prop of [...aRequiredProps, ...aOptionalProps]) {
    if (!DefineTransaction.inputProps.has(prop))
      throw new Error("Property '" + prop + "' is not defined");
  }

  let ctor = function (aInput) {
    
    
    
    if (this == PlacesTransactions)
      return new ctor(aInput);

    if (aRequiredProps.length > 0 || aOptionalProps.length > 0) {
      
      let input = DefineTransaction.verifyInput(aInput, aRequiredProps,
                                                aOptionalProps);
      let executeArgs = [this,
                         ...[input[prop] for (prop of aRequiredProps)],
                         ...[input[prop] for (prop of aOptionalProps)]];
      this.execute = Function.bind.apply(this.execute, executeArgs);
    }
    return TransactionsHistory.proxifyTransaction(this);
  };
  return ctor;
}

function simpleValidateFunc(aCheck) {
  return v => {
    if (!aCheck(v))
      throw new Error("Invalid value");
    return v;
  };
}

DefineTransaction.strValidate = simpleValidateFunc(v => typeof(v) == "string");
DefineTransaction.strOrNullValidate =
  simpleValidateFunc(v => typeof(v) == "string" || v === null);
DefineTransaction.indexValidate =
  simpleValidateFunc(v => Number.isInteger(v) &&
                          v >= PlacesUtils.bookmarks.DEFAULT_INDEX);
DefineTransaction.guidValidate =
  simpleValidateFunc(v => /^[a-zA-Z0-9\-_]{12}$/.test(v));

function isPrimitive(v) {
  return v === null || (typeof(v) != "object" && typeof(v) != "function");
}

DefineTransaction.annotationObjectValidate = function (obj) {
  let checkProperty = (aPropName, aRequired, aCheckFunc) => {
    if (aPropName in obj)
      return aCheckFunc(obj[aPropName]);

    return !aRequired;
  };

  if (obj &&
      checkProperty("name", true, v => typeof(v) == "string" && v.length > 0) &&
      checkProperty("expires", false, Number.isInteger) &&
      checkProperty("flags", false, Number.isInteger) &&
      checkProperty("value", false, isPrimitive) ) {
    
    let validKeys = ["name", "value", "flags", "expires"];
    if (Object.keys(obj).every( (k) => validKeys.indexOf(k) != -1 ))
      return obj;
  }
  throw new Error("Invalid annotation object");
};

DefineTransaction.urlValidate = function(url) {
  
  
  if (url instanceof Components.interfaces.nsIURI)
    return url;
  let spec = url instanceof URL ? url.href : url;
  return NetUtil.newURI(spec);
};

DefineTransaction.inputProps = new Map();
DefineTransaction.defineInputProps =
function (aNames, aValidationFunction, aDefaultValue) {
  for (let name of aNames) {
    
    let propName = name;
    this.inputProps.set(propName, {
      validateValue: function (aValue) {
        if (aValue === undefined)
          return aDefaultValue;
        try {
          return aValidationFunction(aValue);
        }
        catch(ex) {
          throw new Error(`Invalid value for input property ${propName}`);
        }
      },

      validateInput: function (aInput, aRequired) {
        if (aRequired && !(propName in aInput))
          throw new Error(`Required input property is missing: ${propName}`);
        return this.validateValue(aInput[propName]);
      },

      isArrayProperty: false
    });
  }
};

DefineTransaction.defineArrayInputProp =
function (aName, aBasePropertyName) {
  let baseProp = this.inputProps.get(aBasePropertyName);
  if (!baseProp)
    throw new Error(`Unknown input property: ${aBasePropertyName}`);

  this.inputProps.set(aName, {
    validateValue: function (aValue) {
      if (aValue == undefined)
        return [];

      if (!Array.isArray(aValue))
        throw new Error(`${aName} input property value must be an array`);

      
      
      return [for (e of aValue) baseProp.validateValue(e)];
    },

    
    
    
    validateInput: function (aInput, aRequired) {
      if (aName in aInput) {
        
        if (aBasePropertyName in aInput) {
          throw new Error(`It is not allowed to set both ${aName} and
                          ${aBasePropertyName} as  input properties`);
        }
        let array = this.validateValue(aInput[aName]);
        if (aRequired && array.length == 0) {
          throw new Error(`Empty array passed for required input property:
                           ${aName}`);
        }
        return array;
      }
      
      
      if (aRequired && !(aBasePropertyName in aInput))
        throw new Error(`Required input property is missing: ${aName}`);

      if (aBasePropertyName in aInput)
        return [baseProp.validateValue(aInput[aBasePropertyName])];

      return [];
    },

    isArrayProperty: true
  });
};

DefineTransaction.validatePropertyValue =
function (aProp, aInput, aRequired) {
  return this.inputProps.get(aProp).validateInput(aInput, aRequired);
};

DefineTransaction.getInputObjectForSingleValue =
function (aInput, aRequiredProps, aOptionalProps) {
  
  
  
  
  if (aRequiredProps.length > 1 ||
      (aRequiredProps.length == 0 && aOptionalProps.length > 1)) {
    throw new Error("Transaction input isn't an object");
  }

  let propName = aRequiredProps.length == 1 ?
                 aRequiredProps[0] : aOptionalProps[0];
  let propValue =
    this.inputProps.get(propName).isArrayProperty && !Array.isArray(aInput) ?
    [aInput] : aInput;
  return { [propName]: propValue };
};

DefineTransaction.verifyInput =
function (aInput, aRequiredProps = [], aOptionalProps = []) {
  if (aRequiredProps.length == 0 && aOptionalProps.length == 0)
    return {};

  
  
  
  
  
  
  let input = aInput;
  let isSinglePropertyInput =
    isPrimitive(aInput) ||
    Array.isArray(aInput) ||
    (aInput instanceof Components.interfaces.nsISupports);
  if (isSinglePropertyInput) {
    input =  this.getInputObjectForSingleValue(aInput,
                                               aRequiredProps,
                                               aOptionalProps);
  }

  let fixedInput = { };
  for (let prop of aRequiredProps) {
    fixedInput[prop] = this.validatePropertyValue(prop, input, true);
  }
  for (let prop of aOptionalProps) {
    fixedInput[prop] = this.validatePropertyValue(prop, input, false);
  }

  return fixedInput;
};



DefineTransaction.defineInputProps(["url", "feedUrl", "siteUrl"],
                                   DefineTransaction.urlValidate, null);
DefineTransaction.defineInputProps(["guid", "parentGuid", "newParentGuid"],
                                   DefineTransaction.guidValidate);
DefineTransaction.defineInputProps(["title"],
                                   DefineTransaction.strOrNullValidate, null);
DefineTransaction.defineInputProps(["keyword", "postData", "tag",
                                    "excludingAnnotation"],
                                   DefineTransaction.strValidate, "");
DefineTransaction.defineInputProps(["index", "newIndex"],
                                   DefineTransaction.indexValidate,
                                   PlacesUtils.bookmarks.DEFAULT_INDEX);
DefineTransaction.defineInputProps(["annotation"],
                                   DefineTransaction.annotationObjectValidate);
DefineTransaction.defineArrayInputProp("guids", "guid");
DefineTransaction.defineArrayInputProp("urls", "url");
DefineTransaction.defineArrayInputProp("tags", "tag");
DefineTransaction.defineArrayInputProp("annotations", "annotation");
DefineTransaction.defineArrayInputProp("excludingAnnotations",
                                       "excludingAnnotation");


















function* ExecuteCreateItem(aTransaction, aParentGuid, aCreateItemFunction,
                            aOnUndo = null, aOnRedo = null) {
  let parentId = yield PlacesUtils.promiseItemId(aParentGuid),
      itemId = yield aCreateItemFunction(parentId, ""),
      guid = yield PlacesUtils.promiseItemGuid(itemId);

  
  let dateAdded = 0, lastModified = 0;
  aTransaction.undo = function* () {
    if (dateAdded == 0) {
      dateAdded = PlacesUtils.bookmarks.getItemDateAdded(itemId);
      lastModified = PlacesUtils.bookmarks.getItemLastModified(itemId);
    }
    PlacesUtils.bookmarks.removeItem(itemId);
    if (aOnUndo) {
      yield aOnUndo();
    }
  };
  aTransaction.redo = function* () {
    parentId = yield PlacesUtils.promiseItemId(aParentGuid);
    itemId = yield aCreateItemFunction(parentId, guid);
    if (aOnRedo)
      yield aOnRedo();

    
    
    PlacesUtils.bookmarks.setItemDateAdded(itemId, dateAdded);
    PlacesUtils.bookmarks.setItemLastModified(itemId, lastModified);
    PlacesUtils.bookmarks.setItemLastModified(parentId, dateAdded);
  };
  return guid;
}



















function* createItemsFromBookmarksTree(aBookmarksTree, aRestoring = false,
                                       aExcludingAnnotations = []) {
  function extractLivemarkDetails(aAnnos) {
    let feedURI = null, siteURI = null;
    aAnnos = aAnnos.filter(
      aAnno => {
        switch (aAnno.name) {
        case PlacesUtils.LMANNO_FEEDURI:
          feedURI = NetUtil.newURI(aAnno.value);
          return false;
        case PlacesUtils.LMANNO_SITEURI:
          siteURI = NetUtil.newURI(aAnno.value);
          return false;
        default:
          return true;
        }
      } );
    return [feedURI, siteURI];
  }

  function* createItem(aItem,
                       aParentGuid,
                       aIndex = PlacesUtils.bookmarks.DEFAULT_INDEX) {
    let itemId;
    let guid = aRestoring ? aItem.guid : undefined;
    let parentId = yield PlacesUtils.promiseItemId(aParentGuid);
    let annos = aItem.annos ? [...aItem.annos] : [];
    switch (aItem.type) {
      case PlacesUtils.TYPE_X_MOZ_PLACE: {
        let uri = NetUtil.newURI(aItem.uri);
        itemId = PlacesUtils.bookmarks.insertBookmark(
          parentId, uri, aIndex, aItem.title, guid);
        if ("keyword" in aItem)
          PlacesUtils.bookmarks.setKeywordForBookmark(itemId, aItem.keyword);
        if ("tags" in aItem) {
          PlacesUtils.tagging.tagURI(uri, aItem.tags.split(","));
        }
        break;
      }
      case PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER: {
        
        let [feedURI, siteURI] = extractLivemarkDetails(annos);
        if (!feedURI) {
          itemId = PlacesUtils.bookmarks.createFolder(
              parentId, aItem.title, aIndex, guid);
          if (guid === undefined)
            guid = yield PlacesUtils.promiseItemGuid(itemId);
          if ("children" in aItem) {
            for (let child of aItem.children) {
              yield createItem(child, guid);
            }
          }
        }
        else {
          let livemark =
            yield PlacesUtils.livemarks.addLivemark({ title: aItem.title
                                                    , feedURI: feedURI
                                                    , siteURI: siteURI
                                                    , parentId: parentId
                                                    , index: aIndex
                                                    , guid: guid});
          itemId = livemark.id;
        }
        break;
      }
      case PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR: {
        itemId = PlacesUtils.bookmarks.insertSeparator(parentId, aIndex, guid);
        break;
      }
    }
    if (annos.length > 0) {
      if (!aRestoring && aExcludingAnnotations.length > 0) {
        annos = [for(a of annos)
                 if (aExcludingAnnotations.indexOf(a.name) == -1) a];
      }

      PlacesUtils.setAnnotationsForItem(itemId, annos);
    }

    if (aRestoring) {
      if ("dateAdded" in aItem)
        PlacesUtils.bookmarks.setItemDateAdded(itemId, aItem.dateAdded);
      if ("lastModified" in aItem)
        PlacesUtils.bookmarks.setItemLastModified(itemId, aItem.lastModified);
    }
    return itemId;
  }
  return yield createItem(aBookmarksTree,
                          aBookmarksTree.parentGuid,
                          aBookmarksTree.index);
}








let PT = PlacesTransactions;









PT.NewBookmark = DefineTransaction(["parentGuid", "url"],
                                   ["index", "title", "keyword", "postData",
                                    "annotations", "tags"]);
PT.NewBookmark.prototype = Object.seal({
  execute: function (aParentGuid, aURI, aIndex, aTitle,
                     aKeyword, aPostData, aAnnos, aTags) {
    return ExecuteCreateItem(this, aParentGuid,
      function (parentId, guidToRestore = "") {
        let itemId = PlacesUtils.bookmarks.insertBookmark(
          parentId, aURI, aIndex, aTitle, guidToRestore);
        if (aKeyword)
          PlacesUtils.bookmarks.setKeywordForBookmark(itemId, aKeyword);
        if (aPostData)
          PlacesUtils.setPostDataForBookmark(itemId, aPostData);
        if (aAnnos.length)
          PlacesUtils.setAnnotationsForItem(itemId, aAnnos);
        if (aTags.length > 0) {
          let currentTags = PlacesUtils.tagging.getTagsForURI(aURI);
          aTags = [t for (t of aTags) if (currentTags.indexOf(t) == -1)];
          PlacesUtils.tagging.tagURI(aURI, aTags);
        }

        return itemId;
      },
      function _additionalOnUndo() {
        if (aTags.length > 0)
          PlacesUtils.tagging.untagURI(aURI, aTags);
      });
  }
});









PT.NewFolder = DefineTransaction(["parentGuid", "title"],
                                 ["index", "annotations"]);
PT.NewFolder.prototype = Object.seal({
  execute: function (aParentGuid, aTitle, aIndex, aAnnos) {
    return ExecuteCreateItem(this,  aParentGuid,
      function(parentId, guidToRestore = "") {
        let itemId = PlacesUtils.bookmarks.createFolder(
          parentId, aTitle, aIndex, guidToRestore);
        if (aAnnos.length > 0)
          PlacesUtils.setAnnotationsForItem(itemId, aAnnos);
        return itemId;
      });
  }
});










PT.NewSeparator = DefineTransaction(["parentGuid"], ["index"]);
PT.NewSeparator.prototype = Object.seal({
  execute: function (aParentGuid, aIndex) {
    return ExecuteCreateItem(this, aParentGuid,
      function (parentId, guidToRestore = "") {
        let itemId = PlacesUtils.bookmarks.insertSeparator(
          parentId, aIndex, guidToRestore);
        return itemId;
      });
  }
});











PT.NewLivemark = DefineTransaction(["feedUrl", "title", "parentGuid"],
                                   ["siteUrl", "index", "annotations"]);
PT.NewLivemark.prototype = Object.seal({
  execute: function* (aFeedURI, aTitle, aParentGuid, aSiteURI, aIndex, aAnnos) {
    let livemarkInfo = { title: aTitle
                       , feedURI: aFeedURI
                       , siteURI: aSiteURI
                       , index: aIndex };
    let createItem = function* () {
      livemarkInfo.parentId = yield PlacesUtils.promiseItemId(aParentGuid);
      let livemark = yield PlacesUtils.livemarks.addLivemark(livemarkInfo);
      if (aAnnos.length > 0)
        PlacesUtils.setAnnotationsForItem(livemark.id, aAnnos);

      if ("dateAdded" in livemarkInfo) {
        PlacesUtils.bookmarks.setItemDateAdded(livemark.id,
                                               livemarkInfo.dateAdded);
        PlacesUtils.bookmarks.setItemLastModified(livemark.id,
                                                  livemarkInfo.lastModified);
      }
      return livemark;
    };

    let livemark = yield createItem();
    this.undo = function* () {
      livemarkInfo.guid = livemark.guid;
      if (!("dateAdded" in livemarkInfo)) {
        livemarkInfo.dateAdded =
          PlacesUtils.bookmarks.getItemDateAdded(livemark.id);
        livemarkInfo.lastModified =
          PlacesUtils.bookmarks.getItemLastModified(livemark.id);
      }
      yield PlacesUtils.livemarks.removeLivemark(livemark);
    };
    this.redo = function* () {
      livemark = yield createItem();
    };
    return livemark.guid;
  }
});







PT.Move = DefineTransaction(["guid", "newParentGuid"], ["newIndex"]);
PT.Move.prototype = Object.seal({
  execute: function* (aGuid, aNewParentGuid, aNewIndex) {
    let itemId = yield PlacesUtils.promiseItemId(aGuid),
        oldParentId = PlacesUtils.bookmarks.getFolderIdForItem(itemId),
        oldIndex = PlacesUtils.bookmarks.getItemIndex(itemId),
        newParentId = yield PlacesUtils.promiseItemId(aNewParentGuid);

    PlacesUtils.bookmarks.moveItem(itemId, newParentId, aNewIndex);

    let undoIndex = PlacesUtils.bookmarks.getItemIndex(itemId);
    this.undo = () => {
      
      
      if (newParentId == oldParentId && oldIndex > undoIndex)
        PlacesUtils.bookmarks.moveItem(itemId, oldParentId, oldIndex + 1);
      else
        PlacesUtils.bookmarks.moveItem(itemId, oldParentId, oldIndex);
    };
  }
});






PT.EditTitle = DefineTransaction(["guid", "title"]);
PT.EditTitle.prototype = Object.seal({
  execute: function* (aGuid, aTitle) {
    let itemId = yield PlacesUtils.promiseItemId(aGuid),
        oldTitle = PlacesUtils.bookmarks.getItemTitle(itemId);
    PlacesUtils.bookmarks.setItemTitle(itemId, aTitle);
    this.undo = () => { PlacesUtils.bookmarks.setItemTitle(itemId, oldTitle); };
  }
});






PT.EditUrl = DefineTransaction(["guid", "url"]);
PT.EditUrl.prototype = Object.seal({
  execute: function* (aGuid, aURI) {
    let itemId = yield PlacesUtils.promiseItemId(aGuid),
        oldURI = PlacesUtils.bookmarks.getBookmarkURI(itemId),
        oldURITags = PlacesUtils.tagging.getTagsForURI(oldURI),
        newURIAdditionalTags = null;
    PlacesUtils.bookmarks.changeBookmarkURI(itemId, aURI);

    
    if (oldURITags.length > 0) {
      
      if (PlacesUtils.getBookmarksForURI(oldURI, {}).length == 0)
        PlacesUtils.tagging.untagURI(oldURI, oldURITags);

      let currentNewURITags = PlacesUtils.tagging.getTagsForURI(aURI);
      newURIAdditionalTags = [t for (t of oldURITags)
                              if (currentNewURITags.indexOf(t) == -1)];
      if (newURIAdditionalTags)
        PlacesUtils.tagging.tagURI(aURI, newURIAdditionalTags);
    }

    this.undo = () => {
      PlacesUtils.bookmarks.changeBookmarkURI(itemId, oldURI);
      
      if (oldURITags.length > 0) {
        
        if (newURIAdditionalTags && newURIAdditionalTags.length > 0 &&
            PlacesUtils.getBookmarksForURI(aURI, {}).length == 0) {
          PlacesUtils.tagging.untagURI(aURI, newURIAdditionalTags);
        }

        PlacesUtils.tagging.tagURI(oldURI, oldURITags);
      }
    };
  }
});






PT.Annotate = DefineTransaction(["guids", "annotations"]);
PT.Annotate.prototype = {
  *execute(aGuids, aNewAnnos) {
    let undoAnnosForItem = new Map(); 
    for (let guid of aGuids) {
      let itemId = yield PlacesUtils.promiseItemId(guid);
      let currentAnnos = PlacesUtils.getAnnotationsForItem(itemId);

      let undoAnnos = [];
      for (let newAnno of aNewAnnos) {
        let currentAnno = currentAnnos.find(a => a.name == newAnno.name);
        if (!!currentAnno) {
          undoAnnos.push(currentAnno);
        }
        else {
          
          undoAnnos.push({ name: newAnno.name });
        }
      }
      undoAnnosForItem.set(itemId, undoAnnos);

      PlacesUtils.setAnnotationsForItem(itemId, aNewAnnos);
    }

    this.undo = function() {
      for (let [itemId, undoAnnos] of undoAnnosForItem) {
        PlacesUtils.setAnnotationsForItem(itemId, undoAnnos);
      }
    };
    this.redo = function* () {
      for (let guid of aGuids) {
        let itemId = yield PlacesUtils.promiseItemId(guid);
        PlacesUtils.setAnnotationsForItem(itemId, aNewAnnos);
      }
    };
  }
};






PT.EditKeyword = DefineTransaction(["guid", "keyword"]);
PT.EditKeyword.prototype = Object.seal({
  execute: function* (aGuid, aKeyword) {
    let itemId = yield PlacesUtils.promiseItemId(aGuid),
        oldKeyword = PlacesUtils.bookmarks.getKeywordForBookmark(itemId);
    PlacesUtils.bookmarks.setKeywordForBookmark(itemId, aKeyword);
    this.undo = () => {
      PlacesUtils.bookmarks.setKeywordForBookmark(itemId, oldKeyword);
    };
  }
});






PT.SortByName = DefineTransaction(["guid"]);
PT.SortByName.prototype = {
  execute: function* (aGuid) {
    let itemId = yield PlacesUtils.promiseItemId(aGuid),
        oldOrder = [],  
        contents = PlacesUtils.getFolderContents(itemId, false, false).root,
        count = contents.childCount;

    
    let newOrder = [], 
        preSep   = []; 
    let sortingMethod = (a, b) => {
      if (PlacesUtils.nodeIsContainer(a) && !PlacesUtils.nodeIsContainer(b))
        return -1;
      if (!PlacesUtils.nodeIsContainer(a) && PlacesUtils.nodeIsContainer(b))
        return 1;
      return a.title.localeCompare(b.title);
    };

    for (let i = 0; i < count; ++i) {
      let node = contents.getChild(i);
      oldOrder[node.itemId] = i;
      if (PlacesUtils.nodeIsSeparator(node)) {
        if (preSep.length > 0) {
          preSep.sort(sortingMethod);
          newOrder = newOrder.concat(preSep);
          preSep.splice(0, preSep.length);
        }
        newOrder.push(node);
      }
      else
        preSep.push(node);
    }
    contents.containerOpen = false;

    if (preSep.length > 0) {
      preSep.sort(sortingMethod);
      newOrder = newOrder.concat(preSep);
    }

    
    let callback = {
      runBatched: function() {
        for (let i = 0; i < newOrder.length; ++i) {
          PlacesUtils.bookmarks.setItemIndex(newOrder[i].itemId, i);
        }
      }
    };
    PlacesUtils.bookmarks.runInBatchMode(callback, null);

    this.undo = () => {
      let callback = {
        runBatched: function() {
          for (let item in oldOrder) {
            PlacesUtils.bookmarks.setItemIndex(item, oldOrder[item]);
          }
        }
      };
      PlacesUtils.bookmarks.runInBatchMode(callback, null);
    };
  }
};






PT.Remove = DefineTransaction(["guids"]);
PT.Remove.prototype = {
  *execute(aGuids) {
    function promiseBookmarksTree(guid) {
      try {
        return PlacesUtils.promiseBookmarksTree(guid);
      }
      catch(ex) {
        throw new Error("Failed to get info for the specified item (guid: " +
                        guid + "). Ex: " + ex);
      }
    }
    let toRestore = [for (guid of aGuids) yield promiseBookmarksTree(guid)];

    let removeThem = Task.async(function* () {
      for (let guid of aGuids) {
        PlacesUtils.bookmarks.removeItem(yield PlacesUtils.promiseItemId(guid));
      }
    });
    yield removeThem();

    this.undo = Task.async(function* () {
      for (let info of toRestore) {
        yield createItemsFromBookmarksTree(info, true);
      }
    });
    this.redo = removeThem;
  }
};






PT.RemoveBookmarksForUrls = DefineTransaction(["urls"]);
PT.RemoveBookmarksForUrls.prototype = {
  *execute(aUrls) {
    let guids = [];
    for (let url of aUrls) {
      yield PlacesUtils.bookmarks.fetch({ url }, info => {
        guids.push(info.guid);
      });
    }
    let removeTxn = TransactionsHistory.getRawTransaction(PT.Remove(guids));
    yield removeTxn.execute();
    this.undo = removeTxn.undo.bind(removeTxn);
    this.redo = removeTxn.redo.bind(removeTxn);
  }
};






PT.Tag = DefineTransaction(["urls", "tags"]);
PT.Tag.prototype = {
  execute: function* (aURIs, aTags) {
    let onUndo = [], onRedo = [];
    for (let uri of aURIs) {
      
      let currentURI = uri;

      let promiseIsBookmarked = function* () {
        let deferred = Promise.defer();
        PlacesUtils.asyncGetBookmarkIds(
          currentURI, ids => { deferred.resolve(ids.length > 0); });
        return deferred.promise;
      };

      if (yield promiseIsBookmarked(currentURI)) {
        
        let createTxn = TransactionsHistory.getRawTransaction(
          PT.NewBookmark({ url: currentURI
                         , tags: aTags
                         , parentGuid: PlacesUtils.bookmarks.unfiledGuid }));
        yield createTxn.execute();
        onUndo.unshift(createTxn.undo.bind(createTxn));
        onRedo.push(createTxn.redo.bind(createTxn));
      }
      else {
        let currentTags = PlacesUtils.tagging.getTagsForURI(currentURI);
        let newTags = [t for (t of aTags) if (currentTags.indexOf(t) == -1)];
        PlacesUtils.tagging.tagURI(currentURI, newTags);
        onUndo.unshift(() => {
          PlacesUtils.tagging.untagURI(currentURI, newTags);
        });
        onRedo.push(() => {
          PlacesUtils.tagging.tagURI(currentURI, newTags);
        });
      }
    }
    this.undo = function* () {
      for (let f of onUndo) {
        yield f();
      }
    };
    this.redo = function* () {
      for (let f of onRedo) {
        yield f();
      }
    };
  }
};









PT.Untag = DefineTransaction(["urls"], ["tags"]);
PT.Untag.prototype = {
  execute: function* (aURIs, aTags) {
    let onUndo = [], onRedo = [];
    for (let uri of aURIs) {
      
      let currentURI = uri;
      let tagsToRemove;
      let tagsSet = PlacesUtils.tagging.getTagsForURI(currentURI);
      if (aTags.length > 0)
        tagsToRemove = [t for (t of aTags) if (tagsSet.indexOf(t) != -1)];
      else
        tagsToRemove = tagsSet;
      PlacesUtils.tagging.untagURI(currentURI, tagsToRemove);
      onUndo.unshift(() => {
        PlacesUtils.tagging.tagURI(currentURI, tagsToRemove);
      });
      onRedo.push(() => {
        PlacesUtils.tagging.untagURI(currentURI, tagsToRemove);
      });
    }
    this.undo = function* () {
      for (let f of onUndo) {
        yield f();
      }
    };
    this.redo = function* () {
      for (let f of onRedo) {
        yield f();
      }
    };
  }
};







PT.Copy = DefineTransaction(["guid", "newParentGuid"],
                            ["newIndex", "excludingAnnotations"]);
PT.Copy.prototype = {
  execute: function* (aGuid, aNewParentGuid, aNewIndex, aExcludingAnnotations) {
    let creationInfo = null;
    try {
      creationInfo = yield PlacesUtils.promiseBookmarksTree(aGuid);
    }
    catch(ex) {
      throw new Error("Failed to get info for the specified item (guid: " +
                      aGuid + "). Ex: " + ex);
    }
    creationInfo.parentGuid = aNewParentGuid;
    creationInfo.index = aNewIndex;

    let newItemId =
      yield createItemsFromBookmarksTree(creationInfo, false,
                                         aExcludingAnnotations);
    let newItemInfo = null;
    this.undo = function* () {
      if (!newItemInfo) {
        let newItemGuid = yield PlacesUtils.promiseItemGuid(newItemId);
        newItemInfo = yield PlacesUtils.promiseBookmarksTree(newItemGuid);
      }
      PlacesUtils.bookmarks.removeItem(newItemId);
    };
    this.redo = function* () {
      newItemId = yield createItemsFromBookmarksTree(newItemInfo, true);
    }

    return yield PlacesUtils.promiseItemGuid(newItemId);
  }
};
