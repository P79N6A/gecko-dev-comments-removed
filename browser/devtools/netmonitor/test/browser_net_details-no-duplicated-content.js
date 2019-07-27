


"use strict";



let test = Task.async(function* () {
  info("Initializing test");
  let [tab, debuggee, monitor] = yield initNetMonitor(CUSTOM_GET_URL);
  let panel = monitor.panelWin;
  let { NetMonitorView, EVENTS } = panel;
  let { RequestsMenu, NetworkDetails } = NetMonitorView;

  let TEST_CASES = [
    {
      desc: "Test headers tab",
      pageURI: CUSTOM_GET_URL,
      isPost: false,
      tabIndex: 0,
      variablesView: NetworkDetails._headers,
      expectedScopeLength: 2,
    },
    {
      desc: "Test cookies tab",
      pageURI: CUSTOM_GET_URL,
      isPost: false,
      tabIndex: 1,
      variablesView: NetworkDetails._cookies,
      expectedScopeLength: 1,
    },
    {
      desc: "Test params tab",
      pageURI: POST_RAW_URL,
      isPost: true,
      tabIndex: 2,
      variablesView: NetworkDetails._params,
      expectedScopeLength: 1,
    },
  ];

  info("Adding a cookie for the \"Cookie\" tab test");
  debuggee.document.cookie = "a=b; Max-Age=10; path=" +  CUSTOM_GET_URL;
  is(debuggee.document.cookie, "a=b", "Cookie was added.")

  info("Running tests");
  for (let spec of TEST_CASES) {
    yield runTestCase(spec);
  }

  
  
  info("Removing the added cookie.");
  debuggee.document.cookie = "a=; expires=Thu, 01 Jan 1970 00:00:00 GMT";
  is(debuggee.document.cookie, "", "Cookie was removed.");

  yield teardown(monitor);
  finish();

  


  function* runTestCase(spec) {
    info("Running case: " + spec.desc);
    debuggee.content.location = spec.pageURI;

    yield waitForNetworkEvents(monitor, 1);
    RequestsMenu.clear();
    yield waitForFinalDetailTabUpdate(spec.tabIndex, spec.isPost);

    is(spec.variablesView._store.length, spec.expectedScopeLength,
       "View contains " + spec.expectedScopeLength + " scope headers");
  }

  






  function* waitForFinalDetailTabUpdate(tabIndex, isPost) {
    let onNetworkEvent = waitFor(panel, EVENTS.NETWORK_EVENT);
    let onDetailsPopulated = waitFor(panel, EVENTS.NETWORKDETAILSVIEW_POPULATED);
    let onRequestFinished = isPost ?
      waitForNetworkEvents(monitor, 0, 1) : waitForNetworkEvents(monitor, 1);

    info("Performing a request");
    debuggee.performRequests(1, null);

    info("Waiting for NETWORK_EVENT");
    yield onNetworkEvent;

    ok(true, "Received NETWORK_EVENT. Selecting the item.");
    let item = RequestsMenu.getItemAtIndex(0);
    RequestsMenu.selectedItem = item;

    info("Item selected. Waiting for NETWORKDETAILSVIEW_POPULATED");
    yield onDetailsPopulated;

    info("Selecting tab at index " + tabIndex);
    NetworkDetails.widget.selectedIndex = tabIndex;

    ok(true, "Received NETWORKDETAILSVIEW_POPULATED. Waiting for request to finish");
    yield onRequestFinished;

    ok(true, "Request finished. Waiting for tab update to complete");
    let onDetailsUpdateFinished = waitFor(panel, EVENTS.TAB_UPDATED);
    yield onDetailsUpdateFinished;
    ok(true, "Details were updated");
  }
});
