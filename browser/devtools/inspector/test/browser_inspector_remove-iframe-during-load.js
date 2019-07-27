


"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_inspector_remove-iframe-during-load.html";

add_task(function* () {
  let {inspector, toolbox} = yield openInspectorForURL("about:blank");
  yield selectNode("body", inspector);

  
  
  let done = waitForContentMessage("Test:TestPageProcessingDone");
  content.location = TEST_URL;
  yield done;

  
  
  
  
  
  ok(!getNode("iframe", {expectNoMatch: true}),
    "Iframes added by the content page should have been removed");

  
  info("Creating and removing an iframe.");
  let iframe = content.document.createElement("iframe");
  content.document.body.appendChild(iframe);
  iframe.remove();

  ok(!getNode("iframe", {expectNoMatch: true}),
    "The after-load iframe should have been removed.");

  info("Waiting for markup-view to load.");
  yield inspector.once("markuploaded");

  
  ok(!getNode("iframe", {expectNoMatch: true}), "Iframe has been removed.");
  is(getNode("#yay").textContent, "load", "Load event fired.");

  yield selectNode("#yay", inspector);
});
