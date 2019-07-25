







































function test() {
  const BOOKMARKS_SIDEBAR_ID = "viewBookmarksSidebar";
  const BOOKMARKS_SIDEBAR_TREE_ID = "bookmarks-view";
  const HISTORY_SIDEBAR_ID = "viewHistorySidebar";
  const HISTORY_SIDEBAR_TREE_ID = "historyTree";

  
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let bs = PlacesUtils.bookmarks;
  let hs = PlacesUtils.history;
  let sidebarBox = document.getElementById("sidebar-box");
  let sidebar = document.getElementById("sidebar");
  waitForExplicitFinish();

  
  if (!sidebarBox.hidden) {
    info("Unexpected sidebar found - a previous test failed to cleanup correctly");
    toggleSidebar();
  }

  const TEST_URL = "http://mochi.test:8888/browser/browser/components/places/tests/browser/sidebarpanels_click_test_page.html";

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
      let uri = PlacesUtils._uri(TEST_URL);
      hs.addVisit(uri, Date.now() * 1000, null, hs.TRANSITION_TYPED, false, 0);
      let gh = hs.QueryInterface(Ci.nsIGlobalHistory2);
      ok(gh.isVisited(uri), "Item is visited");
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

        function observer(aSubject, aTopic, aData) {
          info("alert dialog observed as expected");
          os.removeObserver(observer, "common-dialog-loaded");
          os.removeObserver(observer, "tabmodal-dialog-loaded");

          aSubject.Dialog.ui.button0.click();

          executeSoon(function () {
              toggleSidebar(currentTest.sidebarName);
              currentTest.cleanup();
              postFunc();
            });
        }
        os.addObserver(observer, "common-dialog-loaded", false);
        os.addObserver(observer, "tabmodal-dialog-loaded", false);

        
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
    
    for (let tabCount = gBrowser.tabContainer.childNodes.length;
         tabCount > 1; tabCount--) {
      gBrowser.selectedTab = gBrowser.tabContainer.childNodes[tabCount - 1];
      gBrowser.removeCurrentTab();
    }

    if (tests.length == 0)
      finish();
    else {
      
      gBrowser.selectedTab = gBrowser.addTab();

      
      currentTest = tests.shift();
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
