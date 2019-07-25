





































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


let (commonFile = do_get_file("../head_common.js", false)) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}





const DAY_MICROSEC = 86400000000;
const today = Date.now() * 1000;
const yesterday = today - DAY_MICROSEC;
const lastweek = today - (DAY_MICROSEC * 7);
const daybefore = today - (DAY_MICROSEC * 2);
const tomorrow = today + DAY_MICROSEC;
const old = today - (DAY_MICROSEC * 3);
const futureday = today + (DAY_MICROSEC * 3);
const olderthansixmonths = today - (DAY_MICROSEC * 31 * 7);







function populateDB(aArray) {
  PlacesUtils.history.runInBatchMode({
    runBatched: function (aUserData)
    {
      aArray.forEach(function (data)
      {
        try {
          
          
          var qdata = new queryData(data);
          if (qdata.isVisit) {
            
            var referrer = qdata.referrer ? uri(qdata.referrer) : null;
            var visitId = PlacesUtils.history.addVisit(uri(qdata.uri), qdata.lastVisit,
                                                       referrer, qdata.transType,
                                                       qdata.isRedirect, qdata.sessionID);
            if (qdata.title && !qdata.isDetails) {
              
              let stmt = DBConn().createStatement(
                "UPDATE moz_places SET title = :title WHERE url = :url"
              );
              stmt.params.title = qdata.title;
              stmt.params.url = qdata.uri;
              try {
                stmt.execute();
              }
              catch (ex) {
                print("Error while setting title.");
              }
              finally {
                stmt.finalize();
              }
            }
            if (qdata.visitCount && !qdata.isDetails) {
              
              
              let stmt = DBConn().createStatement(
                "UPDATE moz_places SET visit_count = :vc WHERE url = :url");
              stmt.params.vc = qdata.visitCount;
              stmt.params.url = qdata.uri;
              try {
                stmt.execute();
              }
              catch (ex) {
                print("Error while setting visit_count.");
              }
              finally {
                stmt.finalize();
              }
            }
          }

          if (qdata.isDetails) {
            
            PlacesUtils.history.addPageWithDetails(uri(qdata.uri),
                                                   qdata.title, qdata.lastVisit);
          }

          if (qdata.markPageAsTyped){
            PlacesUtils.bhistory.markPageAsTyped(uri(qdata.uri));
          }

          if (qdata.hidePage){
            PlacesUtils.bhistory.hidePage(uri(qdata.uri));
          }

          if (qdata.isPageAnnotation) {
            if (qdata.removeAnnotation) 
              PlacesUtils.annotations.removePageAnnotation(uri(qdata.uri),
                                                           qdata.annoName);
            else {
              PlacesUtils.annotations.setPageAnnotation(uri(qdata.uri),
                                                        qdata.annoName,
                                                        qdata.annoVal,
                                                        qdata.annoFlags,
                                                        qdata.annoExpiration);
            }
          }

          if (qdata.isItemAnnotation) {
            if (qdata.removeAnnotation)
              PlacesUtils.annotations.removeItemAnnotation(qdata.itemId,
                                                           qdata.annoName);
            else {
              PlacesUtils.annotations.setItemAnnotation(qdata.itemId,
                                                        qdata.annoName,
                                                        qdata.annoVal,
                                                        qdata.annoFlags,
                                                        qdata.annoExpiration);
            }
          }

          if (qdata.isPageBinaryAnnotation) {
            if (qdata.removeAnnotation)
              PlacesUtils.annotations.removePageAnnotation(uri(qdata.uri),
                                                           qdata.annoName);
            else {
              PlacesUtils.annotations.setPageAnnotationBinary(uri(qdata.uri),
                                                              qdata.annoName,
                                                              qdata.binarydata,
                                                              qdata.binaryDataLength,
                                                              qdata.annoMimeType,
                                                              qdata.annoFlags,
                                                              qdata.annoExpiration);
            }
          }

          if (qdata.isItemBinaryAnnotation) {
            if (qdata.removeAnnotation)
              PlacesUtils.annotations.removeItemAnnotation(qdata.itemId,
                                                           qdata.annoName);
            else {
              PlacesUtils.annotations.setItemAnnotationBinary(qdata.itemId,
                                                              qdata.annoName,
                                                              qdata.binaryData,
                                                              qdata.binaryDataLength,
                                                              qdata.annoMimeType,
                                                              qdata.annoFlags,
                                                              qdata.annoExpiration);
            }
          }

          if (qdata.isFavicon) {
            
            
            try {
              PlacesUtils.favicons.setFaviconData(uri(qdata.faviconURI),
                                                  qdata.favicon,
                                                  qdata.faviconLen,
                                                  qdata.faviconMimeType,
                                                  qdata.faviconExpiration);
            } catch (ex) {}
            PlacesUtils.favicons.setFaviconUrlForPage(uri(qdata.uri),
                                                      uri(qdata.faviconURI));
          }

          if (qdata.isFolder) {
            let folderId = PlacesUtils.bookmarks.createFolder(qdata.parentFolder,
                                                              qdata.title,
                                                              qdata.index);
            if (qdata.readOnly)
              PlacesUtils.bookmarks.setFolderReadonly(folderId, true);
          }

          if (qdata.isLivemark) {
            PlacesUtils.livemarks.createLivemark(qdata.parentFolder,
                                                 qdata.title,
                                                 uri(qdata.uri),
                                                 uri(qdata.feedURI),
                                                 qdata.index);
          }

          if (qdata.isBookmark) {
            let itemId = PlacesUtils.bookmarks.insertBookmark(qdata.parentFolder,
                                                              uri(qdata.uri),
                                                              qdata.index,
                                                              qdata.title);
            if (qdata.keyword)
              PlacesUtils.bookmarks.setKeywordForBookmark(itemId, qdata.keyword);
            if (qdata.dateAdded)
              PlacesUtils.bookmarks.setItemDateAdded(itemId, qdata.dateAdded);
            if (qdata.lastModified)
              PlacesUtils.bookmarks.setItemLastModified(itemId, qdata.lastModified);
          }

          if (qdata.isTag) {
            PlacesUtils.tagging.tagURI(uri(qdata.uri), qdata.tagArray);
          }

          if (qdata.isDynContainer) {
            PlacesUtils.bookmarks.createDynamicContainer(qdata.parentFolder,
                                                         qdata.title,
                                                         qdata.contractId,
                                                         qdata.index);
          }

          if (qdata.isSeparator)
            PlacesUtils.bookmarks.insertSeparator(qdata.parentFolder, qdata.index);
        } catch (ex) {
          
          LOG("Problem with this URI: " + data.uri);
          do_throw("Error creating database: " + ex + "\n");
        }
      }); 
    }
  }, null);
}













