








































const Cc = Components.classes;
const Ci = Components.interfaces;


const SIDEBAR_HISTORY_TREE_ID = "historyTree";
const SIDEBAR_BOOKMARKS_TREE_ID = "bookmarks-view";

const SIDEBAR_HISTORY_ID = "viewHistorySidebar";
const SIDEBAR_BOOKMARKS_ID = "viewBookmarksSidebar";


const SIDEBAR_HISTORY_BYLASTVISITED_VIEW = "bylastvisited";
const SIDEBAR_HISTORY_BYMOSTVISITED_VIEW = "byvisited";
const SIDEBAR_HISTORY_BYDATE_VIEW = "byday";
const SIDEBAR_HISTORY_BYSITE_VIEW = "bysite";
const SIDEBAR_HISTORY_BYDATEANDSITE_VIEW = "bydateandsite";


const ACTION_EDIT = 0;
const ACTION_ADD = 1;



const TYPE_FOLDER = 0;
const TYPE_BOOKMARK = 1;

const TEST_URL = "http://www.mozilla.org/";

var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
         getService(Ci.nsIWindowMediator);
var win = wm.getMostRecentWindow("navigator:browser");
var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
         getService(Ci.nsIWindowWatcher);

function add_visit(aURI, aDate) {
  var visitId = PlacesUtils.history
                           .addVisit(aURI,
                                     aDate,
                                     null, 
                                     PlacesUtils.history.TRANSITION_TYPED,
                                     false, 
                                     0);
  return visitId;
}

function add_bookmark(aURI) {
  var bId = PlacesUtils.bookmarks
                       .insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                       aURI,
                                       PlacesUtils.bookmarks.DEFAULT_INDEX,
                                       "bookmark/" + aURI.spec);
  return bId;
}


var gTests = [];
var gCurrentTest = null;








































gTests.push({
  desc: "Bug 479348 - Properties on a root should be read-only",
  sidebar: SIDEBAR_BOOKMARKS_ID,
  action: ACTION_EDIT,
  itemType: null,
  window: null,

  setup: function() {
    
  },

  selectNode: function(tree) {
    
    var itemId = PlacesUIUtils.leftPaneQueries["UnfiledBookmarks"];
    tree.selectItems([itemId]);
    this.selectedNode = tree.selectedNode;
  },

  run: function() {
    
    ok(this.window.BookmarkPropertiesPanel._readOnly, "Dialog is read-only");

    
    var acceptButton = this.window.document.documentElement.getButton("accept");
    ok(acceptButton.disabled, "Accept button is disabled");

    
    var namepicker = this.window.document.getElementById("editBMPanel_namePicker");
    ok(namepicker.readOnly, "Name field is disabled");
    is(namepicker.value,
       PlacesUtils.bookmarks.getItemTitle(PlacesUtils.unfiledBookmarksFolderId),
       "Node title is correct");
    
    this.window.gEditItemOverlay.onNamePickerChange();
    is(namepicker.value,
       PlacesUtils.bookmarks.getItemTitle(PlacesUtils.unfiledBookmarksFolderId),
       "Root title is correct");
    
    is(PlacesUtils.bookmarks.getItemTitle(this.selectedNode.itemId), null,
       "Shortcut title is null");
    this.finish();
  },

  finish: function() {
    this.window.document.documentElement.cancelDialog();
    toggleSidebar(this.sidebar, false);
    runNextTest();
  },

  cleanup: function() {
    
  }
});




