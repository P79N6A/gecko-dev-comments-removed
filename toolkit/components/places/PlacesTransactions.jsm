



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



function updateCommandsOnActiveWindow() {
  
  try {
    let win = Services.focus.activeWindow;
    if (win)
      win.updateCommands("undo");
  }
  catch(ex) { console.error(ex, "Couldn't update undo commands"); }
}





let TransactionsHistory = [];
TransactionsHistory.__proto__ = {
  __proto__: Array.prototype,

  
  
  _undoPosition: 0,
  get undoPosition() this._undoPosition,

  
  get topUndoEntry() this.undoPosition < this.length ?
                     this[this.undoPosition] : null,
  get topRedoEntry() this.undoPosition > 0 ?
                     this[this.undoPosition - 1] : null,

  
  
  
  
  proxifiedToRaw: new WeakMap(),

  






  proxifyTransaction: function (aRawTransaction) {
    let proxy = Object.freeze({});
    this.proxifiedToRaw.set(proxy, aRawTransaction);
    return proxy;
  },

  






  isProxifiedTransactionObject:
  function (aValue) this.proxifiedToRaw.has(aValue),

  






  getRawTransaction: function (aProxy) this.proxifiedToRaw.get(aProxy),

  


  undo: function* () {
    let entry = this.topUndoEntry;
    if (!entry)
      return;

    for (let transaction of entry) {
      try {
        yield TransactionsHistory.getRawTransaction(transaction).undo();
      }
      catch(ex) {
        
        
        console.error(ex,
                      "Couldn't undo a transaction, clearing all undo entries.");
        this.clearUndoEntries();
        return;
      }
    }
    this._undoPosition++;
    updateCommandsOnActiveWindow();
  },

  


  redo: function* () {
    let entry = this.topRedoEntry;
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
        this.clearRedoEntries();
        return;
      }
    }
    this._undoPosition--;
    updateCommandsOnActiveWindow();
  },

  











  add: function (aProxifiedTransaction, aForceNewEntry = false) {
    if (!this.isProxifiedTransactionObject(aProxifiedTransaction))
      throw new Error("aProxifiedTransaction is not a proxified transaction");

    if (this.length == 0 || aForceNewEntry) {
      this.clearRedoEntries();
      this.unshift([aProxifiedTransaction]);
    }
    else {
      this[this.undoPosition].unshift(aProxifiedTransaction);
    }
    updateCommandsOnActiveWindow();
  },

  


  clearUndoEntries: function () {
    if (this.undoPosition < this.length)
      this.splice(this.undoPosition);
  },

  


  clearRedoEntries: function () {
    if (this.undoPosition > 0) {
      this.splice(0, this.undoPosition);
      this._undoPosition = 0;
    }
  },

  


  clearAllEntries: function () {
    if (this.length > 0) {
      this.splice(0);
      this._undoPosition = 0;
    }
  }
};




let currentTask = Promise.resolve();
function Serialize(aTask) {
  
  return currentTask = currentTask.then( () => Task.spawn(aTask) )
                                  .then(null, Components.utils.reportError);
}




let executedTransactions = new WeakMap(); 
executedTransactions.add = k => executedTransactions.set(k, null);

