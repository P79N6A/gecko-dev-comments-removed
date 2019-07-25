


let fadeAwayUndoButtonDelay;
let fadeAwayUndoButtonDuration;

function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", testCloseLastGroup, false);
  TabView.toggle();
}

function testCloseLastGroup() {
  window.removeEventListener("tabviewshown", testCloseLastGroup, false);
  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;

  is(contentWindow.GroupItems.groupItems.length, 1, "Has one group only");

  let groupItem = contentWindow.GroupItems.groupItems[0];
  
  let checkExistence = function() {
    is(contentWindow.GroupItems.groupItems.length, 1, 
       "Still has one group after delay");

    EventUtils.sendMouseEvent(
      { type: "click" }, groupItem.$undoContainer[0], contentWindow);
  };

  groupItem.addSubscriber("groupHidden", function onHidden() {
    groupItem.removeSubscriber("groupHidden", onHidden);
    
    setTimeout(checkExistence, 3);
  });

  groupItem.addSubscriber("groupShown", function onShown() {
    groupItem.removeSubscriber("groupShown", onShown);

    let endGame = function() {
      window.removeEventListener("tabviewhidden", endGame, false);
      ok(!TabView.isVisible(), "Tab View is hidden");

      groupItem.fadeAwayUndoButtonDelay = fadeAwayUndoButtonDelay;
      groupItem.fadeAwayUndoButtonDuration = fadeAwayUndoButtonDuration;

      finish();
    };
    window.addEventListener("tabviewhidden", endGame, false);

    TabView.toggle();
  });

  let closeButton = groupItem.container.getElementsByClassName("close");
  ok(closeButton, "Group item close button exists");

  
  fadeAwayUndoButtonDelay = groupItem.fadeAwayUndoButtonDelay;
  fadeAwayUndoButtonDuration = groupItem.fadeAwayUndoButtonDuration;

  
  groupItem.fadeAwayUndoButtonDelay = 1;
  groupItem.fadeAwayUndoButtonDuration = 1;

  EventUtils.sendMouseEvent({ type: "click" }, closeButton[0], contentWindow);
}