gTests.push({
  desc: "Bug 462662 - Pressing Enter to select tag from autocomplete closes bookmarks properties dialog",
  sidebar: SIDEBAR_BOOKMARKS_ID,
  action: ACTION_EDIT,
  itemType: null,
  window: null,
  _itemId: null,

  setup: function() {
    
    this._itemId = add_bookmark(PlacesUtils._uri(TEST_URL));
    ok(this._itemId > 0, "Correctly added a bookmark");
    
    PlacesUtils.tagging.tagURI(PlacesUtils._uri(TEST_URL),
                               ["testTag"]);
    var tags = PlacesUtils.tagging.getTagsForURI(PlacesUtils._uri(TEST_URL));
    is(tags[0], "testTag", "Correctly added a tag");
  },

  selectNode: function(tree) {
    tree.selectItems([this._itemId]);
    is(tree.selectedNode.itemId, this._itemId, "Bookmark has been selected");
  },

  run: function() {
    
    var tagsField = this.window.document.getElementById("editBMPanel_tagsField");
    var self = this;

    var windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowclosed") {
          ww.unregisterNotification(this);
          tagsField.popup.removeEventListener("popuphidden", popupListener, true);
          ok(false, "Dialog window should not be closed by pressing Enter on the autocomplete popup");
          self.finish();
        }
      }
    };

    var popupListener = {
      handleEvent: function(aEvent) {
        switch (aEvent.type) {
          case "popuphidden":
            
            ww.unregisterNotification(windowObserver);
            tagsField.popup.removeEventListener("popuphidden", this, true);
            self.window.document.documentElement.cancelDialog();
            self.finish();
            break;
          case "popupshown":
            tagsField.popup.removeEventListener("popupshown", this, true);
            
            
            ww.registerNotification(windowObserver);
            var tree = tagsField.popup.tree;
            
            isnot(tree, null, "Autocomplete results tree exists");
            is(tree.view.rowCount, 1, "We have 1 autocomplete result");
            tagsField.popup.selectedIndex = 0;
            is(tree.view.selection.count, 1,
               "We have selected a tag from the autocomplete popup");
            info("About to focus the autocomplete results tree");
            tree.focus();
            EventUtils.synthesizeKey("VK_RETURN", {}, self.window);
            break;
          default:
            ok(false, "unknown event: " + aEvent.type);
            return;
        }
      }
    };
    tagsField.popup.addEventListener("popupshown", popupListener, true);
    tagsField.popup.addEventListener("popuphidden", popupListener, true);

    
    info("About to focus the tagsField");
    tagsField.focus();
    tagsField.value = "";
    EventUtils.synthesizeKey("t", {}, this.window);
  },

  finish: function() {
    toggleSidebar(this.sidebar, false);
    runNextTest();
  },

  cleanup: function() {
    
    var tags = PlacesUtils.tagging.getTagsForURI(PlacesUtils._uri(TEST_URL));
    is(tags[0], "testTag", "Tag on node has not changed");

    
    PlacesUtils.tagging.untagURI(PlacesUtils._uri(TEST_URL), ["testTag"]);
    PlacesUtils.bookmarks.removeItem(this._itemId);
  }
});




gTests.push({
  desc: "Bug 475529 - Add button in new folder dialog not default anymore",
  sidebar: SIDEBAR_BOOKMARKS_ID,
  action: ACTION_ADD,
  itemType: TYPE_FOLDER,
  window: null,
  _itemId: null,

  setup: function() {
    
  },

  selectNode: function(tree) {
    
    var itemId = PlacesUIUtils.leftPaneQueries["UnfiledBookmarks"];
    tree.selectItems([itemId]);
    this.selectedNode = tree.selectedNode;
  },

  run: function() {
    this._itemId = this.window.gEditItemOverlay._itemId;
    
    var namePicker = this.window.document.getElementById("editBMPanel_namePicker");
    var userEnteredName = this.window.document.getElementById("editBMPanel_userEnteredName");
    var self = this;
    this.window.addEventListener("unload", function(event) {
        this.window.removeEventListener("unload", arguments.callee, false);
        executeSoon(function() {
          self.finish();
        });
      }, false);
    namePicker.value = "n";
    userEnteredName.label = "n";
    info("About to focus the namePicker field");
    namePicker.focus();
    EventUtils.synthesizeKey("VK_RETURN", {}, this.window);
  },

  finish: function() {
    
    toggleSidebar(this.sidebar, false);
    runNextTest();
  },

  cleanup: function() {
    
    is(PlacesUtils.bookmarks.getItemTitle(this._itemId), "n",
       "Folder name has been edited");

    
    PlacesUtils.bookmarks.removeItem(this._itemId);
  }
});




