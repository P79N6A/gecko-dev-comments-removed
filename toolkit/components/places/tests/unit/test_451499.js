






































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                getService(Ci.nsINavBookmarksService);
  var iconsvc = Cc["@mozilla.org/browser/favicon-service;1"].
                  getService(Ci.nsIFaviconService);
} catch(ex) {
  do_throw("Could not get services\n");
}






function readFileData(aFile) {
  var inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                    createInstance(Ci.nsIFileInputStream);
  
  inputStream.init(aFile, 0x01, -1, null);
  var size = inputStream.available();

  
  var bis = Cc["@mozilla.org/binaryinputstream;1"].
            createInstance(Ci.nsIBinaryInputStream);
  bis.setInputStream(inputStream);

  var bytes = bis.readByteArray(size);

  if (size != bytes.length)
      throw "Didn't read expected number of bytes";

  return bytes;
}

var result;


function run_test() {
  var testURI = uri("http://places.test/");

  
  var iconName = "favicon-normal16.png";
  var iconURI = uri("http://places.test/" + iconName);
  var iconMimeType = "image/png";
  var iconFile = do_get_file(iconName);
  var iconData = readFileData(iconFile);
  do_check_eq(iconData.length, 286);
  iconsvc.setFaviconData(iconURI,
                         iconData, iconData.length, iconMimeType,
                         Number.MAX_VALUE);

  
  var testBookmark = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder,
                                          testURI,
                                          bmsvc.DEFAULT_INDEX,
                                          "foo");

  
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([bmsvc.bookmarksMenuFolder, bmsvc.toolbarFolder], 2);
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.maxResults = 10;
  options.excludeQueries = 1;
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  result = histsvc.executeQuery(query, options);
  
  result.viewer = {
                    itemChanged: function(item) {
                      
                      if (item.uri.substr(0,5) == "place")
                        dump("\nTesting itemChanged on: \n " + item.uri + "\n\n");
                        do_check_eq(item.icon.spec, null);
                    }
                  };
  var root = result.root;
  root.containerOpen = true;

  
  
  
  iconsvc.setFaviconUrlForPage(testURI, iconURI);

  do_test_pending();
  
  do_timeout(3500, end_test);
}

function end_test() {
  var root = result.root;
  root.containerOpen = false;
  result.viewer = null;

  do_test_finished();
}
