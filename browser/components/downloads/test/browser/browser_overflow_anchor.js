







function test_task() {
  try {
    
    yield task_resetState();

    
    
    let oldWidth = window.outerWidth;

    
    let button = CustomizableUI.getWidget("downloads-button")
                               .forWindow(window);
    ok(!button.overflowed, "Downloads button should not be overflowed.");

    
    
    
    const kFlexyItems = ["urlbar-container", "search-container"];
    registerCleanupFunction(() => unlockWidth(kFlexyItems));
    lockWidth(kFlexyItems);

    
    
    window.resizeTo(oldWidth / 2, window.outerHeight);
    yield waitForOverflowed(button, true);

    let promise = promisePanelOpened();
    button.node.doCommand();
    yield promise;

    let panel = DownloadsPanel.panel;
    let chevron = document.getElementById("nav-bar-overflow-button");
    is(panel.anchorNode, chevron, "Panel should be anchored to the chevron.");

    DownloadsPanel.hidePanel();

    
    unlockWidth(kFlexyItems);

    
    window.resizeTo(oldWidth, window.outerHeight);

    
    yield waitForOverflowed(button, false);

    
    promise = promisePanelOpened();
    button.node.doCommand();
    yield promise;

    is(panel.anchorNode.id, "downloads-indicator-anchor");

    DownloadsPanel.hidePanel();
  } finally {
    
    yield task_resetState();
  }
}







function lockWidth(aItemIDs) {
  for (let itemID of aItemIDs) {
    let item = document.getElementById(itemID);
    let curWidth = item.getBoundingClientRect().width + "px";
    item.style.minWidth = curWidth;
  }
}






function unlockWidth(aItemIDs) {
  for (let itemID of aItemIDs) {
    let item = document.getElementById(itemID);
    item.style.minWidth = "";
  }
}







function waitForOverflowed(aItem, aIsOverflowed) {
  let deferOverflow = Promise.defer();
  if (aItem.overflowed == aIsOverflowed) {
    return deferOverflow.resolve();
  }

  let observer = new MutationObserver(function(aMutations) {
    if (aItem.overflowed == aIsOverflowed) {
      observer.disconnect();
      deferOverflow.resolve();
    }
  });
  observer.observe(aItem.node, {attributes: true});

  return deferOverflow.promise;
}
