





add_task(function*() {
  let [ aTab, aDebuggee, aMonitor ] = yield initNetMonitor(
    HAR_EXAMPLE_URL + "html_har_post-data-test-page.html");

  info("Starting test... ");

  let { NetMonitorView } = aMonitor.panelWin;
  let { RequestsMenu } = NetMonitorView;

  RequestsMenu.lazyUpdate = false;

  
  aDebuggee.executeTest();
  yield waitForNetworkEvents(aMonitor, 0, 1);

  
  let jsonString = yield RequestsMenu.copyAllAsHar();
  let har = JSON.parse(jsonString);

  
  isnot(har.log, null, "The HAR log must exist");
  is(har.log.pages.length, 1, "There must be one page");
  is(har.log.entries.length, 1, "There must be one request");

  let entry = har.log.entries[0];
  is(entry.request.postData.mimeType, "application/json; charset=UTF-8",
    "Check post data content type");
  is(entry.request.postData.text, "{'first': 'John', 'last': 'Doe'}",
    "Check post data payload");

  
  teardown(aMonitor).then(finish);
});
