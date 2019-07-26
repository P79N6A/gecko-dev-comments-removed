






function test() {
  let monitor, debuggee, requestsContainer, scrollTop;

  initNetMonitor(INFINITE_GET_URL).then(([aTab, aDebuggee, aMonitor]) => {
    monitor = aMonitor;
    debuggee = aDebuggee;
    let win = monitor.panelWin;
    let topNode = win.document.getElementById("requests-menu-contents");
    requestsContainer = topNode.getElementsByTagName("scrollbox")[0];
    ok(!!requestsContainer, "Container element exists as expected.");
  })

  
  
  .then(() => {
    debuggee.performRequests();
    return waitForRequestsToOverflowContainer(monitor, requestsContainer);
  }).then(() => {
    ok(scrolledToBottom(requestsContainer), "Scrolled to bottom on overflow.");
  })

  
  
  .then(() => {
    let children = requestsContainer.childNodes;
    let middleNode = children.item(children.length / 2);
    middleNode.scrollIntoView();
    ok(!scrolledToBottom(requestsContainer), "Not scrolled to bottom.");
    scrollTop = requestsContainer.scrollTop; 
    return waitForNetworkEvents(monitor, 8);
  }).then(() => {
    is(requestsContainer.scrollTop, scrollTop, "Did not scroll.");
  })

  
  
  .then(() => {
    requestsContainer.scrollTop = requestsContainer.scrollHeight;
    ok(scrolledToBottom(requestsContainer), "Set scroll position to bottom.");
    return waitForNetworkEvents(monitor, 8);
  }).then(() => {
    ok(scrolledToBottom(requestsContainer), "Still scrolled to bottom.");
  })

  
  .then(() => {
    return teardown(monitor).then(finish);
  })

  
  .then(null, (err) => {
    ok(false, err);
    finish();
  });

  function waitForRequestsToOverflowContainer (aMonitor, aContainer) {
    return waitForNetworkEvents(aMonitor, 1).then(() => {
      if (aContainer.scrollHeight > aContainer.clientHeight) {
        
        return waitForNetworkEvents(aMonitor, 8);
      } else {
        return waitForRequestsToOverflowContainer(aMonitor, aContainer);
      }
    });
  }

  function scrolledToBottom(aElement) {
    return aElement.scrollTop + aElement.clientHeight >= aElement.scrollHeight;
  }
}