function queryData(obj) {
  this.isVisit = obj.isVisit ? obj.isVisit : false;
  this.isBookmark = obj.isBookmark ? obj.isBookmark: false;
  this.uri = obj.uri ? obj.uri : "";
  this.lastVisit = obj.lastVisit ? obj.lastVisit : today;
  this.referrer = obj.referrer ? obj.referrer : null;
  this.transType = obj.transType ? obj.transType : Ci.nsINavHistoryService.TRANSITION_TYPED;
  this.isRedirect = obj.isRedirect ? obj.isRedirect : false;
  this.sessionID = obj.sessionID ? obj.sessionID : 0;
  this.isDetails = obj.isDetails ? obj.isDetails : false;
  this.title = obj.title ? obj.title : "";
  this.markPageAsTyped = obj.markPageAsTyped ? obj.markPageAsTyped : false;
  this.hidePage = obj.hidePage ? obj.hidePage : false;
  this.isPageAnnotation = obj.isPageAnnotation ? obj.isPageAnnotation : false;
  this.removeAnnotation= obj.removeAnnotation ? true : false;
  this.annoName = obj.annoName ? obj.annoName : "";
  this.annoVal = obj.annoVal ? obj.annoVal : "";
  this.annoFlags = obj.annoFlags ? obj.annoFlags : 0;
  this.annoExpiration = obj.annoExpiration ? obj.annoExpiration : 0;
  this.isItemAnnotation = obj.isItemAnnotation ? obj.isItemAnnotation : false;
  this.itemId = obj.itemId ? obj.itemId : 0;
  this.isPageBinaryAnnotation = obj.isPageBinaryAnnotation ?
                                obj.isPageBinaryAnnotation : false;
  this.isItemBinaryAnnotation = obj.isItemBinaryAnnotation ?
                                obj.isItemBinaryAnnotation : false;
  this.binaryData = obj.binaryData ? obj.binaryData : null;
  this.binaryDataLength = obj.binaryDataLength ? obj.binaryDataLength : 0;
  this.annoMimeType = obj.annoMimeType ? obj.annoMimeType : "";
  this.isTag = obj.isTag ? obj.isTag : false;
  this.tagArray = obj.tagArray ? obj.tagArray : null;
  this.isFavicon = obj.isFavicon ? obj.isFavicon : false;
  this.faviconURI = obj.faviconURI ? obj.faviconURI : "";
  this.faviconLen = obj.faviconLen ? obj.faviconLen : 0;
  this.faviconMimeType = obj.faviconMimeType ? obj.faviconMimeType : "";
  this.faviconExpiration = obj.faviconExpiration ?
                           obj.faviconExpiration : futureday;
  this.isLivemark = obj.isLivemark ? obj.isLivemark : false;
  this.parentFolder = obj.parentFolder ? obj.parentFolder
                                       : PlacesUtils.placesRootId;
  this.feedURI = obj.feedURI ? obj.feedURI : "";
  this.index = obj.index ? obj.index : PlacesUtils.bookmarks.DEFAULT_INDEX;
  this.isFolder = obj.isFolder ? obj.isFolder : false;
  this.contractId = obj.contractId ? obj.contractId : "";
  this.lastModified = obj.lastModified ? obj.lastModified : today;
  this.dateAdded = obj.dateAdded ? obj.dateAdded : today;
  this.keyword = obj.keyword ? obj.keyword : "";
  this.visitCount = obj.visitCount ? obj.visitCount : 0;
  this.readOnly = obj.readOnly ? obj.readOnly : false;
  this.isSeparator = obj.hasOwnProperty("isSeparator") && obj.isSeparator;

  
  
  this.isInQuery = obj.isInQuery ? obj.isInQuery : false;
}


