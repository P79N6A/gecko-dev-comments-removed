


function test() {
  waitForExplicitFinish();

  newWindowWithTabView(onTabViewWindowLoaded);
}

function onTabViewWindowLoaded(win) {
  ok(win.TabView.isVisible(), "Tab View is visible");

  let contentWindow = win.document.getElementById("tab-view").contentWindow;
  let [originalTab] = win.gBrowser.visibleTabs;

  let currentGroup = contentWindow.GroupItems.getActiveGroupItem();

  
  let box = new contentWindow.Rect(100, 100, 400, 430);
  let group = new contentWindow.GroupItem([], { bounds: box });
  ok(group.isEmpty(), "This group is empty");
  contentWindow.UI.setActive(group);
  
  
  let tabs = [];
  tabs.push(win.gBrowser.loadOneTab("about:blank#0", {inBackground: true}));
  tabs.push(win.gBrowser.loadOneTab("about:blank#1", {inBackground: true}));
  tabs.push(win.gBrowser.loadOneTab("about:blank#2", {inBackground: true}));
  tabs.push(win.gBrowser.loadOneTab("about:blank#3", {inBackground: true}));
  tabs.push(win.gBrowser.loadOneTab("about:blank#4", {inBackground: true}));
  tabs.push(win.gBrowser.loadOneTab("about:blank#5", {inBackground: true}));
  tabs.push(win.gBrowser.loadOneTab("about:blank#6", {inBackground: true}));

  ok(!group.shouldStack(group._children.length), "Group should not stack.");
  is(group._columns, 3, "There should be three columns.");
  
  
  let currentTarget = tabs[6]._tabViewTabItem;
  let currentPos = currentTarget.getBounds().center();
  let targetPos = tabs[2]._tabViewTabItem.getBounds().center();
  let vector = new contentWindow.Point(targetPos.x - currentPos.x,
                                       targetPos.y - currentPos.y);
  checkDropIndexAndDropSpace(currentTarget, group, vector.x, vector.y, contentWindow,
                             function(index, dropSpaceActiveValues) {
    
    is(index, 2, "Tab 6 is now in the third position");
    is(dropSpaceActiveValues[0], true, "dropSpace was always showing");

    
    let currentTarget = tabs[1]._tabViewTabItem;
    let currentPos = currentTarget.getBounds().center();
    
    let groupBounds = group.getBounds();
    let bottomPos = new contentWindow.Point(
                      (groupBounds.left + groupBounds.right) / 2,
                      groupBounds.bottom - 15);
    let vector = new contentWindow.Point(bottomPos.x - currentPos.x,
                                         bottomPos.y - currentPos.y);
    checkDropIndexAndDropSpace(currentTarget, group, vector.x, vector.y, contentWindow,
                               function(index, dropSpaceActiveValues) {
      
      is(index, 6, "Tab 1 is now at the end of the group");
      is(dropSpaceActiveValues[0], true, "dropSpace was always showing");
    
      
      
      let currentTarget = tabs[4]._tabViewTabItem;
      let currentPos = currentTarget.getBounds().center();
      
      let belowPos = new contentWindow.Point(
                        (groupBounds.left + groupBounds.right) / 2,
                        groupBounds.bottom + 300);
      let vector = new contentWindow.Point(belowPos.x - currentPos.x,
                                           belowPos.y - currentPos.y);
      checkDropIndexAndDropSpace(currentTarget, group, vector.x, vector.y, contentWindow,
                                 function(index, dropSpaceActiveValues) {
        
        is(index, -1, "Tab 5 is no longer in the group");
        contentWindow.Utils.log('dropSpaceActiveValues',dropSpaceActiveValues);
        is(dropSpaceActiveValues[0], true, "The group began by showing a dropSpace");
        is(dropSpaceActiveValues[dropSpaceActiveValues.length - 1], false, "In the end, the group was not showing a dropSpace");
        
        
        
        setTimeout(function() {
          
          let currentTarget = tabs[4]._tabViewTabItem;
          let currentPos = currentTarget.getBounds().center();
          let targetPos = tabs[5]._tabViewTabItem.getBounds().center();
          
          vector = new contentWindow.Point(targetPos.x - currentPos.x,
                                               targetPos.y - currentPos.y);
          
          checkDropIndexAndDropSpace(currentTarget, group, vector.x, vector.y, contentWindow,
                                     function(index, dropSpaceActiveValues) {

            is(group._columns, 3, "There should be three columns.");

            
            is(index, 4, "Tab 5 is back and again the fifth tab.");
            contentWindow.Utils.log('dropSpaceActiveValues',dropSpaceActiveValues);
            is(dropSpaceActiveValues[0], false, "The group began by not showing a dropSpace");
            is(dropSpaceActiveValues[dropSpaceActiveValues.length - 1], true, "In the end, the group was showing a dropSpace");
            
            
            win.close();
            finish();
          }, 6000, false);
        },1000);
        
      });
    
    });

  });
}

