



this.EXPORTED_SYMBOLS = [ "BookmarkJSONUtils" ];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");

this.BookmarkJSONUtils = Object.freeze({
  











  importFromURL: function BJU_importFromURL(aURL, aReplace) {
    let importer = new BookmarkImporter();
    return importer.importFromURL(aURL, aReplace);
  },

  













  importFromFile: function BJU_importFromFile(aFile, aReplace) {
    let importer = new BookmarkImporter();
    return importer.importFromURL(NetUtil.newURI(aFile).spec, aReplace);
  },

  









  exportToFile: function BJU_exportToFile(aLocalFile) {
    let exporter = new BookmarkExporter();
    return exporter.exportToFile(aLocalFile);
  }
});

function BookmarkImporter() {}
BookmarkImporter.prototype = {
  











  importFromURL: function BI_importFromURL(aURL, aReplace) {
    let deferred = Promise.defer();
    notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_BEGIN);

    let streamObserver = {
      onStreamComplete: function (aLoader, aContext, aStatus, aLength,
                                  aResult) {
        let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                        createInstance(Ci.nsIScriptableUnicodeConverter);
        converter.charset = "UTF-8";

        Task.spawn(function() {
          try {
            let jsonString =
              converter.convertFromByteArray(aResult, aResult.length);
            yield this.importFromJSON(jsonString, aReplace);
            notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_SUCCESS);
            deferred.resolve();
          } catch (ex) {
            notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_FAILED);
            deferred.reject(ex);
          }
        }.bind(this));
      }.bind(this)
    };

    try {
      let channel = Services.io.newChannelFromURI(NetUtil.newURI(aURL));
      let streamLoader = Cc["@mozilla.org/network/stream-loader;1"].
                         createInstance(Ci.nsIStreamLoader);

      streamLoader.init(streamObserver);
      channel.asyncOpen(streamLoader, channel);
    } catch (ex) {
      notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_FAILED);
      deferred.reject(ex);
    }

    return deferred.promise;
  },

  







  importFromJSON: function BI_importFromJSON(aString, aReplace) {
    let deferred = Promise.defer();
    let nodes =
      PlacesUtils.unwrapNodes(aString, PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER);

    if (nodes.length == 0 || !nodes[0].children ||
        nodes[0].children.length == 0) {
      Services.tm.mainThread.dispatch(function() {
        deferred.resolve(); 
      }, Ci.nsIThread.DISPATCH_NORMAL);
    }

    
    nodes[0].children.sort(function sortRoots(aNode, bNode) {
      return (aNode.root && aNode.root == "tagsFolder") ? 1 :
             (bNode.root && bNode.root == "tagsFolder") ? -1 : 0;
    });

    let batch = {
      nodes: nodes[0].children,
      runBatched: function runBatched() {
        if (aReplace) {
          
          
          let excludeItems = PlacesUtils.annotations.getItemsWithAnnotation(
                               PlacesUtils.EXCLUDE_FROM_BACKUP_ANNO);
          
          
          
          let root = PlacesUtils.getFolderContents(PlacesUtils.placesRootId,
                                                   false, false).root;
          let childIds = [];
          for (let i = 0; i < root.childCount; i++) {
            let childId = root.getChild(i).itemId;
            if (excludeItems.indexOf(childId) == -1 &&
                childId != PlacesUtils.tagsFolderId) {
              childIds.push(childId);
            }
          }
          root.containerOpen = false;

          for (let i = 0; i < childIds.length; i++) {
            let rootItemId = childIds[i];
            if (PlacesUtils.isRootItem(rootItemId)) {
              PlacesUtils.bookmarks.removeFolderChildren(rootItemId);
            } else {
              PlacesUtils.bookmarks.removeItem(rootItemId);
            }
          }
        }

        let searchIds = [];
        let folderIdMap = [];

        batch.nodes.forEach(function(node) {
          if (!node.children || node.children.length == 0)
            return; 

          if (node.root) {
            let container = PlacesUtils.placesRootId; 
            switch (node.root) {
              case "bookmarksMenuFolder":
                container = PlacesUtils.bookmarksMenuFolderId;
                break;
              case "tagsFolder":
                container = PlacesUtils.tagsFolderId;
                break;
              case "unfiledBookmarksFolder":
                container = PlacesUtils.unfiledBookmarksFolderId;
                break;
              case "toolbarFolder":
                container = PlacesUtils.toolbarFolderId;
                break;
            }

            
            node.children.forEach(function(child) {
              let index = child.index;
              let [folders, searches] =
                this.importJSONNode(child, container, index, 0);
              for (let i = 0; i < folders.length; i++) {
                if (folders[i])
                  folderIdMap[i] = folders[i];
              }
              searchIds = searchIds.concat(searches);
            }.bind(this));
          } else {
            this.importJSONNode(
              node, PlacesUtils.placesRootId, node.index, 0);
          }
        }.bind(this));

        
        searchIds.forEach(function(aId) {
          let oldURI = PlacesUtils.bookmarks.getBookmarkURI(aId);
          let uri = fixupQuery(oldURI, folderIdMap);
          if (!uri.equals(oldURI)) {
            PlacesUtils.bookmarks.changeBookmarkURI(aId, uri);
          }
        });

        Services.tm.mainThread.dispatch(function() {
          deferred.resolve();
        }, Ci.nsIThread.DISPATCH_NORMAL);
      }.bind(this)
    };

    PlacesUtils.bookmarks.runInBatchMode(batch, null);

    return deferred.promise;
  },

  












  importJSONNode: function BI_importJSONNode(aData, aContainer, aIndex,
                                             aGrandParentId) {
    let folderIdMap = [];
    let searchIds = [];
    let id = -1;
    switch (aData.type) {
      case PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER:
        if (aContainer == PlacesUtils.tagsFolderId) {
          
          if (aData.children) {
            aData.children.forEach(function(aChild) {
              try {
                PlacesUtils.tagging.tagURI(
                  NetUtil.newURI(aChild.uri), [aData.title]);
              } catch (ex) {
                
              }
            });
            return [folderIdMap, searchIds];
          }
        } else if (aData.livemark && aData.annos) {
          
          let feedURI = null;
          let siteURI = null;
          aData.annos = aData.annos.filter(function(aAnno) {
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
          });

          if (feedURI) {
            PlacesUtils.livemarks.addLivemark({
              title: aData.title,
              feedURI: feedURI,
              parentId: aContainer,
              index: aIndex,
              lastModified: aData.lastModified,
              siteURI: siteURI
            }, function(aStatus, aLivemark) {
              if (Components.isSuccessCode(aStatus)) {
                let id = aLivemark.id;
                if (aData.dateAdded)
                  PlacesUtils.bookmarks.setItemDateAdded(id, aData.dateAdded);
                if (aData.annos && aData.annos.length)
                  PlacesUtils.setAnnotationsForItem(id, aData.annos);
              }
            });
          }
        } else {
          id =
            PlacesUtils.bookmarks.createFolder(aContainer, aData.title, aIndex);
          folderIdMap[aData.id] = id;
          
          if (aData.children) {
            aData.children.forEach(function(aChild, aIndex) {
              let [folders, searches] =
                this.importJSONNode(aChild, id, aIndex, aContainer);
              for (let i = 0; i < folders.length; i++) {
                if (folders[i])
                  folderIdMap[i] = folders[i];
              }
              searchIds = searchIds.concat(searches);
            }.bind(this));
          }
        }
        break;
      case PlacesUtils.TYPE_X_MOZ_PLACE:
        id = PlacesUtils.bookmarks.insertBookmark(
               aContainer, NetUtil.newURI(aData.uri), aIndex, aData.title);
        if (aData.keyword)
          PlacesUtils.bookmarks.setKeywordForBookmark(id, aData.keyword);
        if (aData.tags) {
          let tags = aData.tags.split(", ");
          if (tags.length)
            PlacesUtils.tagging.tagURI(NetUtil.newURI(aData.uri), tags);
        }
        if (aData.charset) {
          PlacesUtils.history.setCharsetForURI(
            NetUtil.newURI(aData.uri), aData.charset);
        }
        if (aData.uri.substr(0, 6) == "place:")
          searchIds.push(id);
        if (aData.icon) {
          try {
            
            let faviconURI = NetUtil.newURI("fake-favicon-uri:" + aData.uri);
            PlacesUtils.favicons.replaceFaviconDataFromDataURL(
              faviconURI, aData.icon, 0);
            PlacesUtils.favicons.setAndFetchFaviconForPage(
              NetUtil.newURI(aData.uri), faviconURI, false,
              PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
          } catch (ex) {
            Components.utils.reportError("Failed to import favicon data:" + ex);
          }
        }
        if (aData.iconUri) {
          try {
            PlacesUtils.favicons.setAndFetchFaviconForPage(
              NetUtil.newURI(aData.uri), NetUtil.newURI(aData.iconUri), false,
              PlacesUtils.favicons.FAVICON_LOAD_NON_PRIVATE);
          } catch (ex) {
            Components.utils.reportError("Failed to import favicon URI:" + ex);
          }
        }
        break;
      case PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR:
        id = PlacesUtils.bookmarks.insertSeparator(aContainer, aIndex);
        break;
      default:
        
    }

    
    if (id != -1 && aContainer != PlacesUtils.tagsFolderId &&
        aGrandParentId != PlacesUtils.tagsFolderId) {
      if (aData.dateAdded)
        PlacesUtils.bookmarks.setItemDateAdded(id, aData.dateAdded);
      if (aData.lastModified)
        PlacesUtils.bookmarks.setItemLastModified(id, aData.lastModified);
      if (aData.annos && aData.annos.length)
        PlacesUtils.setAnnotationsForItem(id, aData.annos);
    }

    return [folderIdMap, searchIds];
  }
}

