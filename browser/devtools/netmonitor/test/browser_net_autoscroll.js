







thisTestLeaksUncaughtRejectionsAndShouldBeFixed("TypeError: aValue.content is undefined");




add_task(function*() {
  requestLongerTimeout(2);
  let monitor, debuggee, requestsContainer, scrollTop;

  let [aTab, aDebuggee, aMonitor] = yield initNetMonitor(INFINITE_GET_URL);
  monitor = aMonitor;
  debuggee = aDebuggee;
  let win = monitor.panelWin;
  let topNode = win.document.getElementById("requests-menu-contents");
  requestsContainer = topNode.getElementsByTagName("scrollbox")[0];
  ok(!!requestsContainer, "Container element exists as expected.");

  
  
  yield waitForRequestsToOverflowContainer(monitor, requestsContainer);
  yield waitForScroll(monitor);
  ok(scrolledToBottom(requestsContainer), "Scrolled to bottom on overflow.");

  
  
  let children = requestsContainer.childNodes;
  let middleNode = children.item(children.length / 2);
  middleNode.scrollIntoView();
  ok(!scrolledToBottom(requestsContainer), "Not scrolled to bottom.");
  scrollTop = requestsContainer.scrollTop; 
  yield waitForNetworkEvents(monitor, 8);
  yield waitSomeTime();
  is(requestsContainer.scrollTop, scrollTop, "Did not scroll.");

  
  
  requestsContainer.scrollTop = requestsContainer.scrollHeight;
  ok(scrolledToBottom(requestsContainer), "Set scroll position to bottom.");
  yield waitForNetworkEvents(monitor, 8);
  yield waitForScroll(monitor);
  ok(scrolledToBottom(requestsContainer), "Still scrolled to bottom.");

  
  
  monitor.panelWin.NetMonitorView.RequestsMenu.selectedIndex = 0;
  yield waitForNetworkEvents(monitor, 8);
  yield waitSomeTime();
  is(requestsContainer.scrollTop, 0, "Did not scroll.");

  
  yield teardown(monitor);

  finish();

  function waitForRequestsToOverflowContainer(aMonitor, aContainer) {
    return waitForNetworkEvents(aMonitor, 1).then(() => {
      if (aContainer.scrollHeight > aContainer.clientHeight) {
        return promise.resolve();
      } else {
        return waitForRequestsToOverflowContainer(aMonitor, aContainer);
      }
    });
  }

  function scrolledToBottom(aElement) {
    return aElement.scrollTop + aElement.clientHeight >= aElement.scrollHeight;
  }

  function waitSomeTime() {
    let waitSomeTime = promise.defer();
    setTimeout(waitSomeTime.resolve, 50); 
    return waitSomeTime.promise;
  }

  function waitForScroll(aMonitor) {
    return aMonitor._view.RequestsMenu.widget.once("scroll-to-bottom");
  }
});
