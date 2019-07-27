



function add_bookmark(aURI) {
  return PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                              aURI, PlacesUtils.bookmarks.DEFAULT_INDEX,
                                              "bookmark/" + aURI.spec);
}

Components.utils.import("resource://gre/modules/NetUtil.jsm");

const TEST_URL = "http://example.com/";
const MOZURISPEC = "http://mozilla.com/";

let gLibrary;
let PlacesOrganizer;
let ContentTree;

function test() {
  waitForExplicitFinish();
  gLibrary = openLibrary(onLibraryReady);
}

function onLibraryReady() {
  ok(PlacesUtils, "PlacesUtils in scope");
  ok(PlacesUIUtils, "PlacesUIUtils in scope");

  PlacesOrganizer = gLibrary.PlacesOrganizer;
  ok(PlacesOrganizer, "Places organizer in scope");

  ContentTree = gLibrary.ContentTree;
  ok(ContentTree, "ContentTree is in scope");

  tests.makeHistVisit(function() {
    tests.makeTag();
    tests.focusTag();
    waitForClipboard(function(aData) !!aData,
                     tests.copyHistNode,
                     onClipboardReady,
                     PlacesUtils.TYPE_X_MOZ_PLACE);
  });
}

function onClipboardReady() {
  tests.focusTag();
  PlacesOrganizer._places.controller.paste();
  tests.historyNode();
  tests.checkForBookmarkInUI();

  gLibrary.close();

  
  PlacesUtils.tagging.untagURI(NetUtil.newURI(MOZURISPEC), ["foo"]);
  PlacesUtils.tagging.untagURI(NetUtil.newURI(TEST_URL), ["foo"]);
  let tags = PlacesUtils.tagging.getTagsForURI(NetUtil.newURI(TEST_URL));
  is(tags.length, 0, "tags are gone");
  PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

  PlacesTestUtils.clearHistory().then(finish);
}

let tests = {

  makeHistVisit: function(aCallback) {
    
    let testURI1 = NetUtil.newURI(MOZURISPEC);
    isnot(testURI1, null, "testURI is not null");
    PlacesTestUtils.addVisits(
      {uri: testURI1, transition: PlacesUtils.history.TRANSITION_TYPED}
      ).then(aCallback);
  },

  makeTag: function() {
    
    let bmId = add_bookmark(NetUtil.newURI(TEST_URL));
    ok(bmId > 0, "A bookmark was added");
    PlacesUtils.tagging.tagURI(NetUtil.newURI(TEST_URL), ["foo"]);
    let tags = PlacesUtils.tagging.getTagsForURI(NetUtil.newURI(TEST_URL));
    is(tags[0], 'foo', "tag is foo");
  },

  focusTag: function (){
    
    PlacesOrganizer.selectLeftPaneQuery("Tags");
    let tags = PlacesOrganizer._places.selectedNode;
    tags.containerOpen = true;
    let fooTag = tags.getChild(0);
    let tagNode = fooTag;
    PlacesOrganizer._places.selectNode(fooTag);
    is(tagNode.title, 'foo', "tagNode title is foo");
    let ip = PlacesOrganizer._places.insertionPoint;
    ok(ip.isTag, "IP is a tag");
  },

  copyHistNode: function (){
    
    PlacesOrganizer.selectLeftPaneQuery("History");
    let histContainer = PlacesOrganizer._places.selectedNode;
    PlacesUtils.asContainer(histContainer);
    histContainer.containerOpen = true;
    PlacesOrganizer._places.selectNode(histContainer.getChild(0));
    let histNode = ContentTree.view.view.nodeForTreeIndex(0);
    ContentTree.view.selectNode(histNode);
    is(histNode.uri, MOZURISPEC,
       "historyNode exists: " + histNode.uri);
    
    ContentTree.view.controller.copy();
  },

  historyNode: function (){
    
    PlacesOrganizer.selectLeftPaneQuery("History");
    let histContainer = PlacesOrganizer._places.selectedNode;
    PlacesUtils.asContainer(histContainer);
    histContainer.containerOpen = true;
    PlacesOrganizer._places.selectNode(histContainer.getChild(0));
    let histNode = ContentTree.view.view.nodeForTreeIndex(0);
    ok(histNode, "histNode exists: " + histNode.title);
    
    let tags = PlacesUtils.tagging.getTagsForURI(NetUtil.newURI(MOZURISPEC));
    ok(tags.length == 1, "history node is tagged: " + tags.length);
    
    let isBookmarked = PlacesUtils.bookmarks.isBookmarked(NetUtil.newURI(MOZURISPEC));
    is(isBookmarked, true, MOZURISPEC + " is bookmarked");
    let bookmarkIds = PlacesUtils.bookmarks.getBookmarkIdsForURI(
                        NetUtil.newURI(histNode.uri));
    ok(bookmarkIds.length > 0, "bookmark exists for the tagged history item: " + bookmarkIds);
  },

  checkForBookmarkInUI: function(){
    
    
    PlacesOrganizer.selectLeftPaneQuery("UnfiledBookmarks");
    
    let unsortedNode = ContentTree.view.view.nodeForTreeIndex(1);
    ok(unsortedNode, "unsortedNode is not null: " + unsortedNode.uri);
    is(unsortedNode.uri, MOZURISPEC, "node uri's are the same");
  },
};
