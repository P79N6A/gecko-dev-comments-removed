










































let gLeftPaneFolderIdGetter;
let gAllBookmarksFolderIdGetter;

let gReferenceJSON;
let gLeftPaneFolderId;

let gFolderId;


let gTests = [

  function test1() {
    print("1. Do nothing, checks test calibration.");
  },

  function test2() {
    print("2. Delete the left pane folder.");
    PlacesUtils.bookmarks.removeItem(gLeftPaneFolderId);
  },

  function test3() {
    print("3. Delete a child of the left pane folder.");
    let id = PlacesUtils.bookmarks.getIdForItemAt(gLeftPaneFolderId, 0);
    PlacesUtils.bookmarks.removeItem(id);
  },

  function test4() {
    print("4. Delete AllBookmarks.");
    PlacesUtils.bookmarks.removeItem(PlacesUIUtils.allBookmarksFolderId);
  },

  function test5() {
    print("5. Create a duplicated left pane folder.");
    let id = PlacesUtils.bookmarks.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                                "PlacesRoot",
                                                PlacesUtils.bookmarks.DEFAULT_INDEX);
    PlacesUtils.annotations.setItemAnnotation(id, ORGANIZER_FOLDER_ANNO,
                                              "PlacesRoot", 0,
                                              PlacesUtils.annotations.EXPIRE_NEVER);
  },

  function test6() {
    print("6. Create a duplicated left pane query.");
    let id = PlacesUtils.bookmarks.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                                "AllBookmarks",
                                                PlacesUtils.bookmarks.DEFAULT_INDEX);
    PlacesUtils.annotations.setItemAnnotation(id, ORGANIZER_QUERY_ANNO,
                                              "AllBookmarks", 0,
                                              PlacesUtils.annotations.EXPIRE_NEVER);
  },

  function test7() {
    print("7. Remove the left pane folder annotation.");
    PlacesUtils.annotations.removeItemAnnotation(gLeftPaneFolderId,
                                                 ORGANIZER_FOLDER_ANNO);
  },

  function test8() {
    print("8. Remove a left pane query annotation.");
    PlacesUtils.annotations.removeItemAnnotation(PlacesUIUtils.allBookmarksFolderId,
                                                 ORGANIZER_QUERY_ANNO);
  },

  function test9() {
    print("9. Remove a child of AllBookmarks.");
    let id = PlacesUtils.bookmarks.getIdForItemAt(PlacesUIUtils.allBookmarksFolderId, 0);
    PlacesUtils.bookmarks.removeItem(id);
  },

];

function run_test() {
  
  remove_all_bookmarks();

  
  let scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                     getService(Ci.mozIJSSubScriptLoader);
  scriptLoader.loadSubScript("chrome://browser/content/places/utils.js", this);
  do_check_true(!!PlacesUIUtils);

  
  gLeftPaneFolderIdGetter = PlacesUIUtils.__lookupGetter__("leftPaneFolderId");
  do_check_eq(typeof(gLeftPaneFolderIdGetter), "function");
  gAllBookmarksFolderIdGetter = PlacesUIUtils.__lookupGetter__("allBookmarksFolderId");
  do_check_eq(typeof(gAllBookmarksFolderIdGetter), "function");

  
  gFolderId = PlacesUtils.bookmarks.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                                 "test",
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX);
  PlacesUtils.annotations.setItemAnnotation(gFolderId, ORGANIZER_QUERY_ANNO,
                                            "test", 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);

  
  
  gLeftPaneFolderId = PlacesUIUtils.leftPaneFolderId;
  gReferenceJSON = folderToJSON(gLeftPaneFolderId);

  
  do_test_pending();
  do_timeout(0, run_next_test);
}

function run_next_test() {
  if (gTests.length) {
    
    let test = gTests.shift();
    test();
    
    PlacesUIUtils.__defineGetter__("leftPaneFolderId", gLeftPaneFolderIdGetter);
    gLeftPaneFolderId = PlacesUIUtils.leftPaneFolderId;
    PlacesUIUtils.__defineGetter__("allBookmarksFolderId", gAllBookmarksFolderIdGetter);
    
    let leftPaneJSON = folderToJSON(gLeftPaneFolderId);
    do_check_true(compareJSON(gReferenceJSON, leftPaneJSON));
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(gFolderId), "test");
    
    do_timeout(0, run_next_test);
  }
  else {
    
    remove_all_bookmarks();
    do_test_finished();
  }
}




function folderToJSON(aItemId) {
  let query = PlacesUtils.history.getNewQuery();
  query.setFolders([aItemId], 1);
  let options = PlacesUtils.history.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  let root = PlacesUtils.history.executeQuery(query, options).root;
  let writer = {
    value: "",
    write: function PU_wrapNode__write(aStr, aLen) {
      this.value += aStr;
    }
  };
  PlacesUtils.serializeNodeAsJSONToOutputStream(root, writer, false, false);
  do_check_true(writer.value.length > 0);
  return writer.value;
}





function compareJSON(aNodeJSON_1, aNodeJSON_2) {
  let JSON = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
  node1 = JSON.decode(aNodeJSON_1);
  node2 = JSON.decode(aNodeJSON_2);

  
  const SKIP_PROPS = ["dateAdded", "lastModified", "id"];

  function compareObjects(obj1, obj2) {
    do_check_eq(obj1.__count__, obj2.__count__);
    for (let prop in obj1) {
      
      if (SKIP_PROPS.indexOf(prop) != -1)
        continue;
      
      if (!obj1[prop])
        continue;
      if (typeof(obj1[prop]) == "object")
        return compareObjects(obj1[prop], obj2[prop]);
      if (obj1[prop] !== obj2[prop]) {
        print(prop + ": " + obj1[prop] + "!=" + obj2[prop]);
        return false;
      }
    }
    return true;
  }

  return compareObjects(node1, node2);
}
