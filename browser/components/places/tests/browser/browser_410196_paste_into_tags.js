





































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var gh = hs.QueryInterface(Ci.nsIGlobalHistory2);
var bh = hs.QueryInterface(Ci.nsIBrowserHistory);
var ts = Cc["@mozilla.org/browser/tagging-service;1"].
         getService(Components.interfaces.nsITaggingService);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);

function add_visit(aURI, aReferrer) {
  var visitId = hs.addVisit(aURI,
                            Date.now() * 1000,
                            aReferrer,
                            hs.TRANSITION_TYPED, 
                            false, 
                            0);
  return visitId;
}

function add_bookmark(aURI) {
  var bId = bs.insertBookmark(bs.unfiledBookmarksFolder, aURI,
                              bs.DEFAULT_INDEX, "bookmark/" + aURI.spec);
  return bId;
}

const TEST_URL = "http://example.com/";
const MOZURISPEC = "http://mozilla.com/";

function test() {
  waitForExplicitFinish();
  var win = window.openDialog("chrome://browser/content/places/places.xul",
                              "",
                              "chrome,toolbar=yes,dialog=no,resizable");

  win.addEventListener("load", function onload() {
    win.removeEventListener("load", onload, false);
    executeSoon(function () {
      var PU = win.PlacesUtils;
      var PO = win.PlacesOrganizer;
      var PUIU = win.PlacesUIUtils;

      
      var tests = {

        sanity: function(){
          
          ok(PU, "PlacesUtils in scope");
          ok(PUIU, "PlacesUIUtils in scope");
          ok(PO, "Places organizer in scope");
        },

        makeHistVisit: function() {
          
          var testURI1 = PU._uri(MOZURISPEC);
          isnot(testURI1, null, "testURI is not null");
          var visitId = add_visit(testURI1);
          ok(visitId > 0, "A visit was added to the history");
          ok(gh.isVisited(testURI1), MOZURISPEC + " is a visited url.");
        },

        makeTag: function() {
          
          var bmId = add_bookmark(PlacesUtils._uri(TEST_URL));
          ok(bmId > 0, "A bookmark was added");
          ts.tagURI(PlacesUtils._uri(TEST_URL), ["foo"]);
          var tags = ts.getTagsForURI(PU._uri(TEST_URL));
          is(tags[0], 'foo', "tag is foo");
        },

        focusTag: function (paste){
          
          PO.selectLeftPaneQuery("Tags");
          var tags = PO._places.selectedNode;
          tags.containerOpen = true;
          var fooTag = tags.getChild(0);
          this.tagNode = fooTag;
          PO._places.selectNode(fooTag);
          is(this.tagNode.title, 'foo', "tagNode title is foo");
          var ip = PO._places.insertionPoint;
          ok(ip.isTag, "IP is a tag");
          if (paste) {
            ok(true, "About to paste");
            PO._places.controller.paste();
          }
        },

        histNode: null,

        copyHistNode: function (){
          
          PO.selectLeftPaneQuery("History");
          var histContainer = PO._places.selectedNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
          histContainer.containerOpen = true;
          PO._places.selectNode(histContainer.getChild(0));
          this.histNode = PO._content.view.nodeForTreeIndex(0);
          PO._content.selectNode(this.histNode);
          is(this.histNode.uri, MOZURISPEC,
             "historyNode exists: " + this.histNode.uri);
          
          PO._content.controller.copy();
        },

        waitForPaste: function (){
          try {
            var xferable = Cc["@mozilla.org/widget/transferable;1"].
                           createInstance(Ci.nsITransferable);
            xferable.addDataFlavor(PU.TYPE_X_MOZ_PLACE);
            PUIU.clipboard.getData(xferable, Ci.nsIClipboard.kGlobalClipboard);
            var data = { }, type = { };
            xferable.getAnyTransferData(type, data, { });
            
            continue_test();
          } catch (ex) {
            
            setTimeout(tests.waitForPaste, 100);
          }
        },

        pasteToTag: function (){
          
          this.focusTag(true);
        },

        historyNode: function (){
          
          PO.selectLeftPaneQuery("History");
          var histContainer = PO._places.selectedNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
          histContainer.containerOpen = true;
          PO._places.selectNode(histContainer.getChild(0));
          var histNode = PO._content.view.nodeForTreeIndex(0);
          ok(histNode, "histNode exists: " + histNode.title);
          
          var tags = PU.tagging.getTagsForURI(PU._uri(MOZURISPEC));
          ok(tags.length == 1, "history node is tagged: " + tags.length);
          
          var isBookmarked = PU.bookmarks.isBookmarked(PU._uri(MOZURISPEC));
          is(isBookmarked, true, MOZURISPEC + " is bookmarked");
          var bookmarkIds = PU.bookmarks.getBookmarkIdsForURI(
                              PU._uri(histNode.uri));
          ok(bookmarkIds.length > 0, "bookmark exists for the tagged history item: " + bookmarkIds);
        },

        checkForBookmarkInUI: function(){
          
          
          PO.selectLeftPaneQuery("UnfiledBookmarks");
          
          var unsortedNode = PO._content.view.nodeForTreeIndex(1);
          ok(unsortedNode, "unsortedNode is not null: " + unsortedNode.uri);
          is(unsortedNode.uri, MOZURISPEC, "node uri's are the same");
        },

        tagNode: null,

        cleanUp: function(){
          ts.untagURI(PU._uri(MOZURISPEC), ["foo"]);
          ts.untagURI(PU._uri(TEST_URL), ["foo"]);
          hs.removeAllPages();
          var tags = ts.getTagsForURI(PU._uri(TEST_URL));
          is(tags.length, 0, "tags are gone");
          bs.removeFolderChildren(bs.unfiledBookmarksFolder);
        }
      };

      tests.sanity();
      tests.makeHistVisit();
      tests.makeTag();
      tests.focusTag();
      tests.copyHistNode();
      tests.waitForPaste();
      
      function continue_test() {
        tests.pasteToTag();
        tests.historyNode();
        tests.checkForBookmarkInUI();

        
        tests.cleanUp();

        win.close();
        finish();
      }

    });
  },false);
}