let PlacesTransactions = {
  

































  transact: function (aToTransact) {
    let isGeneratorObj =
      o => Object.prototype.toString.call(o) ==  "[object Generator]";

    let generator = null;
    if (typeof(aToTransact) == "function") {
      generator = aToTransact();
      if (!isGeneratorObj(generator))
        throw new Error("aToTransact is not a generator function");
    }
    else if (!TransactionsHistory.isProxifiedTransactionObject(aToTransact)) {
      throw new Error("aToTransact is not a valid transaction object");
    }
    else if (executedTransactions.has(aToTransact)) {
      throw new Error("Transactions objects may not be recycled.");
    }

    return Serialize(function* () {
      
      
      
      
      
      
      let forceNewEntry = true;
      function* transactOneTransaction(aTransaction) {
        let retval =
          yield TransactionsHistory.getRawTransaction(aTransaction).execute();
        executedTransactions.add(aTransaction);
        TransactionsHistory.add(aTransaction, forceNewEntry);
        forceNewEntry = false;
        return retval;
      }

      function* transactBatch(aGenerator) {
        let error = false;
        let sendValue = undefined;
        while (true) {
          let next = error ?
                     aGenerator.throw(sendValue) : aGenerator.next(sendValue);
          sendValue = next.value;
          if (isGeneratorObj(sendValue)) {
            sendValue = yield transactBatch(sendValue);
          }
          else if (typeof(sendValue) == "object" && sendValue) {
            if (TransactionsHistory.isProxifiedTransactionObject(sendValue)) {
              if (executedTransactions.has(sendValue)) {
                sendValue = new Error("Transactions may not be recycled.");
                error = true;
              }
              else {
                sendValue = yield transactOneTransaction(sendValue);
              }
            }
            else if ("then" in sendValue) {
              sendValue = yield sendValue;
            }
          }
          if (next.done)
            break;
        }
        return sendValue;
      }

      if (generator)
        return yield transactBatch(generator);
      else
        return yield transactOneTransaction(aToTransact);
    }.bind(this));
  },

  








  undo: function () Serialize(() => TransactionsHistory.undo()),

  








  redo: function () Serialize(() => TransactionsHistory.redo()),

  













  clearTransactionsHistory:
  function (aUndoEntries = true, aRedoEntries = true) {
    return Serialize(function* () {
      if (aUndoEntries && aRedoEntries)
        TransactionsHistory.clearAllEntries();
      else if (aUndoEntries)
        TransactionsHistory.clearUndoEntries();
      else if (aRedoEntries)
        TransactionsHistory.clearRedoEntries();
      else
        throw new Error("either aUndoEntries or aRedoEntries should be true");
    });
  },

  


  get length() TransactionsHistory.length,

  











  entry: function (aIndex) {
    if (!Number.isInteger(aIndex) || aIndex < 0 || aIndex >= this.length)
      throw new Error("Invalid index");

    return TransactionsHistory[aIndex];
  },

  





  get undoPosition() TransactionsHistory.undoPosition,

  


  get topUndoEntry() TransactionsHistory.topUndoEntry,

  


  get topRedoEntry() TransactionsHistory.topRedoEntry
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

DefineTransaction.isStr = v => typeof(v) == "string";
DefineTransaction.isStrOrNull = v => typeof(v) == "string" || v === null;
DefineTransaction.isURI = v => v instanceof Components.interfaces.nsIURI;
DefineTransaction.isIndex = v => Number.isInteger(v) &&
                                 v >= PlacesUtils.bookmarks.DEFAULT_INDEX;
DefineTransaction.isGUID = v => /^[a-zA-Z0-9\-_]{12}$/.test(v);
DefineTransaction.isPrimitive = v => v === null || (typeof(v) != "object" &&
                                                    typeof(v) != "function");
DefineTransaction.isAnnotationObject = function (obj) {
  let checkProperty = (aPropName, aRequired, aCheckFunc) => {
    if (aPropName in obj)
      return aCheckFunc(obj[aPropName]);

    return !aRequired;
  };

  if (obj &&
      checkProperty("name",    true,  DefineTransaction.isStr)      &&
      checkProperty("expires", false, Number.isInteger) &&
      checkProperty("flags",   false, Number.isInteger) &&
      checkProperty("value",   false, DefineTransaction.isPrimitive) ) {
    
    let validKeys = ["name", "value", "flags", "expires"];
    if (Object.keys(obj).every( (k) => validKeys.indexOf(k) != -1 ))
      return true;
  }
  return false;
};

DefineTransaction.inputProps = new Map();
DefineTransaction.defineInputProps =
function (aNames, aValidationFunction, aDefaultValue) {
  for (let name of aNames) {
    this.inputProps.set(name, {
      validate:     aValidationFunction,
      defaultValue: aDefaultValue,
      isGUIDProp:   false
    });
  }
};

DefineTransaction.defineArrayInputProp =
function (aName, aValidationFunction, aDefaultValue) {
  this.inputProps.set(aName, {
    validate:     (v) => Array.isArray(v) && v.every(aValidationFunction),
    defaultValue: aDefaultValue,
    isGUIDProp:   false
  });
};

DefineTransaction.verifyPropertyValue =
function (aProp, aValue, aRequired) {
  if (aValue === undefined) {
    if (aRequired)
      throw new Error("Required property is missing: " + aProp);
    return this.inputProps.get(aProp).defaultValue;
  }

  if (!this.inputProps.get(aProp).validate(aValue))
    throw new Error("Invalid value for property: " + aProp);

  if (Array.isArray(aValue)) {
    
    
    return Components.utils.cloneInto(aValue, {});
  }

  return aValue;
};

DefineTransaction.verifyInput =
function (aInput, aRequired = [], aOptional = []) {
  if (aRequired.length == 0 && aOptional.length == 0)
    return {};

  
  
  
  
  
  
  let isSinglePropertyInput =
    this.isPrimitive(aInput) ||
    (aInput instanceof Components.interfaces.nsISupports);
  let fixedInput = { };
  if (aRequired.length > 0) {
    if (isSinglePropertyInput) {
      if (aRequired.length == 1) {
        let prop = aRequired[0], value = aInput;
        value = this.verifyPropertyValue(prop, value, true);
        fixedInput[prop] = value;
      }
      else {
        throw new Error("Transaction input isn't an object");
      }
    }
    else {
      for (let prop of aRequired) {
        let value = this.verifyPropertyValue(prop, aInput[prop], true);
        fixedInput[prop] = value;
      }
    }
  }

  if (aOptional.length > 0) {
    if (isSinglePropertyInput && !aRequired.length > 0) {
      if (aOptional.length == 1) {
        let prop = aOptional[0], value = aInput;
        value = this.verifyPropertyValue(prop, value, true);
        fixedInput[prop] = value;
      }
      else if (aInput !== null && aInput !== undefined) {
        throw new Error("Transaction input isn't an object");
      }
    }
    else {
      for (let prop of aOptional) {
        let value = this.verifyPropertyValue(prop, aInput[prop], false);
        if (value !== undefined)
          fixedInput[prop] = value;
        else
          fixedInput[prop] = this.defaultValues[prop];
      }
    }
  }

  return fixedInput;
};



DefineTransaction.defineInputProps(["uri", "feedURI", "siteURI"],
                                   DefineTransaction.isURI, null);
DefineTransaction.defineInputProps(["GUID", "parentGUID", "newParentGUID"],
                                   DefineTransaction.isGUID);
DefineTransaction.defineInputProps(["title"],
                                   DefineTransaction.isStrOrNull, null);
DefineTransaction.defineInputProps(["keyword", "postData"],
                                   DefineTransaction.isStr, "");
DefineTransaction.defineInputProps(["index", "newIndex"],
                                   DefineTransaction.isIndex,
                                   PlacesUtils.bookmarks.DEFAULT_INDEX);
DefineTransaction.defineInputProps(["annotationObject"],
                                   DefineTransaction.isAnnotationObject);
DefineTransaction.defineArrayInputProp("tags",
                                       DefineTransaction.isStr, null);
DefineTransaction.defineArrayInputProp("annotations",
                                       DefineTransaction.isAnnotationObject,
                                       null);


















function* ExecuteCreateItem(aTransaction, aParentGUID, aCreateItemFunction,
                            aOnUndo = null, aOnRedo = null) {
  let parentId = yield PlacesUtils.promiseItemId(aParentGUID),
      itemId = yield aCreateItemFunction(parentId, ""),
      guid = yield PlacesUtils.promiseItemGUID(itemId);

  
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
    parentId = yield PlacesUtils.promiseItemId(aParentGUID);
    itemId = yield aCreateItemFunction(parentId, guid);
    if (aOnRedo)
      yield aOnRedo();

    
    
    PlacesUtils.bookmarks.setItemDateAdded(itemId, dateAdded);
    PlacesUtils.bookmarks.setItemLastModified(itemId, lastModified);
    PlacesUtils.bookmarks.setItemLastModified(parentId, dateAdded);
  };
  return guid;
}
















