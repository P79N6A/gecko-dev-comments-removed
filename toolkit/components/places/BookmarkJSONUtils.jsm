



this.EXPORTED_SYMBOLS = [ "BookmarkJSONUtils" ];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/PromiseUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesBackups",
  "resource://gre/modules/PlacesBackups.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
  "resource://gre/modules/Deprecated.jsm");

XPCOMUtils.defineLazyGetter(this, "gTextDecoder", () => new TextDecoder());
XPCOMUtils.defineLazyGetter(this, "gTextEncoder", () => new TextEncoder());







function generateHash(aString) {
  let cryptoHash = Cc["@mozilla.org/security/hash;1"]
                     .createInstance(Ci.nsICryptoHash);
  cryptoHash.init(Ci.nsICryptoHash.MD5);
  let stringStream = Cc["@mozilla.org/io/string-input-stream;1"]
                       .createInstance(Ci.nsIStringInputStream);
  stringStream.data = aString;
  cryptoHash.updateFromStream(stringStream, -1);
  
  return cryptoHash.finish(true).replace(/\//g, "-");
}

this.BookmarkJSONUtils = Object.freeze({
  











  importFromURL: function BJU_importFromURL(aSpec, aReplace) {
    return Task.spawn(function* () {
      notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_BEGIN);
      try {
        let importer = new BookmarkImporter(aReplace);
        yield importer.importFromURL(aSpec);

        notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_SUCCESS);
      } catch(ex) {
        Cu.reportError("Failed to restore bookmarks from " + aSpec + ": " + ex);
        notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_FAILED);
      }
    });
  },

  














  importFromFile: function BJU_importFromFile(aFilePath, aReplace) {
    if (aFilePath instanceof Ci.nsIFile) {
      Deprecated.warning("Passing an nsIFile to BookmarksJSONUtils.importFromFile " +
                         "is deprecated. Please use an OS.File path string instead.",
                         "https://developer.mozilla.org/docs/JavaScript_OS.File");
      aFilePath = aFilePath.path;
    }

    return Task.spawn(function* () {
      notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_BEGIN);
      try {
        if (!(yield OS.File.exists(aFilePath)))
          throw new Error("Cannot restore from nonexisting json file");

        let importer = new BookmarkImporter(aReplace);
        if (aFilePath.endsWith("jsonlz4")) {
          yield importer.importFromCompressedFile(aFilePath);
        } else {
          yield importer.importFromURL(OS.Path.toFileURI(aFilePath));
        }
        notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_SUCCESS);
      } catch(ex) {
        Cu.reportError("Failed to restore bookmarks from " + aFilePath + ": " + ex);
        notifyObservers(PlacesUtils.TOPIC_BOOKMARKS_RESTORE_FAILED);
        throw ex;
      }
    });
  },

  

















  exportToFile: function BJU_exportToFile(aFilePath, aOptions={}) {
    if (aFilePath instanceof Ci.nsIFile) {
      Deprecated.warning("Passing an nsIFile to BookmarksJSONUtils.exportToFile " +
                         "is deprecated. Please use an OS.File path string instead.",
                         "https://developer.mozilla.org/docs/JavaScript_OS.File");
      aFilePath = aFilePath.path;
    }
    return Task.spawn(function* () {
      let [bookmarks, count] = yield PlacesBackups.getBookmarksTree();
      let startTime = Date.now();
      let jsonString = JSON.stringify(bookmarks);
      
      try {
        Services.telemetry
                .getHistogramById("PLACES_BACKUPS_TOJSON_MS")
                .add(Date.now() - startTime);
      } catch (ex) {
        Components.utils.reportError("Unable to report telemetry.");
      }

      let hash = generateHash(jsonString);

      if (hash === aOptions.failIfHashIs) {
        let e = new Error("Hash conflict");
        e.becauseSameHash = true;
        throw e;
      }

      
      
      
      let writeOptions = { tmpPath: OS.Path.join(aFilePath + ".tmp") };
      if (aOptions.compress)
        writeOptions.compression = "lz4";

      yield OS.File.writeAtomic(aFilePath, jsonString, writeOptions);
      return { count: count, hash: hash };
    });
  }
});

