let doc;

function test() {
  waitForExplicitFinish();
  Task.spawn(function(){
    info(chromeRoot + "browser_tilegrid.xul");
    yield addTab(chromeRoot + "browser_tilegrid.xul");
    doc = Browser.selectedTab.browser.contentWindow.document;
  }).then(runTests);
}

gTests.push({
  desc: "richgrid binding is applied",
  run: function() {
    ok(doc, "doc got defined");

    let grid = doc.querySelector("#grid1");
    ok(grid, "#grid1 is found");
    is(typeof grid.clearSelection, "function", "#grid1 has the binding applied");

    is(grid.items.length, 2, "#grid1 has a 2 items");
    is(grid.items[0].control, grid, "#grid1 item's control points back at #grid1'");
  }
});

gTests.push({
  desc: "item clicks are handled",
  run: function() {
    let grid = doc.querySelector("#grid1");
    is(typeof grid.handleItemClick, "function", "grid.handleItemClick is a function");
    let handleStub = stubMethod(grid, 'handleItemClick');
    let itemId = "grid1_item1"; 

    
    EventUtils.sendMouseEvent({type: 'click'}, itemId, doc.defaultView);
    yield waitForMs(0);

    is(handleStub.callCount, 1, "handleItemClick was called when we clicked an item");
    handleStub.restore();

    
    let gridController = {
      handleItemClick: function() {}
    };
    let controllerHandleStub = stubMethod(gridController, "handleItemClick");
    let origController = grid.controller;
    grid.controller = gridController;

    
    EventUtils.sendMouseEvent({type: 'click'}, itemId, doc.defaultView);
    yield waitForMs(0);

    is(controllerHandleStub.callCount, 1, "controller.handleItemClick was called when we clicked an item");
    is(controllerHandleStub.calledWith[0], doc.getElementById(itemId), "controller.handleItemClick was passed the grid item");
    grid.controller = origController;
  }
});

gTests.push({
  desc: "arrangeItems",
  run: function() {
     
    let container = doc.getElementById("alayout");
    let grid = doc.querySelector("#grid_layout");

    is(typeof grid.arrangeItems, "function", "arrangeItems is a function on the grid");

    ok(grid.tileHeight, "grid has truthy tileHeight value");
    ok(grid.tileWidth, "grid has truthy tileWidth value");

    
    container.style.height = 3 * grid.tileHeight + 20 + "px";

    
    grid.appendItem("test title", "about:blank", true);
    grid.appendItem("test title", "about:blank", true);
    grid.appendItem("test title", "about:blank", true);
    grid.appendItem("test title", "about:blank", true);
    grid.appendItem("test title", "about:blank", true);

    grid.arrangeItems();
    
    is(grid.rowCount, 3, "rowCount is calculated correctly for a given container height and tileheight");
    is(grid.columnCount, 2, "columnCount is calculated correctly for a given container maxWidth and tilewidth");

    
    

    let under3rowsHeight = (3 * grid.tileHeight -20) + "px";
    container.style.height = under3rowsHeight;

    let arrangedPromise = waitForEvent(grid, "arranged");
    grid.arrangeItems();
    yield arrangedPromise;

    ok(true, "arranged event is fired when arrangeItems is called");
    is(grid.rowCount, 2, "rowCount is re-calculated correctly for a given container height");
  }
});

gTests.push({
  desc: "clearAll",
  run: function() {
    let grid = doc.getElementById("clearGrid");
    grid.arrangeItems();

    
    is(typeof grid.clearAll, "function", "clearAll is a function on the grid");
    is(grid.itemCount, 3, "grid has 3 items initially");
    is(grid.rowCount, 2, "grid has 2 rows initially");
    is(grid.columnCount, 2, "grid has 2 cols initially");

    let arrangeSpy = spyOnMethod(grid, "arrangeItems");
    grid.clearAll();

    is(grid.itemCount, 0, "grid has 0 itemCount after clearAll");
    is(grid.items.length, 0, "grid has 0 items after clearAll");
    

    is(arrangeSpy.callCount, 1, "arrangeItems is called once when we clearAll");
    arrangeSpy.restore();
  }
});

