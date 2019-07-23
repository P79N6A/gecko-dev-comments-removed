




































const TEST_URL = "http://www.mozilla.org";
const TEST_TITLE = "example_title";

var gBookmarksToolbar = window.document.getElementById("bookmarksBarContent");
var dragDirections = { LEFT: 0, UP: 1, RIGHT: 2, DOWN: 3 };


















function synthesizeDragWithDirection(aElement, aExpectedDragData, aDirection) {
  var trapped = false;

  
  var trapDrag = function(event) {
    trapped = true;
    var dataTransfer = event.dataTransfer;
    is(dataTransfer.mozItemCount, aExpectedDragData.length,
       "Number of dragged items should be the same.");

    for (var t = 0; t < dataTransfer.mozItemCount; t++) {
      var types = dataTransfer.mozTypesAt(t);
      var expecteditem = aExpectedDragData[t];
      is(types.length, expecteditem.length,
        "Number of flavors for item " + t + " should be the same.");

      for (var f = 0; f < types.length; f++) {
        is(types[f], expecteditem[f].substring(0, types[f].length),
           "Flavor " + types[f] + " for item " + t + " should be the same.");
        is(dataTransfer.mozGetDataAt(types[f], t),
           expecteditem[f].substring(types[f].length + 2),
           "Contents for item " + t + " with flavor " + types[f] + " should be the same.");
      }
    }

    if (!aExpectedDragData.length)
      ok(event.getPreventDefault(), "Drag has been canceled.");

    event.preventDefault();
    event.stopPropagation();
  }

  var prevent = function(aEvent) {aEvent.preventDefault();}

  var xIncrement = 0;
  var yIncrement = 0;

  switch (aDirection) {
    case dragDirections.LEFT:
      xIncrement = -1;
      break;
    case dragDirections.RIGHT:
      xIncrement = +1;
      break;
    case dragDirections.UP:
      yIncrement = -1;
      break;
    case dragDirections.DOWN:
      yIncrement = +1;
      break;
  }

  var rect = aElement.getBoundingClientRect();
  var startingPoint = { x: (rect.right - rect.left)/2,
                        y: (rect.bottom - rect.top)/2 };

  EventUtils.synthesizeMouse(aElement,
                             startingPoint.x,
                             startingPoint.y,
                             { type: "mousedown" });
  EventUtils.synthesizeMouse(aElement,
                             startingPoint.x + xIncrement * 1,
                             startingPoint.y + yIncrement * 1,
                             { type: "mousemove" });
  gBookmarksToolbar.addEventListener("dragstart", trapDrag, false);
  EventUtils.synthesizeMouse(aElement,
                             startingPoint.x + xIncrement * 9,
                             startingPoint.y + yIncrement * 9,
                             { type: "mousemove" });
  ok(trapped, "A dragstart event has been trapped.");
  gBookmarksToolbar.removeEventListener("dragstart", trapDrag, false);

  
  
  aElement.addEventListener("click", prevent, false);
  EventUtils.synthesizeMouse(aElement,
                             startingPoint.x + xIncrement * 9,
                             startingPoint.y + yIncrement * 9,
                             { type: "mouseup" });
  aElement.removeEventListener("click", prevent, false);

  
  if (aElement.localName == "menu" && aElement.open)
    aElement.open = false;
}

function getToolbarNodeForItemId(aItemId) {
  var children = gBookmarksToolbar.childNodes;
  var node = null;
  for (var i = 0; i < children.length; i++) {
    if (aItemId == children[i].node.itemId) {
      node = children[i];
      break;
    }
  }
  return node;
}

function getExpectedDataForPlacesNode(aNode) {
  var wrappedNode = [];
  var flavors = ["text/x-moz-place",
                 "text/x-moz-url",
                 "text/plain",
                 "text/html"];

  flavors.forEach(function(aFlavor) {
    var wrappedFlavor = aFlavor + ": " +
                        PlacesUtils.wrapNode(aNode, aFlavor);
    wrappedNode.push(wrappedFlavor);
  });

  return [wrappedNode];
}

var gTests = [



  {
    desc: "Drag a folder on toolbar",
    run: function() {
      
      var folderId = PlacesUtils.bookmarks
                                .createFolder(PlacesUtils.toolbarFolderId,
                                              TEST_TITLE,
                                              PlacesUtils.bookmarks.DEFAULT_INDEX);
      var element = getToolbarNodeForItemId(folderId);
      isnot(element, null, "Found node on toolbar");

      isnot(element.node, null, "Toolbar node has an associated Places node.");
      var expectedData = getExpectedDataForPlacesNode(element.node);

      ok(true, "Dragging left");
      synthesizeDragWithDirection(element, expectedData, dragDirections.LEFT);
      ok(true, "Dragging right");
      synthesizeDragWithDirection(element, expectedData, dragDirections.RIGHT);
      ok(true, "Dragging up");
      synthesizeDragWithDirection(element, expectedData, dragDirections.UP);
      ok(true, "Dragging down");
      synthesizeDragWithDirection(element, new Array(), dragDirections.DOWN);

      
      PlacesUtils.bookmarks.removeItem(folderId);
    }
  },



  {
    desc: "Drag a bookmark on toolbar",
    run: function() {
      
      var itemId = PlacesUtils.bookmarks
                              .insertBookmark(PlacesUtils.toolbarFolderId,
                                              PlacesUtils._uri(TEST_URL),
                                              PlacesUtils.bookmarks.DEFAULT_INDEX,
                                              TEST_TITLE);
      var element = getToolbarNodeForItemId(itemId);
      isnot(element, null, "Found node on toolbar");

      isnot(element.node, null, "Toolbar node has an associated Places node.");
      var expectedData = getExpectedDataForPlacesNode(element.node);

      ok(true, "Dragging left");
      synthesizeDragWithDirection(element, expectedData, dragDirections.LEFT);
      ok(true, "Dragging right");
      synthesizeDragWithDirection(element, expectedData, dragDirections.RIGHT);
      ok(true, "Dragging up");
      synthesizeDragWithDirection(element, expectedData, dragDirections.UP);
      ok(true, "Dragging down");
      synthesizeDragWithDirection(element, expectedData, dragDirections.DOWN);

      
      PlacesUtils.bookmarks.removeItem(itemId);
    }
  },
];

function nextTest() {
  if (gTests.length) {
    var test = gTests.shift();
    info("Start of test: " + test.desc);
    test.run();

    setTimeout(nextTest, 0);
  }
  else
    finish();
}

function test() {
  waitForExplicitFinish();

  nextTest();
}

