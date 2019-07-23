






































let ac = Cc["@mozilla.org/autocomplete/search;1?name=history"].
  getService(Ci.nsIAutoCompleteSearch);
let bm = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
  getService(Ci.nsINavBookmarksService);
let io = Cc["@mozilla.org/network/io-service;1"].
  getService(Ci.nsIIOService);
let ts = Cc["@mozilla.org/browser/tagging-service;1"].
  getService(Ci.nsITaggingService);

let _ = function(some, debug, message) print(Array.slice(arguments).join(" "));

function run_test() {
  
  var prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefBranch);
  prefs.setIntPref("browser.urlbar.search.sources", 3);
  prefs.setIntPref("browser.urlbar.default.behavior", 0);

  let uri = io.newURI("http://uri/", null, null);
  bm.insertBookmark(bm.toolbarFolder, uri, bm.DEFAULT_INDEX, "title");

  _("Adding 3 tags to the bookmark");
  let tagIds = [];
  for (let i = 0; i < 3; i++) {
    _("Tagging uri with tag:", i);
    ts.tagURI(uri, ["" + i]);
    let id = bm.getIdForItemAt(bm.tagsFolder, i);
    _("Saving bookmark id of tag to rename it later:", id);
    tagIds.push(id);
  }

  _("Search 4 times: make sure we get the right amount of tags then remove one");
  (function doSearch(query) {
    _("Searching for:", query);
    ac.startSearch(query, "", null, {
      onSearchResult: function(search, result) {
        _("Got results with status:", result.searchResult);
        if (result.searchResult != result.RESULT_SUCCESS)
          return;

        let comment = result.getCommentAt(0);
        _("Got the title/tags:", comment);

        _("Until we get bug 489443, we have title and tags separated by \u2013");
        if (comment.indexOf("\u2013") == -1) {
          let num = tagIds.length;
          _("No tags in result, so making sure we have no tags:", num);
          do_check_eq(num, 0);
          do_test_finished();
          return;
        }

        let num = tagIds.length;
        _("Making sure we get the expected number of tags:", num);
        do_check_eq(comment.split(",").length, num);

        _("Removing a tag from the tag ids array:", tagIds);
        let id = tagIds.shift();
        _("Setting an empty title for tag:", id);
        bm.setItemTitle(id, "");

        _("Creating a timer to do a new search to let the current one finish");
        let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        let next = query.slice(1);
        _("Do a new, uncached search with a new query:", next);
        timer.initWithCallback({ notify: function() doSearch(next) },
          0, timer.TYPE_ONE_SHOT);
      }
    });
  })("title");

  do_test_pending();
}