gTests.push({
  desc: "empty grid",
  run: function() {
    let grid = doc.getElementById("emptyGrid");
    grid.arrangeItems();
    yield waitForCondition(() => !grid.isArranging);

    
    ok(grid.isBound, "binding was applied");
    is(grid.itemCount, 0, "empty grid has 0 items");
    is(grid.rowCount, 0, "empty grid has 0 rows");
    is(grid.columnCount, 0, "empty grid has 0 cols");

    let columnsNode = grid._grid;
    let cStyle = doc.defaultView.getComputedStyle(columnsNode);
    is(cStyle.getPropertyValue("-moz-column-count"), "auto", "empty grid has -moz-column-count: auto");
  }
});

gTests.push({
  desc: "appendItem",
  run: function() {
     
     
    let grid = doc.querySelector("#emptygrid");

    is(grid.itemCount, 0, "0 itemCount when empty");
    is(grid.items.length, 0, "0 items when empty");
    is(typeof grid.appendItem, "function", "appendItem is a function on the grid");

    let arrangeStub = stubMethod(grid, "arrangeItems");
    let newItem = grid.appendItem("test title", "about:blank");

    ok(newItem && grid.items[0]==newItem, "appendItem gives back the item");
    is(grid.itemCount, 1, "itemCount is incremented when we appendItem");
    is(newItem.getAttribute("label"), "test title", "title ends up on label attribute");
    is(newItem.getAttribute("value"), "about:blank", "url ends up on value attribute");

    is(arrangeStub.callCount, 1, "arrangeItems is called when we appendItem");
    arrangeStub.restore();
  }
});

gTests.push({
  desc: "getItemAtIndex",
  run: function() {
     
    let grid = doc.querySelector("#grid2");
    is(typeof grid.getItemAtIndex, "function", "getItemAtIndex is a function on the grid");
    is(grid.getItemAtIndex(0).getAttribute("id"), "grid2_item1", "getItemAtIndex retrieves the first item");
    is(grid.getItemAtIndex(1).getAttribute("id"), "grid2_item2", "getItemAtIndex item at index 2");
    ok(!grid.getItemAtIndex(5), "getItemAtIndex out-of-bounds index returns falsy");
  }
});

gTests.push({
  desc: "removeItemAt",
  run: function() {
     
     
    let grid = doc.querySelector("#grid2");

    is(grid.itemCount, 2, "2 items initially");
    is(typeof grid.removeItemAt, "function", "removeItemAt is a function on the grid");

    let arrangeStub = stubMethod(grid, "arrangeItems");
    let removedItem = grid.removeItemAt(0);

    ok(removedItem, "removeItemAt gives back an item");
    is(removedItem.getAttribute("id"), "grid2_item1", "removeItemAt gives back the correct item");
    is(grid.items[0].getAttribute("id"), "grid2_item2", "2nd item becomes the first item");
    is(grid.itemCount, 1, "itemCount is decremented when we removeItemAt");

    is(arrangeStub.callCount, 1, "arrangeItems is called when we removeItemAt");
    arrangeStub.restore();
  }
});

gTests.push({
  desc: "insertItemAt",
  run: function() {
     
     
    let grid = doc.querySelector("#grid3");

    is(grid.itemCount, 2, "2 items initially");
    is(typeof grid.insertItemAt, "function", "insertItemAt is a function on the grid");

    let arrangeStub = stubMethod(grid, "arrangeItems");
    let insertedItem = grid.insertItemAt(1, "inserted item", "http://example.com/inserted");

    ok(insertedItem, "insertItemAt gives back an item");
    is(grid.items[1], insertedItem, "item is inserted at the correct index");
    is(insertedItem.getAttribute("label"), "inserted item", "insertItemAt creates item with the correct label");
    is(insertedItem.getAttribute("value"), "http://example.com/inserted", "insertItemAt creates item with the correct url value");
    is(grid.items[2].getAttribute("id"), "grid3_item2", "following item ends up at the correct index");
    is(grid.itemCount, 3, "itemCount is incremented when we insertItemAt");

    is(arrangeStub.callCount, 1, "arrangeItems is called when we insertItemAt");
    arrangeStub.restore();
  }
});

