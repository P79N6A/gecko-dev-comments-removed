


"use strict";




const TEST_URL = TEST_BASE_HTTP + "doc_uncached.html";

add_task(function() {
  info("Opening netmonitor");
  let tab = yield addTab("about:blank");
  let target = TargetFactory.forTab(tab);
  let toolbox = yield gDevTools.showToolbox(target, "netmonitor");
  let netmonitor = toolbox.getPanel("netmonitor");

  info("Navigating to test page");
  yield navigateTo(TEST_URL);

  info("Opening Style Editor");
  let styleeditor = yield toolbox.selectTool("styleeditor");

  info("Waiting for the source to be loaded.");
  yield styleeditor.UI.editors[0].getSourceEditor();

  info("Checking Netmonitor contents.");
  let requestsForCss = 0;
  for (let item of netmonitor._view.RequestsMenu) {
    if (item.attachment.url.endsWith("doc_uncached.css")) {
      requestsForCss++;
    }
  }

  is(requestsForCss, 1,
     "Got one request for doc_uncached.css after Style Editor was loaded.");
});