function BookmarkImporter(aReplace) {
  this._replace = aReplace;
}
BookmarkImporter.prototype = {
  









  importFromURL(spec) {
    return new Promise((resolve, reject) => {
      let streamObserver = {
        onStreamComplete: (aLoader, aContext, aStatus, aLength, aResult) => {
          let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                          createInstance(Ci.nsIScriptableUnicodeConverter);
          converter.charset = "UTF-8";
          try {
            let jsonString = converter.convertFromByteArray(aResult,
                                                            aResult.length);
            resolve(this.importFromJSON(jsonString));
          } catch (ex) {
            Cu.reportError("Failed to import from URL: " + ex);
            reject(ex);
          }
        }
      };

      let uri = NetUtil.newURI(spec);
      let channel = NetUtil.newChannel({
        uri,
        loadingPrincipal: Services.scriptSecurityManager.getNoAppCodebasePrincipal(uri),
        contentPolicyType: Ci.nsIContentPolicy.TYPE_DATAREQUEST
      });
      let streamLoader = Cc["@mozilla.org/network/stream-loader;1"]
                           .createInstance(Ci.nsIStreamLoader);
      streamLoader.init(streamObserver);
      channel.asyncOpen(streamLoader, channel);
    });
  },

  









  importFromCompressedFile: function* BI_importFromCompressedFile(aFilePath) {
      let aResult = yield OS.File.read(aFilePath, { compression: "lz4" });
      let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                        createInstance(Ci.nsIScriptableUnicodeConverter);
      converter.charset = "UTF-8";
      let jsonString = converter.convertFromByteArray(aResult, aResult.length);
      yield this.importFromJSON(jsonString);
  },

  





  importFromJSON: Task.async(function* (aString) {
    this._importPromises = [];
    let deferred = PromiseUtils.defer();
    let nodes =
      PlacesUtils.unwrapNodes(aString, PlacesUtils.TYPE_X_MOZ_PLACE_CONTAINER);

    if (nodes.length == 0 || !nodes[0].children ||
        nodes[0].children.length == 0) {
      deferred.resolve(); 
    } else {
      
      nodes[0].children.sort(function sortRoots(aNode, bNode) {
        return (aNode.root && aNode.root == "tagsFolder") ? 1 :
               (bNode.root && bNode.root == "tagsFolder") ? -1 : 0;
      });

      let batch = {
        nodes: nodes[0].children,
        runBatched: function runBatched() {
          if (this._replace) {
            
            
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

          for (let node of batch.nodes) {
            if (!node.children || node.children.length == 0)
              continue; 

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

              
              for (let child of node.children) {
                let index = child.index;
                let [folders, searches] =
                  this.importJSONNode(child, container, index, 0);
                for (let i = 0; i < folders.length; i++) {
                  if (folders[i])
                    folderIdMap[i] = folders[i];
                }
                searchIds = searchIds.concat(searches);
              }
            } else {
              this.importJSONNode(
                node, PlacesUtils.placesRootId, node.index, 0);
            }
          }

          
          searchIds.forEach(function(aId) {
            let oldURI = PlacesUtils.bookmarks.getBookmarkURI(aId);
            let uri = fixupQuery(oldURI, folderIdMap);
            if (!uri.equals(oldURI)) {
              PlacesUtils.bookmarks.changeBookmarkURI(aId, uri);
            }
          });

          deferred.resolve();
        }.bind(this)
      };

      PlacesUtils.bookmarks.runInBatchMode(batch, null);
    }
    yield deferred.promise;
    
    
    try {
      yield Promise.all(this._importPromises);
    } finally {
      delete this._importPromises;
    }
  }),

  












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
        } else if (aData.annos &&
                   aData.annos.some(anno => anno.name == PlacesUtils.LMANNO_FEEDURI)) {
          
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
            }).then(function (aLivemark) {
              let id = aLivemark.id;
              if (aData.dateAdded)
                PlacesUtils.bookmarks.setItemDateAdded(id, aData.dateAdded);
              if (aData.annos && aData.annos.length)
                PlacesUtils.setAnnotationsForItem(id, aData.annos);
            }, Cu.reportError);
          }
        } else {
          id = PlacesUtils.bookmarks.createFolder(
                 aContainer, aData.title, aIndex);
          folderIdMap[aData.id] = id;
          
          if (aData.children) {
            for (let i = 0; i < aData.children.length; i++) {
              let child = aData.children[i];
              let [folders, searches] =
                this.importJSONNode(child, id, i, aContainer);
              for (let j = 0; j < folders.length; j++) {
                if (folders[j])
                  folderIdMap[j] = folders[j];
              }
              searchIds = searchIds.concat(searches);
            }
          }
        }
        break;
      case PlacesUtils.TYPE_X_MOZ_PLACE:
        id = PlacesUtils.bookmarks.insertBookmark(
               aContainer, NetUtil.newURI(aData.uri), aIndex, aData.title);
        if (aData.keyword) {
          
          
          
          let postDataAnno = aData.annos &&
                             aData.annos.find(anno => anno.name == PlacesUtils.POST_DATA_ANNO);
          let postData = aData.postData || (postDataAnno && postDataAnno.value);
          let kwPromise = PlacesUtils.keywords.insert({ keyword: aData.keyword,
                                                        url: aData.uri,
                                                        postData });
          this._importPromises.push(kwPromise);
        }
        if (aData.tags) {
          
          let tags = aData.tags.split(",").map(tag => tag.trim());
          if (tags.length)
            PlacesUtils.tagging.tagURI(NetUtil.newURI(aData.uri), tags);
        }
        if (aData.charset) {
          PlacesUtils.annotations.setPageAnnotation(
            NetUtil.newURI(aData.uri), PlacesUtils.CHARSET_ANNO, aData.charset,
            0, Ci.nsIAnnotationService.EXPIRE_NEVER);
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
