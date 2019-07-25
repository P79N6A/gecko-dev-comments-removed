




































function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  if (TabView.isVisible())
    onTabViewWindowLoaded();
  else
    TabView.show();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  let contentWindow = document.getElementById("tab-view").contentWindow;
  let [originalTab] = gBrowser.visibleTabs;

  ok(TabView.isVisible(), "Tab View is visible");
  is(contentWindow.GroupItems.groupItems.length, 1, "There is only one group");
  let currentActiveGroup = contentWindow.GroupItems.getActiveGroupItem();




  let endGame = function() {
    let onTabViewHidden = function() {
      window.removeEventListener("tabviewhidden", onTabViewHidden, false);
      ok(!TabView.isVisible(), "TabView is shown");
      finish();
    };
    window.addEventListener("tabviewhidden", onTabViewHidden, false);

    ok(TabView.isVisible(), "TabView is shown");
    
    gBrowser.selectedTab = originalTab;
    TabView.hide();
  }
  
  let part1 = function() {

    
    checkSnap(currentActiveGroup, 0, 20, contentWindow, function(snapped){
      ok(!snapped,"Move away from the edge");

      
      checkSnap(currentActiveGroup, 0, 0, contentWindow, function(snapped){
        ok(!snapped,"Just pick it up and drop it");
        
        checkSnap(currentActiveGroup, 0, 1, contentWindow, function(snapped){
          ok(snapped,"Drag one pixel: should snap");
    
          checkSnap(currentActiveGroup, 0, 5, contentWindow, function(snapped){
            ok(!snapped,"Moving five pixels: shouldn't snap");
            endGame();
          });
        });
      });
    });
  }
  
  part1();
}

function simulateDragDrop(tabItem, offsetX, offsetY, contentWindow) {
  
  let dataTransfer;

  EventUtils.synthesizeMouse(
    tabItem.container, 1, 1, { type: "mousedown" }, contentWindow);
  event = contentWindow.document.createEvent("DragEvents");
  event.initDragEvent(
    "dragenter", true, true, contentWindow, 0, 0, 0, 0, 0,
    false, false, false, false, 1, null, dataTransfer);
  tabItem.container.dispatchEvent(event);

  
  if (offsetX || offsetY) {
    let Ci = Components.interfaces;
    let utils = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor).
                              getInterface(Ci.nsIDOMWindowUtils);
    let rect = tabItem.getBounds();
    for (let i = 1; i <= 5; i++) {
      let left = rect.left + 1 + Math.round(i * offsetX / 5);
      let top = rect.top + 1 + Math.round(i * offsetY / 5);
      utils.sendMouseEvent("mousemove", left, top, 0, 1, 0);
    }
    event = contentWindow.document.createEvent("DragEvents");
    event.initDragEvent(
      "dragover", true, true, contentWindow, 0, 0, 0, 0, 0,
      false, false, false, false, 0, null, dataTransfer);
    tabItem.container.dispatchEvent(event);
  }
  
  
  EventUtils.synthesizeMouse(
    tabItem.container, 0, 0, { type: "mouseup" }, contentWindow);
  event = contentWindow.document.createEvent("DragEvents");
  event.initDragEvent(
    "drop", true, true, contentWindow, 0, 0, 0, 0, 0,
    false, false, false, false, 0, null, dataTransfer);
  tabItem.container.dispatchEvent(event);
}

function checkSnap(item, offsetX, offsetY, contentWindow, callback) {
  let firstTop = item.getBounds().top;
  let firstLeft = item.getBounds().left;
  let onDrop = function() {
    let snapped = false;
    item.container.removeEventListener('drop', onDrop, false);
    if (item.getBounds().top != firstTop + offsetY)
      snapped = true;
    if (item.getBounds().left != firstLeft + offsetX)
      snapped = true;
    callback(snapped);
  };
  item.container.addEventListener('drop', onDrop, false);
  simulateDragDrop(item, offsetX, offsetY, contentWindow);
}
