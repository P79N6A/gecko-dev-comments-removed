













const TAB_URL = EXAMPLE_URL + "doc_event-listeners-01.html";

add_task(function* () {
  let tab = yield addTab(TAB_URL);

  
  
  let tabs = require('sdk/tabs');
  let sdkTab = [...tabs].find(tab => tab.url === TAB_URL);
  ok(sdkTab, "Add-on SDK found the loaded tab.");

  info("Attaching an event handler via add-on sdk content scripts.");
  let worker = sdkTab.attach({
    contentScript: "document.body.addEventListener('click', e => alert(e))",
    onError: ok.bind(this, false)
  });

  let [,, panel, win] = yield initDebugger(tab);
  let gDebugger = panel.panelWin;
  let fetched = waitForDebuggerEvents(panel, gDebugger.EVENTS.EVENT_LISTENERS_FETCHED);

  info("Scheduling event listener fetch.");
  gDebugger.DebuggerController.Breakpoints.DOM.scheduleEventListenersFetch();

  info("Waiting for updated event listeners to arrive.");
  yield fetched;

  ok(true, "The listener update did not hang.");
});
