







































function test() {
  const BOOKMARKS_SIDEBAR_ID = "viewBookmarksSidebar";
  const BOOKMARKS_SIDEBAR_TREE_ID = "bookmarks-view";
  const HISTORY_SIDEBAR_ID = "viewHistorySidebar";
  const HISTORY_SIDEBAR_TREE_ID = "historyTree";

  
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  let bs = PlacesUtils.bookmarks;
  let hs = PlacesUtils.history;
  let sidebarBox = document.getElementById("sidebar-box");
  let sidebar = document.getElementById("sidebar");
  waitForExplicitFinish();

  
  if (!sidebarBox.hidden) {
    info("Unexpected sidebar found - a previous test failed to cleanup correctly");
    toggleSidebar();
  }

  const TEST_URL = "javascript:alert(\"test\");";

  let tests = [];
  tests.push({
    _itemID: null,
    init: function() {
      
      this._itemID = bs.insertBookmark(bs.unfiledBookmarksFolder,
                                       PlacesUtils._uri(TEST_URL),
                                       bs.DEFAULT_INDEX, "test");
    },
    prepare: function() {
    },
    selectNode: function(tree) {
      tree.selectItems([this._itemID]);
    },
    cleanup: function() {
      bs.removeFolderChildren(bs.unfiledBookmarksFolder);
    },
    sidebarName: BOOKMARKS_SIDEBAR_ID,
    treeName: BOOKMARKS_SIDEBAR_TREE_ID,
    desc: "Bookmarks sidebar test"
  });

  tests.push({
    init: function() {
      
      this.cleanup();
      hs.addVisit(PlacesUtils._uri(TEST_URL), Date.now() * 1000,
                  null, hs.TRANSITION_TYPED, false, 0);
    },
    prepare: function() {
      sidebar.contentDocument.getElementById("byvisited").doCommand();
    },
    selectNode: function(tree) {
      tree.selectNode(tree.view.nodeForTreeIndex(0));
      is(tree.selectedNode.uri, TEST_URL, "The correct visit has been selected");
      is(tree.selectedNode.itemId, -1, "The selected node is not bookmarked");
    },
    cleanup: function() {
      hs.QueryInterface(Ci.nsIBrowserHistory)
        .removeAllPages();
    },
    sidebarName: HISTORY_SIDEBAR_ID,
    treeName: HISTORY_SIDEBAR_TREE_ID,
    desc: "History sidebar test"
  });

  let currentTest;

  function testPlacesPanel(preFunc, postFunc) {
    currentTest.init();

    sidebar.addEventListener("load", function() {
      sidebar.removeEventListener("load", arguments.callee, true);

      let doc = sidebar.contentDocument;
      let tree = doc.getElementById(currentTest.treeName);
      let tbo = tree.treeBoxObject;

      executeSoon(function() {
        currentTest.prepare();
        if (preFunc)
          preFunc();

        let observer = {
          observe: function(aSubject, aTopic, aData) {
            if (aTopic === "domwindowopened") {
              ww.unregisterNotification(this);
              let alertDialog = aSubject.QueryInterface(Ci.nsIDOMWindow);
              alertDialog.addEventListener("load", function() {
                alertDialog.removeEventListener("load", arguments.callee, false);
                info("alert dialog observed as expected");
                executeSoon(function() {
                  alertDialog.close();
                  toggleSidebar(currentTest.sidebarName);
                  currentTest.cleanup();
                  postFunc();
                });
              }, false);
            }
          }
        };
        ww.registerNotification(observer);

        
        currentTest.selectNode(tree);
        is(tbo.view.selection.count, 1,
           "The test node should be successfully selected");
        
        let min = {}, max = {};
        tbo.view.selection.getRangeAt(0, min, max);
        let rowID = min.value;
        tbo.ensureRowIsVisible(rowID);

        
        let x = {}, y = {}, width = {}, height = {};
        tbo.getCoordsForCellItem(rowID, tree.columns[0], "text",
                                 x, y, width, height);
        x = x.value + width.value / 2;
        y = y.value + height.value / 2;
        
        EventUtils.synthesizeMouse(tree.body, x, y, {}, doc.defaultView);
        
        
        
        
        
        
      });
    }, true);
    toggleSidebar(currentTest.sidebarName);
  }

  function changeSidebarDirection(aDirection) {
    document.getElementById("sidebar")
            .contentDocument
            .documentElement
            .style.direction = aDirection;
  }

  function runNextTest() {
    if (tests.length == 0)
      finish();
    else {
      currentTest = tests.push();
      testPlacesPanel(function() {
        changeSidebarDirection("ltr");
        info("Running " + currentTest.desc + " in LTR mode");
      }, function() {
        executeSoon(function() {
          testPlacesPanel(function() {
            
            changeSidebarDirection("rtl");
            info("Running " + currentTest.desc + " in RTL mode");
          }, function() {
            executeSoon(runNextTest);
          });
        });
      });
    }
  }

  runNextTest();
}