function notifyObservers(topic) {
  Services.obs.notifyObservers(null, topic, "json");
}












function fixupQuery(aQueryURI, aFolderIdMap) {
  let convert = function(str, p1, offset, s) {
    return "folder=" + aFolderIdMap[p1];
  }
  let stringURI = aQueryURI.spec.replace(/folder=([0-9]+)/g, convert);

  return NetUtil.newURI(stringURI);
}

function BookmarkExporter() {}
BookmarkExporter.prototype = {
  exportToFile: function BE_exportToFile(aLocalFile) {
    return Task.spawn(this._writeToFile(aLocalFile));
  },

  _converterOut: null,

  _writeToFile: function BE__writeToFile(aLocalFile) {
    
    let safeFileOut = Cc["@mozilla.org/network/safe-file-output-stream;1"].
                      createInstance(Ci.nsIFileOutputStream);
    safeFileOut.init(aLocalFile, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE |
                     FileUtils.MODE_TRUNCATE, parseInt("0600", 8), 0);

    try {
      
      let bufferedOut = Cc["@mozilla.org/network/buffered-output-stream;1"].
                        createInstance(Ci.nsIBufferedOutputStream);
      bufferedOut.init(safeFileOut, 4096);
      try {
        
        this._converterOut = Cc["@mozilla.org/intl/converter-output-stream;1"].
                             createInstance(Ci.nsIConverterOutputStream);
        this._converterOut.init(bufferedOut, "utf-8", 0, 0);
        try {
          yield this._writeContentToFile();

          
          bufferedOut.QueryInterface(Ci.nsISafeOutputStream).finish();
        } finally {
          this._converterOut.close();
          this._converterOut = null;
        }
      } finally {
        bufferedOut.close();
      }
    } finally {
      safeFileOut.close();
    }
  },

  _writeContentToFile: function BE__writeContentToFile() {
    
    let streamProxy = {
      converter: this._converterOut,
      write: function(aData, aLen) {
        this.converter.writeString(aData);
      }
    };

    
    let excludeItems = PlacesUtils.annotations.getItemsWithAnnotation(
                         PlacesUtils.EXCLUDE_FROM_BACKUP_ANNO);
    let root = PlacesUtils.getFolderContents(PlacesUtils.placesRootId, false,
                                             false).root;

    
    BookmarkNode.serializeAsJSONToOutputStream(root, streamProxy, false, false,
                                               excludeItems);
    root.containerOpen = false;
  }
}

