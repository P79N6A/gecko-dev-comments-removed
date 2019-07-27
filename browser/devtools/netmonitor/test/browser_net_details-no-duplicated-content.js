


"use strict";



let test = Task.async(function* () {
  info("Initializing test");
  let [tab, debuggee, monitor] = yield initNetMonitor(CUSTOM_GET_URL);
  let panel = monitor.panelWin;
  let { NetMonitorView, EVENTS } = panel;
  let { RequestsMenu, NetworkDetails } = NetMonitorView;

  const COOKIE_UNIQUE_PATH = "/do-not-use-in-other-tests-using-cookies";

  let TEST_CASES = [
    {
      desc: "Test headers tab",
      pageURI: CUSTOM_GET_URL,
      requestURI: null,
      isPost: false,
      tabIndex: 0,
      variablesView: NetworkDetails._headers,
      expectedScopeLength: 2,
    },
    {
      desc: "Test cookies tab",
      pageURI: CUSTOM_GET_URL,
      requestURI: COOKIE_UNIQUE_PATH,
      isPost: false,
      tabIndex: 1,
      variablesView: NetworkDetails._cookies,
      expectedScopeLength: 1,
    },
    {
      desc: "Test params tab",
      pageURI: POST_RAW_URL,
      requestURI: null,
      isPost: true,
      tabIndex: 2,
      variablesView: NetworkDetails._params,
      expectedScopeLength: 1,
    },
  ];

  info("Adding a cookie for the \"Cookie\" tab test");
  debuggee.document.cookie = "a=b; path=" + COOKIE_UNIQUE_PATH;

  info("Running tests");
  for (let spec of TEST_CASES) {
    yield runTestCase(spec);
  }

  
  
  info("Removing the added cookie.");
  debuggee.document.cookie = "a=; expires=Thu, 01 Jan 1970 00:00:00 GMT; path=" +
    COOKIE_UNIQUE_PATH;

  yield teardown(monitor);
  finish();

  


  function* runTestCase(spec) {
    info("Running case: " + spec.desc);
    debuggee.content.location = spec.pageURI;

    yield waitForNetworkEvents(monitor, 1);
    RequestsMenu.clear();
    yield waitForFinalDetailTabUpdate(spec.tabIndex, spec.isPost, spec.requestURI);

    is(spec.variablesView._store.length, spec.expectedScopeLength,
       "View contains " + spec.expectedScopeLength + " scope headers");
  }

  






  function* waitForFinalDetailTabUpdate(tabIndex, isPost, uri) {
    let onNetworkEvent = waitFor(panel, EVENTS.NETWORK_EVENT);
    let onDetailsPopulated = waitFor(panel, EVENTS.NETWORKDETAILSVIEW_POPULATED);
    let onRequestFinished = isPost ?
      waitForNetworkEvents(monitor, 0, 1) : waitForNetworkEvents(monitor, 1);

    info("Performing a request");
    debuggee.performRequests(1, uri);

    info("Waiting for NETWORK_EVENT");
    yield onNetworkEvent;

    if (!RequestsMenu.getItemAtIndex(0)) {
      info("Waiting for the request to be added to the view")
      yield monitor.panelWin.once(monitor.panelWin.EVENTS.REQUEST_ADDED);
    }

    ok(true, "Received NETWORK_EVENT. Selecting the item.");
    let item = RequestsMenu.getItemAtIndex(0);
    RequestsMenu.selectedItem = item;

    info("Item selected. Waiting for NETWORKDETAILSVIEW_POPULATED");
    yield onDetailsPopulated;

    info("Received populated event. Selecting tab at index " + tabIndex);
    NetworkDetails.widget.selectedIndex = tabIndex;

    info("Waiting for request to finish.");
    yield onRequestFinished;

    ok(true, "Request finished.");

    



















    let hasQueuedUpdates = RequestsMenu._updateQueue.length !== 0;
    let hasRunningTabUpdate = NetworkDetails._viewState.updating[tabIndex];

    if (hasQueuedUpdates || hasRunningTabUpdate) {
      info("There's pending updates - waiting for them to finish.");
      info("  hasQueuedUpdates: " + hasQueuedUpdates);
      info("  hasRunningTabUpdate: " + hasRunningTabUpdate);

      if (hasQueuedUpdates && hasRunningTabUpdate) {
        info("Waiting for updates to be flushed.");
        
        yield waitFor(panel, EVENTS.NETWORKDETAILSVIEW_POPULATED);

        info("Requests flushed.");
      }

      info("Waiting for final tab update.");
      yield waitFor(panel, EVENTS.TAB_UPDATED);
    }

    info("All updates completed.");
  }
});