gTests.push({
  desc: "getIndexOfItem",
  run: function() {
     
     
    let grid = doc.querySelector("#grid4");

    is(grid.itemCount, 2, "2 items initially");
    is(typeof grid.getIndexOfItem, "function", "getIndexOfItem is a function on the grid");

    let item = doc.getElementById("grid4_item2");
    let badItem = doc.createElement("richgriditem");

    is(grid.getIndexOfItem(item), 1, "getIndexOfItem returns the correct value for an item");
    is(grid.getIndexOfItem(badItem), -1, "getIndexOfItem returns -1 for items it doesn't contain");
  }
});

gTests.push({
  desc: "getItemsByUrl",
  run: function() {
    let grid = doc.querySelector("#grid5");

    is(grid.itemCount, 4, "4 items total");
    is(typeof grid.getItemsByUrl, "function", "getItemsByUrl is a function on the grid");

    ['about:blank', 'http://bugzilla.mozilla.org/'].forEach(function(testUrl) {
      let items = grid.getItemsByUrl(testUrl);
      is(items.length, 2, "2 matching items in the test grid");
      is(items.item(0).url, testUrl, "Matched item has correct url property");
      is(items.item(1).url, testUrl, "Matched item has correct url property");
    });

    let badUrl = 'http://gopher.well.com:70/';
    let items = grid.getItemsByUrl(badUrl);
    is(items.length, 0, "0 items matched url: "+badUrl);

  }
});

gTests.push({
  desc: "removeItem",
  run: function() {
    let grid = doc.querySelector("#grid5");

    is(grid.itemCount, 4, "4 items total");
    is(typeof grid.removeItem, "function", "removeItem is a function on the grid");

    let arrangeStub = stubMethod(grid, "arrangeItems");
    let removedFirst = grid.removeItem( grid.items[0] );

    is(arrangeStub.callCount, 1, "arrangeItems is called when we removeItem");

    let removed2nd = grid.removeItem( grid.items[0], true);
    is(removed2nd.getAttribute("label"), "2nd item", "the next item was returned");
    is(grid.itemCount, 2, "2 items remain");

    
    is(arrangeStub.callCount, 1, "arrangeItems is not called when we pass the truthy skipArrange param");

    let otherItem = grid.ownerDocument.querySelector("#grid6_item1");
    let removedFail = grid.removeItem(otherItem);
    ok(!removedFail, "Falsy value returned when non-child item passed");
    is(grid.itemCount, 2, "2 items remain");

    
    is(arrangeStub.callCount, 1, "arrangeItems is not called when nothing is matched");

    arrangeStub.restore();
  }
});

