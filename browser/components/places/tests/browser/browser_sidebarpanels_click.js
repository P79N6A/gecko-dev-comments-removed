






function test() {
  waitForExplicitFinish();
  ignoreAllUncaughtExceptions();

  const BOOKMARKS_SIDEBAR_ID = "viewBookmarksSidebar";
  const BOOKMARKS_SIDEBAR_TREE_ID = "bookmarks-view";
  const HISTORY_SIDEBAR_ID = "viewHistorySidebar";
  const HISTORY_SIDEBAR_TREE_ID = "historyTree";
  const TEST_URL = "http://mochi.test:8888/browser/browser/components/places/tests/browser/sidebarpanels_click_test_page.html";

  
  if (!document.getElementById("sidebar-box").hidden) {
    info("Unexpected sidebar found - a previous test failed to cleanup correctly");
    SidebarUI.hide();
  }

  let sidebar = document.getElementById("sidebar");
  let tests = [];
  let currentTest;

  tests.push({
    _itemID: null,
    init: function(aCallback) {
      
      this._itemID = PlacesUtils.bookmarks.insertBookmark(
        PlacesUtils.unfiledBookmarksFolderId, PlacesUtils._uri(TEST_URL),
        PlacesUtils.bookmarks.DEFAULT_INDEX, "test"
      );
      aCallback();
    },
    prepare: function() {
    },
    selectNode: function(tree) {
      tree.selectItems([this._itemID]);
    },
    cleanup: function(aCallback) {
      PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);
      executeSoon(aCallback);
    },
    sidebarName: BOOKMARKS_SIDEBAR_ID,
    treeName: BOOKMARKS_SIDEBAR_TREE_ID,
    desc: "Bookmarks sidebar test"
  });

  tests.push({
    init: function(aCallback) {
      
      let uri = PlacesUtils._uri(TEST_URL);
      PlacesTestUtils.addVisits({
        uri: uri, visitDate: Date.now() * 1000,
        transition: PlacesUtils.history.TRANSITION_TYPED
      }).then(aCallback);
    },
    prepare: function() {
      sidebar.contentDocument.getElementById("byvisited").doCommand();
    },
    selectNode: function(tree) {
      tree.selectNode(tree.view.nodeForTreeIndex(0));
      is(tree.selectedNode.uri, TEST_URL, "The correct visit has been selected");
      is(tree.selectedNode.itemId, -1, "The selected node is not bookmarked");
    },
    cleanup: function(aCallback) {
      PlacesTestUtils.clearHistory().then(aCallback);
    },
    sidebarName: HISTORY_SIDEBAR_ID,
    treeName: HISTORY_SIDEBAR_TREE_ID,
    desc: "History sidebar test"
  });

  function testPlacesPanel(preFunc, postFunc) {
    currentTest.init(function() {
      SidebarUI.show(currentTest.sidebarName);
    });

    sidebar.addEventListener("load", function() {
      sidebar.removeEventListener("load", arguments.callee, true);
      executeSoon(function() {
        currentTest.prepare();

        if (preFunc)
          preFunc();

        function observer(aSubject, aTopic, aData) {
          info("alert dialog observed as expected");
          Services.obs.removeObserver(observer, "common-dialog-loaded");
          Services.obs.removeObserver(observer, "tabmodal-dialog-loaded");

          aSubject.Dialog.ui.button0.click();

          executeSoon(function () {
              SidebarUI.hide();
              currentTest.cleanup(postFunc);
            });
        }
        Services.obs.addObserver(observer, "common-dialog-loaded", false);
        Services.obs.addObserver(observer, "tabmodal-dialog-loaded", false);

        let tree = sidebar.contentDocument.getElementById(currentTest.treeName);

        
        currentTest.selectNode(tree);

        synthesizeClickOnSelectedTreeCell(tree);
        
        
        
        
        
        
      });
    }, true);
  }

  function changeSidebarDirection(aDirection) {
    sidebar.contentDocument.documentElement.style.direction = aDirection;
  }

  function runNextTest() {
    
    while (gBrowser.tabs.length > 1) {
      gBrowser.removeTab(gBrowser.tabContainer.lastChild);
    }

    if (tests.length == 0) {
      finish();
    }
    else {
      
      gBrowser.selectedTab = gBrowser.addTab();
      currentTest = tests.shift();
      testPlacesPanel(function() {
                        changeSidebarDirection("ltr");
                        info("Running " + currentTest.desc + " in LTR mode");
                      },
                      function() {
                        testPlacesPanel(function() {
                          
                          changeSidebarDirection("rtl");
                          info("Running " + currentTest.desc + " in RTL mode");
                        },
                        function() {
                          runNextTest();
                        });
                      });
    }
  }

  
  PlacesTestUtils.clearHistory().then(runNextTest);
}