function* createItemsFromBookmarksTree(aBookmarksTree, aRestoring = false) {
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
                       aParentGUID,
                       aIndex = PlacesUtils.bookmarks.DEFAULT_INDEX) {
    let itemId;
    let guid = aRestoring ? aItem.guid : undefined;
    let parentId = yield PlacesUtils.promiseItemId(aParentGUID);
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
            guid = yield PlacesUtils.promiseItemGUID(itemId);
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
    if (annos.length > 0)
      PlacesUtils.setAnnotationsForItem(itemId, annos);

    if (aRestoring) {
      if ("dateAdded" in aItem)
        PlacesUtils.bookmarks.setItemDateAdded(itemId, aItem.dateAdded);
      if ("lastModified" in aItem)
        PlacesUtils.bookmarks.setItemLastModified(itemId, aItem.lastModified);
    }
    return itemId;
  }
  return yield createItem(aBookmarksTree,
                          aBookmarksTree.parentGUID,
                          aBookmarksTree.index);
}








let PT = PlacesTransactions;









PT.NewBookmark = DefineTransaction(["parentGUID", "uri"],
                                   ["index", "title", "keyword", "postData",
                                    "annotations", "tags"]);
PT.NewBookmark.prototype = Object.seal({
  execute: function (aParentGUID, aURI, aIndex, aTitle,
                     aKeyword, aPostData, aAnnos, aTags) {
    return ExecuteCreateItem(this, aParentGUID,
      function (parentId, guidToRestore = "") {
        let itemId = PlacesUtils.bookmarks.insertBookmark(
          parentId, aURI, aIndex, aTitle, guidToRestore);
        if (aKeyword)
          PlacesUtils.bookmarks.setKeywordForBookmark(itemId, aKeyword);
        if (aPostData)
          PlacesUtils.setPostDataForBookmark(itemId, aPostData);
        if (aAnnos)
          PlacesUtils.setAnnotationsForItem(itemId, aAnnos);
        if (aTags && aTags.length > 0) {
          let currentTags = PlacesUtils.tagging.getTagsForURI(aURI);
          aTags = [t for (t of aTags) if (currentTags.indexOf(t) == -1)];
          PlacesUtils.tagging.tagURI(aURI, aTags);
        }

        return itemId;
      },
      function _additionalOnUndo() {
        if (aTags && aTags.length > 0)
          PlacesUtils.tagging.untagURI(aURI, aTags);
      });
  }
});