function simulateSlowDragDrop(srcElement, offsetX, offsetY, contentWindow, time) {
  
  let dataTransfer;

  let bounds = srcElement.getBoundingClientRect();

  EventUtils.synthesizeMouse(
    srcElement, 2, 2, { type: "mousedown" }, contentWindow);
  let event = contentWindow.document.createEvent("DragEvents");
  event.initDragEvent(
    "dragenter", true, true, contentWindow, 0, 0, 0, 0, 0,
    false, false, false, false, 1, null, dataTransfer);
  srcElement.dispatchEvent(event);
  
  let steps = 20;
  
  
  let moveIncremental = function moveIncremental(i, steps) {
    
    let offsetXDiff = Math.round(i * offsetX / steps) - Math.round((i - 1) * offsetX / steps);
    let offsetYDiff = Math.round(i * offsetY / steps) - Math.round((i - 1) * offsetY / steps);
    
    
    EventUtils.synthesizeMouse(
      srcElement, offsetXDiff + 2, offsetYDiff + 2,
      { type: "mousemove" }, contentWindow);
    
    let event = contentWindow.document.createEvent("DragEvents");
    event.initDragEvent(
      "dragover", true, true, contentWindow, 0, 0, 0, 0, 0,
      false, false, false, false, 0, null, dataTransfer);
    srcElement.dispatchEvent(event);
    let bounds = srcElement.getBoundingClientRect();
    
  };
  for (let i = 1; i <= steps; i++)
    setTimeout(moveIncremental, i / (steps + 1) * time, i, steps);

  
  let finalDrop = function finalDrop() {
    EventUtils.synthesizeMouseAtCenter(srcElement, { type: "mouseup" }, contentWindow);
    event = contentWindow.document.createEvent("DragEvents");
    event.initDragEvent(
      "drop", true, true, contentWindow, 0, 0, 0, 0, 0,
      false, false, false, false, 0, null, dataTransfer);
    srcElement.dispatchEvent(event);
    contentWindow.iQ(srcElement).css({border: 'green 1px solid'});
  }
  setTimeout(finalDrop, time);
}

function checkDropIndexAndDropSpace(item, group, offsetX, offsetY, contentWindow, callback, time) {
  contentWindow.UI.setActive(item);
  let dropSpaceActiveValues = [];
  let recordDropSpaceValue = function() {
    dropSpaceActiveValues.push(group._dropSpaceActive);
  };

  let onDrop = function() {
    item.container.removeEventListener('dragover', recordDropSpaceValue, false);
    item.container.removeEventListener('drop', onDrop, false);
    let index = group._children.indexOf(item);
    callback(index, dropSpaceActiveValues);
  };
  item.container.addEventListener('dragover', recordDropSpaceValue, false);
  item.container.addEventListener('drop', onDrop, false);
  simulateSlowDragDrop(item.container, offsetX, offsetY, contentWindow, time || 1000);
}