gTests.push({
  desc: "Bug 476020 - Pressing Esc while having the tag autocomplete open closes the bookmarks panel",
  sidebar: SIDEBAR_BOOKMARKS_ID,
  action: ACTION_EDIT,
  itemType: null,
  window: null,
  _itemId: null,

  setup: function() {
    
    this._itemId = add_bookmark(PlacesUtils._uri(TEST_URL));
    ok(this._itemId > 0, "Correctly added a bookmark");
    
    PlacesUtils.tagging.tagURI(PlacesUtils._uri(TEST_URL),
                               ["testTag"]);
    var tags = PlacesUtils.tagging.getTagsForURI(PlacesUtils._uri(TEST_URL));
    is(tags[0], "testTag", "Correctly added a tag");
  },

  selectNode: function(tree) {
    tree.selectItems([this._itemId]);
    is(tree.selectedNode.itemId, this._itemId, "Bookmark has been selected");
  },

  run: function() {
    
    var tagsField = this.window.document.getElementById("editBMPanel_tagsField");
    var self = this;

    var windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowclosed") {
          ww.unregisterNotification(this);
          tagsField.popup.removeEventListener("popuphidden", popupListener, true);
          ok(false, "Dialog window should not be closed by pressing Escape on the autocomplete popup");
          self.finish();
        }
      }
    };

    var popupListener = {
      handleEvent: function(aEvent) {
        switch (aEvent.type) {
          case "popuphidden":
            
            ww.unregisterNotification(windowObserver);
            tagsField.popup.removeEventListener("popuphidden", this, true);
            self.window.document.documentElement.cancelDialog();
            self.finish();
            break;
          case "popupshown":
            tagsField.popup.removeEventListener("popupshown", this, true);
            
            
            ww.registerNotification(windowObserver);
            var tree = tagsField.popup.tree;
            
            isnot(tree, null, "Autocomplete results tree exists");
            is(tree.view.rowCount, 1, "We have 1 autocomplete result");
            tagsField.popup.selectedIndex = 0;
            is(tree.view.selection.count, 1,
               "We have selected a tag from the autocomplete popup");
            info("About to focus the autocomplete results tree");
            tree.focus();
            EventUtils.synthesizeKey("VK_ESCAPE", {}, self.window);
            break;
          default:
            ok(false, "unknown event: " + aEvent.type);
            return;
        }
      }
    };
    tagsField.popup.addEventListener("popupshown", popupListener, true);
    tagsField.popup.addEventListener("popuphidden", popupListener, true);

    
    info("About to focus the tagsField");
    tagsField.focus();
    tagsField.value = "";
    EventUtils.synthesizeKey("t", {}, this.window);
  },

  finish: function() {
    toggleSidebar(this.sidebar, false);
    runNextTest();
  },

  cleanup: function() {
    
    var tags = PlacesUtils.tagging.getTagsForURI(PlacesUtils._uri(TEST_URL));
    is(tags[0], "testTag", "Tag on node has not changed");

    
    PlacesUtils.tagging.untagURI(PlacesUtils._uri(TEST_URL),
                                 ["testTag"]);
    PlacesUtils.bookmarks.removeItem(this._itemId);
  }
});




gTests.push({
  desc: " Bug 491269 - Test that editing folder name in bookmarks properties dialog does not accept the dialog",
  sidebar: SIDEBAR_HISTORY_ID,
  action: ACTION_ADD,
  historyView: SIDEBAR_HISTORY_BYLASTVISITED_VIEW,
  window: null,

  setup: function() {
    
    add_visit(PlacesUtils._uri(TEST_URL), Date.now() * 1000);
    
    var gh = PlacesUtils.history.QueryInterface(Ci.nsIGlobalHistory2);
    ok(gh.isVisited(PlacesUtils._uri(TEST_URL)), TEST_URL + " is a visited url.");
  },

  selectNode: function(tree) {
    var visitNode = tree.view.nodeForTreeIndex(0);
    tree.selectNode(visitNode);
    is(tree.selectedNode.uri, TEST_URL, "The correct visit has been selected");
    is(tree.selectedNode.itemId, -1, "The selected node is not bookmarked");
  },

  run: function() {
    
    var foldersExpander = this.window.document.getElementById("editBMPanel_foldersExpander");
    var folderTree = this.window.document.getElementById("editBMPanel_folderTree");
    var self = this;

    var windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowclosed") {
          ww.unregisterNotification(this);
          ok(self._cleanShutdown,
             "Dialog window should not be closed by pressing ESC in folder name textbox");
          self.finish();
        }
      }
    };
    ww.registerNotification(windowObserver);

    folderTree.addEventListener("DOMAttrModified", function onDOMAttrModified(event) {
      if (event.attrName != "place")
        return;
      folderTree.removeEventListener("DOMAttrModified", arguments.callee, false);
      executeSoon(function () {
        
        var newFolderButton = self.window.document.getElementById("editBMPanel_newFolderButton");
        newFolderButton.doCommand();
        ok(folderTree.hasAttribute("editing"),
           "We are editing new folder name in folder tree");

        
        EventUtils.synthesizeKey("VK_ESCAPE", {}, self.window);
        ok(!folderTree.hasAttribute("editing"),
           "We have finished editing folder name in folder tree");
        self._cleanShutdown = true;
        self.window.document.documentElement.cancelDialog();
      });
    }, false);
    foldersExpander.doCommand();
  },

  finish: function() {
    toggleSidebar(this.sidebar, false);
    runNextTest();
  },

  cleanup: function() {
    var bh = PlacesUtils.history.QueryInterface(Ci.nsIBrowserHistory);
    bh.removeAllPages();
  }
});