gTests.push({
  desc: "selections (single)",
  run: function() {
     
     
     
     
     
    let grid = doc.querySelector("#grid-select1");

    is(typeof grid.clearSelection, "function", "clearSelection is a function on the grid");
    is(typeof grid.selectedItems, "object", "selectedItems is a property on the grid");
    is(typeof grid.toggleItemSelection, "function", "toggleItemSelection is function on the grid");
    is(typeof grid.selectItem, "function", "selectItem is a function on the grid");

    is(grid.itemCount, 2, "2 items initially");
    is(grid.selectedItems.length, 0, "nothing selected initially");

    grid.toggleItemSelection(grid.items[1]);
    ok(grid.items[1].selected, "toggleItemSelection sets truthy selected prop on previously-unselected item");
    is(grid.selectedIndex, 1, "selectedIndex is correct");

    grid.toggleItemSelection(grid.items[1]);
    ok(!grid.items[1].selected, "toggleItemSelection sets falsy selected prop on previously-selected item");
    is(grid.selectedIndex, -1, "selectedIndex reports correctly with nothing selected");

    
    grid.selectItem(grid.items[1]);
    ok(grid.items[1].selected, "Item selected property is truthy after grid.selectItem");
    ok(grid.items[1].getAttribute("selected"), "Item selected attribute is truthy after grid.selectItem");
    ok(grid.selectedItems.length, "There are selectedItems after grid.selectItem");

    
    grid.selectItem(grid.items[0]);
    grid.selectItem(grid.items[1]);
    grid.clearSelection();
    is(grid.selectedItems.length, 0, "Nothing selected when we clearSelection");
    is(grid.selectedIndex, -1, "selectedIndex resets after clearSelection");

    
    
    
    let handler = {
      handleEvent: function(aEvent) {}
    };
    let handlerStub = stubMethod(handler, "handleEvent");
    doc.defaultView.addEventListener("select", handler, false);
    info("select listener added");

    info("calling selectItem, currently it is:" + grid.items[0].selected);
    
    grid.selectItem(grid.items[0]);
    info("calling selectItem, now it is:" + grid.items[0].selected);
    yield waitForMs(0);

    is(handlerStub.callCount, 1, "select event handler was called when we selected an item");
    is(handlerStub.calledWith[0].type, "select", "handler got a select event");
    is(handlerStub.calledWith[0].target, grid, "select event had the originating grid as the target");
    handlerStub.restore();
    doc.defaultView.removeEventListener("select", handler, false);
  }
});

gTests.push({
  desc: "selections (multiple)",
  run: function() {
     
     
     
     
    let grid = doc.querySelector("#grid-select2");

    is(typeof grid.clearSelection, "function", "clearSelection is a function on the grid");
    is(typeof grid.selectedItems, "object", "selectedItems is a property on the grid");
    is(typeof grid.toggleItemSelection, "function", "toggleItemSelection is function on the grid");
    is(typeof grid.selectItem, "function", "selectItem is a function on the grid");

    is(grid.itemCount, 2, "2 items initially");
    is(grid.selectedItems.length, 0, "nothing selected initially");

    grid.toggleItemSelection(grid.items[1]);
    ok(grid.items[1].selected, "toggleItemSelection sets truthy selected prop on previously-unselected item");
    is(grid.selectedItems.length, 1, "1 item selected when we first toggleItemSelection");
    is(grid.selectedItems[0], grid.items[1], "the right item is selected");
    is(grid.selectedIndex, 1, "selectedIndex is correct");

    grid.toggleItemSelection(grid.items[1]);
    is(grid.selectedItems.length, 0, "Nothing selected when we toggleItemSelection again");

    
    grid.items[0].selected=true;
    grid.items[1].selected=true;
    is(grid.selectedItems.length, 2, "Both items are selected before calling clearSelection");
    grid.clearSelection();
    is(grid.selectedItems.length, 0, "Nothing selected when we clearSelection");
    ok(!(grid.items[0].selected || grid.items[1].selected), "selected properties all falsy when we clearSelection");

    
    
    
    let handler = {
      handleEvent: function(aEvent) {}
    };
    let handlerStub = stubMethod(handler, "handleEvent");
    doc.defaultView.addEventListener("selectionchange", handler, false);
    info("selectionchange listener added");

    info("calling toggleItemSelection, currently it is:" + grid.items[0].selected);
    
    grid.toggleItemSelection(grid.items[0]);
    info("/calling toggleItemSelection, now it is:" + grid.items[0].selected);
    yield waitForMs(0);

    is(handlerStub.callCount, 1, "selectionchange event handler was called when we selected an item");
    is(handlerStub.calledWith[0].type, "selectionchange", "handler got a selectionchange event");
    is(handlerStub.calledWith[0].target, grid, "select event had the originating grid as the target");
    handlerStub.restore();
    doc.defaultView.removeEventListener("selectionchange", handler, false);
  }
});
