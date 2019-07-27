





const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


{
  let commonFile = do_get_file("../head_common.js", false);
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








function task_populateDB(aArray)
{
  
  for ([, data] in Iterator(aArray)) {
    try {
      
      
      var qdata = new queryData(data);
      if (qdata.isVisit) {
        
        yield PlacesTestUtils.addVisits({
          uri: uri(qdata.uri),
          transition: qdata.transType,
          visitDate: qdata.lastVisit,
          referrer: qdata.referrer ? uri(qdata.referrer) : null,
          title: qdata.title
        });
        if (qdata.visitCount && !qdata.isDetails) {
          
          
          let stmt = DBConn().createAsyncStatement(
            "UPDATE moz_places SET visit_count = :vc WHERE url = :url");
          stmt.params.vc = qdata.visitCount;
          stmt.params.url = qdata.uri;
          try {
            stmt.executeAsync();
          }
          catch (ex) {
            print("Error while setting visit_count.");
          }
          finally {
            stmt.finalize();
          }
        }
      }

      if (qdata.isRedirect) {
        
        
        let stmt = DBConn().createAsyncStatement(
          "UPDATE moz_places SET hidden = 1 WHERE url = :url");
        stmt.params.url = qdata.uri;
        try {
          stmt.executeAsync();
        }
        catch (ex) {
          print("Error while setting hidden.");
        }
        finally {
          stmt.finalize();
        }
      }

      if (qdata.isDetails) {
        
        yield PlacesTestUtils.addVisits({
          uri: uri(qdata.uri),
          visitDate: qdata.lastVisit,
          title: qdata.title
        });
      }

      if (qdata.markPageAsTyped) {
        PlacesUtils.history.markPageAsTyped(uri(qdata.uri));
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

      if (qdata.isFolder) {
        yield PlacesUtils.bookmarks.insert({
          parentGuid: qdata.parentGuid,
          type: PlacesUtils.bookmarks.TYPE_FOLDER,
          title: qdata.title,
          index: qdata.index
        });
      }

      if (qdata.isLivemark) {
        yield PlacesUtils.livemarks.addLivemark({ title: qdata.title
                                                , parentId: (yield PlacesUtils.promiseItemId(qdata.parentGuid))
                                                , index: qdata.index
                                                , feedURI: uri(qdata.feedURI)
                                                , siteURI: uri(qdata.uri)
                                                });
      }

      if (qdata.isBookmark) {
        let data = {
          parentGuid: qdata.parentGuid,
          index: qdata.index,
          title: qdata.title,
          url: qdata.uri
        };

        if (qdata.dateAdded) {
          data.dateAdded = new Date(qdata.dateAdded / 1000);
        }

        if (qdata.lastModified) {
          data.lastModified = new Date(qdata.lastModified / 1000);
        }

        let item = yield PlacesUtils.bookmarks.insert(data);

        if (qdata.keyword) {
          yield PlacesUtils.keywords.insert({ url: qdata.uri,
                                              keyword: qdata.keyword });
        }
      }

      if (qdata.isTag) {
        PlacesUtils.tagging.tagURI(uri(qdata.uri), qdata.tagArray);
      }

      if (qdata.isSeparator) {
        yield PlacesUtils.bookmarks.insert({
          parentGuid: qdata.parentGuid,
          type: PlacesUtils.bookmarks.TYPE_SEPARATOR,
          index: qdata.index
        });
      }
    } catch (ex) {
      
      LOG("Problem with this URI: " + data.uri);
      do_throw("Error creating database: " + ex + "\n");
    }
  }
}













function queryData(obj) {
  this.isVisit = obj.isVisit ? obj.isVisit : false;
  this.isBookmark = obj.isBookmark ? obj.isBookmark: false;
  this.uri = obj.uri ? obj.uri : "";
  this.lastVisit = obj.lastVisit ? obj.lastVisit : today;
  this.referrer = obj.referrer ? obj.referrer : null;
  this.transType = obj.transType ? obj.transType : Ci.nsINavHistoryService.TRANSITION_TYPED;
  this.isRedirect = obj.isRedirect ? obj.isRedirect : false;
  this.isDetails = obj.isDetails ? obj.isDetails : false;
  this.title = obj.title ? obj.title : "";
  this.markPageAsTyped = obj.markPageAsTyped ? obj.markPageAsTyped : false;
  this.isPageAnnotation = obj.isPageAnnotation ? obj.isPageAnnotation : false;
  this.removeAnnotation= obj.removeAnnotation ? true : false;
  this.annoName = obj.annoName ? obj.annoName : "";
  this.annoVal = obj.annoVal ? obj.annoVal : "";
  this.annoFlags = obj.annoFlags ? obj.annoFlags : 0;
  this.annoExpiration = obj.annoExpiration ? obj.annoExpiration : 0;
  this.isItemAnnotation = obj.isItemAnnotation ? obj.isItemAnnotation : false;
  this.itemId = obj.itemId ? obj.itemId : 0;
  this.annoMimeType = obj.annoMimeType ? obj.annoMimeType : "";
  this.isTag = obj.isTag ? obj.isTag : false;
  this.tagArray = obj.tagArray ? obj.tagArray : null;
  this.isLivemark = obj.isLivemark ? obj.isLivemark : false;
  this.parentGuid = obj.parentGuid || PlacesUtils.bookmarks.rootGuid;
  this.feedURI = obj.feedURI ? obj.feedURI : "";
  this.index = obj.index ? obj.index : PlacesUtils.bookmarks.DEFAULT_INDEX;
  this.isFolder = obj.isFolder ? obj.isFolder : false;
  this.contractId = obj.contractId ? obj.contractId : "";
  this.lastModified = obj.lastModified ? obj.lastModified : null;
  this.dateAdded = obj.dateAdded ? obj.dateAdded : null;
  this.keyword = obj.keyword ? obj.keyword : "";
  this.visitCount = obj.visitCount ? obj.visitCount : 0;
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
  if (expectedResultCount != aRoot.childCount) {
    
    dump_table("moz_places");
    dump_table("moz_historyvisits");
    LOG("Found children:");
    for (let i = 0; i < aRoot.childCount; i++) {
      LOG(aRoot.getChild(i).uri);
    }
    LOG("Expected:");
    for (let i = 0; i < aArray.length; i++) {
      if (aArray[i].isInQuery)
        LOG(aArray[i].uri);
    }
  }
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
