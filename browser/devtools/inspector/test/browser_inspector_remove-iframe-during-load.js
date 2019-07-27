


"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_inspector_remove-iframe-during-load.html";

add_task(function* () {
  let {inspector, toolbox, testActor} = yield openInspectorForURL("about:blank");
  yield selectNode("body", inspector);

  
  
  yield testActor.loadAndWaitForCustomEvent(TEST_URL);

  
  
  
  
  
  ok(!(yield testActor.hasNode("iframe")),
     "Iframes added by the content page should have been removed");

  
  info("Creating and removing an iframe.");
  let onMarkupLoaded = inspector.once("markuploaded");
  testActor.eval("new " + function () {
    var iframe = document.createElement("iframe");
    document.body.appendChild(iframe);
    iframe.remove();
  });

  ok(!(yield testActor.hasNode("iframe")),
     "The after-load iframe should have been removed.");

  info("Waiting for markup-view to load.");
  yield onMarkupLoaded;

  
  ok(!(yield testActor.hasNode("iframe")), "Iframe has been removed.");
  is((yield testActor.getProperty("#yay", "textContent")), "load", "Load event fired.");

  yield selectNode("#yay", inspector);
});
