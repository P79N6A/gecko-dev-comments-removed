



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
    if (Array.isArray(aToTransact)) {
      if (aToTransact.some(
           o => !TransactionsHistory.isProxifiedTransactionObject(o))) {
        throw new Error("aToTransact contains non-transaction element");
      }
      
      return this.transact(function* () {
        for (let t of aToTransact) {
          yield t;
        }
      });
    }

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

DefineTransaction.uriValidate = function(uriOrSpec) {
  if (uriOrSpec instanceof Components.interfaces.nsIURI)
    return uriOrSpec;
  return NetUtil.newURI(uriOrSpec);
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



DefineTransaction.defineInputProps(["uri", "feedURI", "siteURI"],
                                   DefineTransaction.uriValidate, null);
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
DefineTransaction.defineArrayInputProp("uris", "uri");
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









PT.NewBookmark = DefineTransaction(["parentGuid", "uri"],
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











PT.NewLivemark = DefineTransaction(["feedURI", "title", "parentGuid"],
                                   ["siteURI", "index", "annotations"]);
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






PT.EditURI = DefineTransaction(["guid", "uri"]);
PT.EditURI.prototype = Object.seal({
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






PT.Annotate = DefineTransaction(["guid", "annotations"]);
PT.Annotate.prototype = {
  execute: function* (aGuid, aNewAnnos) {
    let itemId = yield PlacesUtils.promiseItemId(aGuid);
    let currentAnnos = PlacesUtils.getAnnotationsForItem(itemId);
    let undoAnnos = [];
    for (let newAnno of aNewAnnos) {
      let currentAnno = currentAnnos.find( a => a.name == newAnno.name );
      if (!!currentAnno) {
        undoAnnos.push(currentAnno);
      }
      else {
        
        undoAnnos.push({ name: newAnno.name });
      }
    }

    PlacesUtils.setAnnotationsForItem(itemId, aNewAnnos);
    this.undo = () => {
      PlacesUtils.setAnnotationsForItem(itemId, undoAnnos);
    };
    this.redo = () => {
      PlacesUtils.setAnnotationsForItem(itemId, aNewAnnos);
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






PT.Remove = DefineTransaction(["guid"]);
PT.Remove.prototype = {
  execute: function* (aGuid) {
    const bms = PlacesUtils.bookmarks;

    let itemInfo = null;
    try {
      itemInfo = yield PlacesUtils.promiseBookmarksTree(aGuid);
    }
    catch(ex) {
      throw new Error("Failed to get info for the specified item (guid: " +
                      aGuid + "). Ex: " + ex);
    }
    PlacesUtils.bookmarks.removeItem(yield PlacesUtils.promiseItemId(aGuid));
    this.undo = createItemsFromBookmarksTree.bind(null, itemInfo, true);
  }
};






PT.Tag = DefineTransaction(["uris", "tags"]);
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
        
        let unfiledGuid =
          yield PlacesUtils.promiseItemGuid(PlacesUtils.unfiledBookmarksFolderId);
        let createTxn = TransactionsHistory.getRawTransaction(
          PT.NewBookmark({ uri: currentURI
                         , tags: aTags, parentGuid: unfiledGuid }));
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









PT.Untag = DefineTransaction(["uris"], ["tags"]);
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