function test() {
  waitForExplicitFinish();
  
  ok(PlacesUtils, "PlacesUtils in context");
  ok(PlacesUIUtils, "PlacesUIUtils in context");

  
  runNextTest();
}

function runNextTest() {
  
  if (gCurrentTest) {
    gCurrentTest.cleanup();
    info("End of test: " + gCurrentTest.desc);
  }

  if (gTests.length > 0) {
    
    gCurrentTest = gTests.shift();
    info("Start of test: " + gCurrentTest.desc);
    gCurrentTest.setup();
    execute_test_in_sidebar();
  }
  else {
    
    finish();
  }
}





function execute_test_in_sidebar() {
    var sidebar = document.getElementById("sidebar");
    sidebar.addEventListener("load", function() {
      sidebar.removeEventListener("load", arguments.callee, true);
      
      executeSoon(open_properties_dialog);
    }, true);
    toggleSidebar(gCurrentTest.sidebar, true);
}

function open_properties_dialog() {
    var sidebar = document.getElementById("sidebar");

    
    if (gCurrentTest.sidebar == SIDEBAR_HISTORY_ID)
      sidebar.contentDocument.getElementById(gCurrentTest.historyView).doCommand();

    
    var sidebarTreeID = gCurrentTest.sidebar == SIDEBAR_BOOKMARKS_ID ?
                                                SIDEBAR_BOOKMARKS_TREE_ID :
                                                SIDEBAR_HISTORY_TREE_ID;
    var tree = sidebar.contentDocument.getElementById(sidebarTreeID);
    ok(tree, "Sidebar tree has been loaded");

    
    gCurrentTest.selectNode(tree);
    ok(tree.selectedNode,
       "We have a places node selected: " + tree.selectedNode.title);

    
    var windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowopened") {
          ww.unregisterNotification(this);
          var win = aSubject.QueryInterface(Ci.nsIDOMWindow);
          win.addEventListener("focus", function(event) {
            win.removeEventListener("focus", arguments.callee, false);
            
            executeSoon(function () {
              
              ok(win.gEditItemOverlay._initialized, "EditItemOverlay is initialized");
              gCurrentTest.window = win;
              try {
                gCurrentTest.run();
              } catch (ex) {
                ok(false, "An error occured during test run: " + ex.message);
              }
            });
          }, false);
        }
      }
    };
    ww.registerNotification(windowObserver);

    var command = null;
    switch(gCurrentTest.action) {
      case ACTION_EDIT:
        command = "placesCmd_show:info";
        break;
      case ACTION_ADD:
        if (gCurrentTest.sidebar == SIDEBAR_BOOKMARKS_ID) {
          if (gCurrentTest.itemType == TYPE_FOLDER)
            command = "placesCmd_new:folder";
          else if (gCurrentTest.itemType == TYPE_BOOKMARK)
            command = "placesCmd_new:bookmark";
          else
            ok(false, "You didn't set a valid itemType for adding an item");
        }
        else
          command = "placesCmd_createBookmark";
        break;
      default:
        ok(false, "You didn't set a valid action for this test");
    }
    
    ok(tree.controller.isCommandEnabled(command),
       " command '" + command + "' on current selected node is enabled");

    
    tree.controller.doCommand(command);
}