let BookmarkNode = {
  

















  serializeAsJSONToOutputStream: function BN_serializeAsJSONToOutputStream(
    aNode, aStream, aIsUICommand, aResolveShortcuts, aExcludeItems) {

    
    let array = [];
    if (this._appendConvertedNode(aNode, null, array, aIsUICommand,
                                  aResolveShortcuts, aExcludeItems)) {
      let json = JSON.stringify(array[0]);
      aStream.write(json, json.length);
    } else {
      throw Cr.NS_ERROR_UNEXPECTED;
    }
  },

  _appendConvertedNode: function BN__appendConvertedNode(
    bNode, aIndex, aArray, aIsUICommand, aResolveShortcuts, aExcludeItems) {
    let node = {};

    
    
    
    if (aIndex)
      node.index = aIndex;

    this._addGenericProperties(bNode, node, aResolveShortcuts);

    let parent = bNode.parent;
    let grandParent = parent ? parent.parent : null;

    if (PlacesUtils.nodeIsURI(bNode)) {
      
      if (parent && parent.itemId == PlacesUtils.tagsFolderId)
        return false;

      
      
      
      try {
        NetUtil.newURI(bNode.uri);
      } catch (ex) {
        return false;
      }

      this._addURIProperties(bNode, node, aIsUICommand);
    } else if (PlacesUtils.nodeIsContainer(bNode)) {
      
      if (grandParent && grandParent.itemId == PlacesUtils.tagsFolderId)
        return false;

      this._addContainerProperties(bNode, node, aIsUICommand,
                                   aResolveShortcuts);
    } else if (PlacesUtils.nodeIsSeparator(bNode)) {
      
      
      if ((parent && parent.itemId == PlacesUtils.tagsFolderId) ||
          (grandParent && grandParent.itemId == PlacesUtils.tagsFolderId))
        return false;

      this._addSeparatorProperties(bNode, node);
    }

    if (!node.feedURI && node.type == PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER) {
      return this._appendConvertedComplexNode(node, bNode, aArray, aIsUICommand,
                                              aResolveShortcuts, aExcludeItems);
    }

    aArray.push(node);
    return true;
  },

  _addGenericProperties: function BN__addGenericProperties(
    aPlacesNode, aJSNode, aResolveShortcuts) {
    aJSNode.title = aPlacesNode.title;
    aJSNode.id = aPlacesNode.itemId;
    if (aJSNode.id != -1) {
      let parent = aPlacesNode.parent;
      if (parent)
        aJSNode.parent = parent.itemId;
      let dateAdded = aPlacesNode.dateAdded;
      if (dateAdded)
        aJSNode.dateAdded = dateAdded;
      let lastModified = aPlacesNode.lastModified;
      if (lastModified)
        aJSNode.lastModified = lastModified;

      
      let annos = [];
      try {
        annos =
          PlacesUtils.getAnnotationsForItem(aJSNode.id).filter(function(anno) {
          
          
          
          
          if (anno.name == PlacesUtils.LMANNO_FEEDURI)
            aJSNode.livemark = 1;
          if (anno.name == PlacesUtils.READ_ONLY_ANNO && aResolveShortcuts) {
            
            return false;
          }
          return true;
        });
      } catch(ex) {}
      if (annos.length != 0)
        aJSNode.annos = annos;
    }
    
  },

  _addURIProperties: function BN__addURIProperties(
    aPlacesNode, aJSNode, aIsUICommand) {
    aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE;
    aJSNode.uri = aPlacesNode.uri;
    if (aJSNode.id && aJSNode.id != -1) {
      
      let keyword = PlacesUtils.bookmarks.getKeywordForBookmark(aJSNode.id);
      if (keyword)
        aJSNode.keyword = keyword;
    }

    let tags = aIsUICommand ? aPlacesNode.tags : null;
    if (tags)
      aJSNode.tags = tags;

    
    let uri = NetUtil.newURI(aPlacesNode.uri);
    let lastCharset = PlacesUtils.history.getCharsetForURI(uri);
    if (lastCharset)
      aJSNode.charset = lastCharset;
  },

  _addSeparatorProperties: function BN__addSeparatorProperties(
    aPlacesNode, aJSNode) {
    aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE_SEPARATOR;
  },

  _addContainerProperties: function BN__addContainerProperties(
    aPlacesNode, aJSNode, aIsUICommand, aResolveShortcuts) {
    let concreteId = PlacesUtils.getConcreteItemId(aPlacesNode);
    if (concreteId != -1) {
      
      if (PlacesUtils.nodeIsQuery(aPlacesNode) ||
          (concreteId != aPlacesNode.itemId && !aResolveShortcuts)) {
        aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE;
        aJSNode.uri = aPlacesNode.uri;
        
        if (aIsUICommand)
          aJSNode.concreteId = concreteId;
      } else {
        
        aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER;

        
        if (aJSNode.id == PlacesUtils.placesRootId)
          aJSNode.root = "placesRoot";
        else if (aJSNode.id == PlacesUtils.bookmarksMenuFolderId)
          aJSNode.root = "bookmarksMenuFolder";
        else if (aJSNode.id == PlacesUtils.tagsFolderId)
          aJSNode.root = "tagsFolder";
        else if (aJSNode.id == PlacesUtils.unfiledBookmarksFolderId)
          aJSNode.root = "unfiledBookmarksFolder";
        else if (aJSNode.id == PlacesUtils.toolbarFolderId)
          aJSNode.root = "toolbarFolder";
      }
    } else {
      
      aJSNode.type = PlacesUtils.TYPE_X_MOZ_PLACE;
      aJSNode.uri = aPlacesNode.uri;
    }
  },

  _appendConvertedComplexNode: function BN__appendConvertedComplexNode(
    aNode, aSourceNode, aArray, aIsUICommand, aResolveShortcuts,
    aExcludeItems) {
    let repr = {};

    for (let [name, value] in Iterator(aNode))
      repr[name] = value;

    
    let children = repr.children = [];
    if (!aNode.livemark) {
      PlacesUtils.asContainer(aSourceNode);
      let wasOpen = aSourceNode.containerOpen;
      if (!wasOpen)
        aSourceNode.containerOpen = true;
      let cc = aSourceNode.childCount;
      for (let i = 0; i < cc; ++i) {
        let childNode = aSourceNode.getChild(i);
        if (aExcludeItems && aExcludeItems.indexOf(childNode.itemId) != -1)
          continue;
        this._appendConvertedNode(aSourceNode.getChild(i), i, children,
                                  aIsUICommand, aResolveShortcuts,
                                  aExcludeItems);
      }
      if (!wasOpen)
        aSourceNode.containerOpen = false;
    }

    aArray.push(repr);
    return true;
  }
}
