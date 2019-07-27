


"use strict";

Cu.import("resource://testing-common/ContentTask.jsm", this);

const URL = "http://example.com/browser/toolkit/components/aboutperformance/tests/browser/browser_compartments.html?test=" + Math.random();


function frameScript() {
  "use strict";

  addMessageListener("aboutperformance-test:hasItems", ({data: url}) => {
    let hasPlatform = false;
    let hasURL = false;

    try {
      let eltData = content.document.getElementById("liveData");
      if (!eltData) {
        return;
      }

      
      hasPlatform = eltData.querySelector("tr.platform") != null;

      
      hasURL = false;
      for (let eltContent of eltData.querySelectorAll("tr.content td.name")) {
        if (eltContent.textContent == url) {
          hasURL = true;
          break;
        }
      }

    } catch (ex) {
      Cu.reportError("Error in content: " + ex);
      Cu.reportError(ex.stack);
    } finally {
      sendAsyncMessage("aboutperformance-test:hasItems", {hasPlatform, hasURL});
    }
  });
}


add_task(function* test() {
  let tabAboutPerformance = gBrowser.addTab("about:performance");
  let tabContent = gBrowser.addTab(URL);

  yield ContentTask.spawn(tabAboutPerformance.linkedBrowser, null, frameScript);

  while (true) {
    yield new Promise(resolve => setTimeout(resolve, 100));
    let {hasPlatform, hasURL} = (yield promiseContentResponse(tabAboutPerformance.linkedBrowser, "aboutperformance-test:hasItems", URL));
    info(`Platform: ${hasPlatform}, url: ${hasURL}`);
    if (hasPlatform && hasURL) {
      Assert.ok(true, "Found a row for <platform> and a row for our URL");
      break;
    }
  }

  
  gBrowser.removeTab(tabContent);
  gBrowser.removeTab(tabAboutPerformance);
});
