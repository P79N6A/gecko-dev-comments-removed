














































const bmServ = PlacesUtils.bookmarks;
const histServ = PlacesUtils.history;
const lmServ = PlacesUtils.livemarks;

let tests = [

{
  desc: ["Frecency of unvisited, separately bookmarked livemark item's URI ",
         "should be zero after bookmark removed."].join(""),
  run: function ()
  {
    
    
    let lmItemURL = "http://example.com/livemark-item";
    let lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);
    let bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                     lmItemURI,
                                     bmServ.DEFAULT_INDEX,
                                     "bookmark title");
    waitForFrecency(lmItemURL, function(aFrecency) aFrecency > 0,
                    this.check1, this, [lmItemURL, bmId]);
  },
  check1: function (aUrl, aItemId)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    bmServ.removeItem(aItemId);
    waitForFrecency(aUrl, function(aFrecency) aFrecency == 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    print("URI's only bookmark is now unvisited livemark item => frecency = 0");
    do_check_eq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

{
  desc: ["Frecency of visited, separately bookmarked livemark item's URI ",
         "should not be zero after bookmark removed."].join(""),
  run: function ()
  {
    
    
    let lmItemURL = "http://example.com/livemark-item";
    let lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);
    let bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                     lmItemURI,
                                     bmServ.DEFAULT_INDEX,
                                     "bookmark title");
    waitForFrecency(lmItemURL, function(aFrecency) aFrecency > 0,
                    this.check1, this, [lmItemURL, bmId]);
  },
  check1: function (aUrl, aItemId)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    visit(uri(aUrl));
    bmServ.removeItem(aItemId);
    waitForFrecency(aUrl, function(aFrecency) aFrecency > 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    print("URI's only bookmark is now *visited* livemark item => frecency != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

{
  desc: ["After removing bookmark, frecency of bookmark's URI should be zero ",
         "if URI is unvisited and no longer bookmarked."].join(""),
  run: function ()
  {
    let url = "http://example.com/1";
    let bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                     uri(url),
                                     bmServ.DEFAULT_INDEX,
                                     "bookmark title");
    waitForFrecency(url, function(aFrecency) aFrecency > 0,
                    this.check1, this, [url, bmId]);
  },
  check1: function (aUrl, aItemId)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    bmServ.removeItem(aItemId);
    waitForFrecency(aUrl, function(aFrecency) aFrecency == 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    print("Unvisited URI no longer bookmarked => frecency should = 0");
    do_check_eq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

{
  desc: ["After removing bookmark, frecency of bookmark's URI should not be ",
         "zero if URI is visited."].join(""),
  run: function ()
  {
    let bmURL = "http://example.com/1";
    let bmURI = uri(bmURL);

    let bmId = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                     bmURI,
                                     bmServ.DEFAULT_INDEX,
                                     "bookmark title");
    waitForFrecency(bmURL, function(aFrecency) aFrecency > 0,
                    this.check1, this, [bmURL, bmId]);
  },
  check1: function (aUrl, aItemId)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    visit(uri(aUrl));
    bmServ.removeItem(aItemId);
    waitForFrecency(aUrl, function(aFrecency) aFrecency > 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    print("*Visited* URI no longer bookmarked => frecency should != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

{
  desc: ["After removing bookmark, frecency of bookmark's URI should not be ",
         "zero if URI is still bookmarked."].join(""),
  run: function ()
  {
    let bmURL = "http://example.com/1";
    let bmURI = uri(bmURL);

    let bm1Id = bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                                      bmURI,
                                      bmServ.DEFAULT_INDEX,
                                      "bookmark 1 title");

    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          bmURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark 2 title");
    waitForFrecency(bmURL, function(aFrecency) aFrecency > 0,
                    this.check1, this, [bmURL, bm1Id]);
  },
  check1: function (aUrl, aItemId)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    bmServ.removeItem(aItemId);
    waitForFrecency(aUrl, function(aFrecency) aFrecency > 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    print("URI still bookmarked => frecency should != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

{
  desc: ["Frecency of unvisited, separately bookmarked livemark item's URI ",
         "should be zero after all children removed from bookmark's ",
         "parent."].join(""),
  run: function ()
  {
    
    
    let lmItemURL = "http://example.com/livemark-item";
    let lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);

    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          lmItemURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark title");
    waitForFrecency(lmItemURL, function(aFrecency) aFrecency > 0,
                    this.check1, this, [lmItemURL]);
  },
  check1: function (aUrl)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);
    waitForFrecency(aUrl, function(aFrecency) aFrecency == 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    print("URI's only bookmark is now unvisited livemark item => frecency = 0");
    do_check_eq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

{
  desc: ["Frecency of visited, separately bookmarked livemark item's URI ",
         "should not be zero after all children removed from bookmark's ",
         "parent."].join(""),
  run: function ()
  {
    
    
    let lmItemURL = "http://example.com/livemark-item";
    let lmItemURI = uri(lmItemURL);
    createLivemark(lmItemURI);
    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          lmItemURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark title");
    waitForFrecency(lmItemURL, function(aFrecency) aFrecency > 0,
                    this.check1, this, [lmItemURL]);
  },
  check1: function (aUrl, aItemId)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    visit(uri(aUrl));
    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);
    waitForFrecency(aUrl, function(aFrecency) aFrecency > 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    print("URI's only bookmark is now *visited* livemark item => frecency != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

{
  desc: ["After removing all children from bookmark's parent, frecency of ",
         "bookmark's URI should be zero if URI is unvisited and no longer ",
         "bookmarked."].join(""),
  run: function ()
  {
    let url = "http://example.com/1";
    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          uri(url),
                          bmServ.DEFAULT_INDEX,
                          "bookmark title");
    waitForFrecency(url, function(aFrecency) aFrecency > 0,
                    this.check1, this, [url]);
  },
  check1: function (aUrl, aItemId)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);
    waitForFrecency(aUrl, function(aFrecency) aFrecency == 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    print("Unvisited URI no longer bookmarked => frecency should = 0");
    do_check_eq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

{
  desc: ["After removing all children from bookmark's parent, frecency of ",
         "bookmark's URI should not be zero if URI is visited."].join(""),
  run: function ()
  {
    let bmURL = "http://example.com/1";
    let bmURI = uri(bmURL);

    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          bmURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark title");
    waitForFrecency(bmURL, function(aFrecency) aFrecency > 0,
                    this.check1, this, [bmURL]);
  },
  check1: function (aUrl)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    visit(uri(aUrl));
    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);
    waitForFrecency(aUrl, function(aFrecency) aFrecency > 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    print("*Visited* URI no longer bookmarked => frecency should != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

{
  desc: ["After removing all children from bookmark's parent, frecency of ",
         "bookmark's URI should not be zero if URI is still ",
         "bookmarked."].join(""),
  run: function ()
  {
    let bmURL = "http://example.com/1";
    let bmURI = uri(bmURL);

    bmServ.insertBookmark(bmServ.toolbarFolder,
                          bmURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark 1 title");

    bmServ.insertBookmark(bmServ.unfiledBookmarksFolder,
                          bmURI,
                          bmServ.DEFAULT_INDEX,
                          "bookmark 2 title");
    waitForFrecency(bmURL, function(aFrecency) aFrecency > 0,
                    this.check1, this, [bmURL]);
  },
  check1: function (aUrl)
  {
    print("Bookmarked => frecency of URI should be != 0");
    do_check_neq(frecencyForUrl(aUrl), 0);

    bmServ.removeFolderChildren(bmServ.unfiledBookmarksFolder);
    waitForFrecency(aUrl, function(aFrecency) aFrecency > 0,
                    this.check2, this, [aUrl]);
  },
  check2: function (aUrl)
  {
    
    do_check_neq(frecencyForUrl(aUrl), 0);
    run_next_test();
  }
},

];










function createLivemark(aLmChildItemURI) {
  let lmItemId = lmServ.createLivemarkFolderOnly(bmServ.unfiledBookmarksFolder,
                                                 "livemark title",
                                                 uri("http://example.com/"),
                                                 uri("http://example.com/rdf"),
                                                 -1);
  return bmServ.insertBookmark(lmItemId,
                               aLmChildItemURI,
                               bmServ.DEFAULT_INDEX,
                               "livemark item title");
}







function visit(aURI) {
  let visitId = histServ.addVisit(aURI,
                                  Date.now() * 1000,
                                  null,
                                  histServ.TRANSITION_BOOKMARK,
                                  false,
                                  0);
}



function run_test() {
  do_test_pending();
  run_next_test();
}

function run_next_test() {
  if (tests.length) {
    let test = tests.shift();
    print("\n ***Test: " + test.desc);
    remove_all_bookmarks();
    waitForClearHistory(function() {
      test.run.call(test);
    });
  }
  else {
    do_test_finished();
  }
}