queryData.prototype = { }








function compareArrayToResult(aArray, aRoot) {
  LOG("Comparing Array to Results");

  var wasOpen = aRoot.containerOpen;
  if (!wasOpen)
    aRoot.containerOpen = true;

  
  var expectedResultCount = aArray.filter(function(aEl) { return aEl.isInQuery; }).length;
  do_check_eq(expectedResultCount, aRoot.childCount);

  var inQueryIndex = 0;
  for (var i = 0; i < aArray.length; i++) {
    if (aArray[i].isInQuery) {
      var child = aRoot.getChild(inQueryIndex);
      
      if (!aArray[i].isFolder && !aArray[i].isSeparator) {
        LOG("testing testData[" + aArray[i].uri + "] vs result[" + child.uri + "]");
        if (aArray[i].uri != child.uri) {
          dump_table("moz_places");
          do_throw("Expected " + aArray[i].uri + " found " + child.uri);
        }
      }
      if (!aArray[i].isSeparator && aArray[i].title != child.title)
        do_throw("Expected " + aArray[i].title + " found " + child.title);
      if (aArray[i].hasOwnProperty("lastVisit") &&
          aArray[i].lastVisit != child.time)
        do_throw("Expected " + aArray[i].lastVisit + " found " + child.time);
      if (aArray[i].hasOwnProperty("index") &&
          aArray[i].index != PlacesUtils.bookmarks.DEFAULT_INDEX &&
          aArray[i].index != child.bookmarkIndex)
        do_throw("Expected " + aArray[i].index + " found " + child.bookmarkIndex);

      inQueryIndex++;
    }
  }

  if (!wasOpen)
    aRoot.containerOpen = false;
  LOG("Comparing Array to Results passes");
}











function isInResult(aQueryData, aRoot) {
  var rv = false;
  var uri;
  var wasOpen = aRoot.containerOpen;
  if (!wasOpen)
    aRoot.containerOpen = true;

  
  
  if ("uri" in aQueryData) {
    uri = aQueryData.uri;
  } else {
    uri = aQueryData[0].uri;
  }

  for (var i=0; i < aRoot.childCount; i++) {
    if (uri == aRoot.getChild(i).uri) {
      rv = true;
      break;
    }
  }
  if (!wasOpen)
    aRoot.containerOpen = false;
  return rv;
}






function displayResultSet(aRoot) {

  var wasOpen = aRoot.containerOpen;
  if (!wasOpen)
    aRoot.containerOpen = true;

  if (!aRoot.hasChildren) {
    
    LOG("Result Set Empty");
    return;
  }

  for (var i=0; i < aRoot.childCount; ++i) {
    LOG("Result Set URI: " + aRoot.getChild(i).uri + "   Title: " +
        aRoot.getChild(i).title + "   Visit Time: " + aRoot.getChild(i).time);
  }
  if (!wasOpen)
    aRoot.containerOpen = false;
}