PT.NewFolder = DefineTransaction(["parentGUID", "title"],
                                 ["index", "annotations"]);
PT.NewFolder.prototype = Object.seal({
  execute: function (aParentGUID, aTitle, aIndex, aAnnos) {
    return ExecuteCreateItem(this,  aParentGUID,
      function(parentId, guidToRestore = "") {
        let itemId = PlacesUtils.bookmarks.createFolder(
          parentId, aTitle, aIndex, guidToRestore);
        if (aAnnos)
          PlacesUtils.setAnnotationsForItem(itemId, aAnnos);
        return itemId;
      });
  }
});










PT.NewSeparator = DefineTransaction(["parentGUID"], ["index"]);
PT.NewSeparator.prototype = Object.seal({
  execute: function (aParentGUID, aIndex) {
    return ExecuteCreateItem(this, aParentGUID,
      function (parentId, guidToRestore = "") {
        let itemId = PlacesUtils.bookmarks.insertSeparator(
          parentId, aIndex, guidToRestore);
        return itemId;
      });
  }
});











PT.NewLivemark = DefineTransaction(["feedURI", "title", "parentGUID"],
                                   ["siteURI", "index", "annotations"]);
PT.NewLivemark.prototype = Object.seal({
  execute: function* (aFeedURI, aTitle, aParentGUID, aSiteURI, aIndex, aAnnos) {
    let livemarkInfo = { title: aTitle
                       , feedURI: aFeedURI
                       , siteURI: aSiteURI
                       , index: aIndex };
    let createItem = function* () {
      livemarkInfo.parentId = yield PlacesUtils.promiseItemId(aParentGUID);
      let livemark = yield PlacesUtils.livemarks.addLivemark(livemarkInfo);
      if (aAnnos)
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







PT.Move = DefineTransaction(["GUID", "newParentGUID"], ["newIndex"]);
PT.Move.prototype = Object.seal({
  execute: function* (aGUID, aNewParentGUID, aNewIndex) {
    let itemId = yield PlacesUtils.promiseItemId(aGUID),
        oldParentId = PlacesUtils.bookmarks.getFolderIdForItem(itemId),
        oldIndex = PlacesUtils.bookmarks.getItemIndex(itemId),
        newParentId = yield PlacesUtils.promiseItemId(aNewParentGUID);

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






PT.EditTitle = DefineTransaction(["GUID", "title"]);
PT.EditTitle.prototype = Object.seal({
  execute: function* (aGUID, aTitle) {
    let itemId = yield PlacesUtils.promiseItemId(aGUID),
        oldTitle = PlacesUtils.bookmarks.getItemTitle(itemId);
    PlacesUtils.bookmarks.setItemTitle(itemId, aTitle);
    this.undo = () => { PlacesUtils.bookmarks.setItemTitle(itemId, oldTitle); };
  }
});






PT.EditURI = DefineTransaction(["GUID", "uri"]);
PT.EditURI.prototype = Object.seal({
  execute: function* (aGUID, aURI) {
    let itemId = yield PlacesUtils.promiseItemId(aGUID),
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






PT.SetItemAnnotation = DefineTransaction(["GUID", "annotationObject"]);
PT.SetItemAnnotation.prototype = {
  execute: function* (aGUID, aAnno) {
    let itemId = yield PlacesUtils.promiseItemId(aGUID), oldAnno;
    if (PlacesUtils.annotations.itemHasAnnotation(itemId, aAnno.name)) {
      
      let flags = {}, expires = {};
      PlacesUtils.annotations.getItemAnnotationInfo(itemId, aAnno.name, flags,
                                                    expires, { });
      let value = PlacesUtils.annotations.getItemAnnotation(itemId, aAnno.name);
      oldAnno = { name: aAnno.name, flags: flags.value,
                  value: value, expires: expires.value };
    }
    else {
      
      oldAnno = { name: aAnno.name };
    }

    PlacesUtils.setAnnotationsForItem(itemId, [aAnno]);
    this.undo = () => { PlacesUtils.setAnnotationsForItem(itemId, [oldAnno]); };
  }
};






PT.EditKeyword = DefineTransaction(["GUID", "keyword"]);
PT.EditKeyword.prototype = Object.seal({
  execute: function* (aGUID, aKeyword) {
    let itemId = yield PlacesUtils.promiseItemId(aGUID),
        oldKeyword = PlacesUtils.bookmarks.getKeywordForBookmark(itemId);
    PlacesUtils.bookmarks.setKeywordForBookmark(itemId, aKeyword);
    this.undo = () => {
      PlacesUtils.bookmarks.setKeywordForBookmark(itemId, oldKeyword);
    };
  }
});






PT.SortByName = DefineTransaction(["GUID"]);
PT.SortByName.prototype = {
  execute: function* (aGUID) {
    let itemId = yield PlacesUtils.promiseItemId(aGUID),
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






PT.Remove = DefineTransaction(["GUID"]);
PT.Remove.prototype = {
  execute: function* (aGUID) {
    const bms = PlacesUtils.bookmarks;

    let itemInfo = null;
    try {
      itemInfo = yield PlacesUtils.promiseBookmarksTree(aGUID);
    }
    catch(ex) {
      throw new Error("Failed to get info for the specified item (guid: " +
                      aGUID + "). Ex: " + ex);
    }
    PlacesUtils.bookmarks.removeItem(yield PlacesUtils.promiseItemId(aGUID));
    this.undo = createItemsFromBookmarksTree.bind(null, itemInfo, true);
  }
};






PT.TagURI = DefineTransaction(["uri", "tags"]);
PT.TagURI.prototype = {
  execute: function* (aURI, aTags) {
    if (PlacesUtils.getMostRecentBookmarkForURI(aURI) == -1) {
      
      let unfileGUID =
        yield PlacesUtils.promiseItemGUID(PlacesUtils.unfiledBookmarksFolderId);
      let createTxn = TransactionsHistory.getRawTransaction(
        PT.NewBookmark({ uri: aURI, tags: aTags, parentGUID: unfileGUID }));
      yield createTxn.execute();
      this.undo = createTxn.undo.bind(createTxn);
      this.redo = createTxn.redo.bind(createTxn);
    }
    else {
      let currentTags = PlacesUtils.tagging.getTagsForURI(aURI);
      let newTags = [t for (t of aTags) if (currentTags.indexOf(t) == -1)];
      PlacesUtils.tagging.tagURI(aURI, newTags);
      this.undo = () => { PlacesUtils.tagging.untagURI(aURI, newTags); };
      this.redo = () => { PlacesUtils.tagging.tagURI(aURI, newTags); };
    }
  }
};









PT.UntagURI = DefineTransaction(["uri"], ["tags"]);
PT.UntagURI.prototype = {
  execute: function* (aURI, aTags) {
    let tagsSet = PlacesUtils.tagging.getTagsForURI(aURI);

    if (aTags && aTags.length > 0)
      aTags = [t for (t of aTags) if (tagsSet.indexOf(t) != -1)];
    else
      aTags = tagsSet;

    PlacesUtils.tagging.untagURI(aURI, aTags);
    this.undo = () => { PlacesUtils.tagging.tagURI(aURI, aTags); };
    this.redo = () => { PlacesUtils.tagging.untagURI(aURI, aTags); };
  }
};







PT.Copy = DefineTransaction(["GUID", "newParentGUID"],
                            ["newIndex"]);
PT.Copy.prototype = {
  execute: function* (aGUID, aNewParentGUID, aNewIndex) {
    let creationInfo = null;
    try {
      creationInfo = yield PlacesUtils.promiseBookmarksTree(aGUID);
    }
    catch(ex) {
      throw new Error("Failed to get info for the specified item (guid: " +
                      aGUID + "). Ex: " + ex);
    }
    creationInfo.parentGUID = aNewParentGUID;
    creationInfo.index = aNewIndex;

    let newItemId = yield createItemsFromBookmarksTree(creationInfo, false);
    let newItemInfo = null;
    this.undo = function* () {
      if (!newItemInfo) {
        let newItemGUID = yield PlacesUtils.promiseItemGUID(newItemId);
        newItemInfo = yield PlacesUtils.promiseBookmarksTree(newItemGUID);
      }
      PlacesUtils.bookmarks.removeItem(newItemId);
    };
    this.redo = function* () {
      newItemId = yield createItemsFromBookmarksTree(newItemInfo, true);
    }

    return yield PlacesUtils.promiseItemGUID(newItemId);
  }
};
