








































const Cc = Components.classes;
const Ci = Components.interfaces;


const SIDEBAR_HISTORY_ID = "historyTree";
const SIDEBAR_BOOKMARKS_ID = "bookmarks-view";


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
    toggleSidebar("viewBookmarksSidebar", false);
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
    var tags = PlacesUtils.tagging.getTagsForURI(PlacesUtils._uri(TEST_URL), {});
    is(tags[0], "testTag", "Correctly added a tag");
  },

  selectNode: function(tree) {
    tree.selectItems([this._itemId]);
    is(tree.selectedNode.itemId, this._itemId, "Bookmark has been selected");
  },

  run: function() {
    
    var tagsField = this.window.document.getElementById("editBMPanel_tagsField");
    var self = this;
    tagsField.popup.addEventListener("popupshown", function (aEvent) {
        tagsField.popup.removeEventListener("popupshown", arguments.callee, true);
        tagsField.popup.focus();
        EventUtils.synthesizeKey("VK_RETURN", {}, self.window);
      }, true);
    tagsField.popup.addEventListener("popuphidden", function (aEvent) {
        tagsField.popup.removeEventListener("popuphidden", arguments.callee, true);
        self.finish();
      }, true);
    tagsField.focus();
    tagsField.value = "";
    EventUtils.synthesizeKey("t", {}, this.window);
  },

  finish: function() {
    isnot(this.window, null, "Window is still open");
    this.window.document.documentElement.cancelDialog();
    toggleSidebar("viewBookmarksSidebar", false);
    runNextTest();
  },

  cleanup: function() {
    
    var tags = PlacesUtils.tagging.getTagsForURI(PlacesUtils._uri(TEST_URL), {});
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
    namePicker.value = "";
    var self = this;
    this.window.addEventListener("unload", function(event) {
        this.window.removeEventListener("unload", arguments.callee, false);
        executeSoon(function() {
          self.finish();
        });
      }, false);
    namePicker.focus();
    EventUtils.synthesizeKey("n", {}, this.window);
    EventUtils.synthesizeKey("VK_RETURN", {}, this.window);
  },

  finish: function() {
    
    toggleSidebar("viewBookmarksSidebar", false);
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
    var tags = PlacesUtils.tagging.getTagsForURI(PlacesUtils._uri(TEST_URL), {});
    is(tags[0], "testTag", "Correctly added a tag");
  },

  selectNode: function(tree) {
    tree.selectItems([this._itemId]);
    is(tree.selectedNode.itemId, this._itemId, "Bookmark has been selected");
  },

  run: function() {
    
    var tagsField = this.window.document.getElementById("editBMPanel_tagsField");
    var self = this;
    tagsField.popup.addEventListener("popupshown", function (aEvent) {
        tagsField.popup.removeEventListener("popupshown", arguments.callee, true);
        tagsField.popup.focus();
        EventUtils.synthesizeKey("VK_ESCAPE", {}, self.window);
      }, true);
    tagsField.popup.addEventListener("popuphidden", function (aEvent) {
        tagsField.popup.removeEventListener("popuphidden", arguments.callee, true);
        self.finish();
      }, true);
    tagsField.focus();
    tagsField.value = "";
    EventUtils.synthesizeKey("t", {}, this.window);
  },

  finish: function() {
    isnot(this.window, null, "Window is still open");
    this.window.document.documentElement.cancelDialog();
    toggleSidebar("viewBookmarksSidebar", false);
    runNextTest();
  },

  cleanup: function() {
    
    var tags = PlacesUtils.tagging.getTagsForURI(PlacesUtils._uri(TEST_URL), {});
    is(tags[0], "testTag", "Tag on node has not changed");

    
    PlacesUtils.tagging.untagURI(PlacesUtils._uri(TEST_URL),
                                 ["testTag"]);
    PlacesUtils.bookmarks.removeItem(this._itemId);
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
    ok(true, "*** FINISHED TEST ***");
  }

  if (gTests.length > 0) {
    
    gCurrentTest = gTests.shift();
    ok(true, "*** TEST: " + gCurrentTest.desc);
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
      sidebar.focus();
      
      executeSoon(open_properties_dialog);
    }, true);
    toggleSidebar("viewBookmarksSidebar", true);
}

function open_properties_dialog() {
    var sidebar = document.getElementById("sidebar");
    
    var tree = sidebar.contentDocument.getElementById(gCurrentTest.sidebar);
    ok(tree, "Sidebar tree has been loaded");
    
    gCurrentTest.selectNode(tree);
    ok(tree.selectedNode,
       "We have a places node selected: " + tree.selectedNode.title);

    
    var windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowopened") {
          ww.unregisterNotification(this);
          var win = aSubject.QueryInterface(Ci.nsIDOMWindow);
          win.addEventListener("load", function onLoad(event) {
            win.removeEventListener("load", onLoad, false);
            
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
        if (gCurrentTest.itemType == TYPE_FOLDER)
          command = "placesCmd_new:folder";
        else if (gCurrentTest.itemType == TYPE_BOOKMARK)
          command = "placesCmd_new:bookmark";
        else
          ok(false, "You didn't set a valid itemType for adding an item");
        break;
      default:
        ok(false, "You didn't set a valid action for this test");
    }
    
    ok(tree.controller.isCommandEnabled(command),
       "Properties command on current selected node is enabled");

    
    tree.controller.doCommand(command);
}
