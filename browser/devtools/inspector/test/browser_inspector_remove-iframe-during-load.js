


"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_inspector_remove-iframe-during-load.html";

add_task(function* () {
  let { inspector, toolbox } = yield openInspectorForURL("about:blank");

  yield selectNode("body", inspector);

  let loaded = once(gBrowser.selectedBrowser, "load", true);
  content.location = TEST_URL;

  info("Waiting for test page to load.");
  yield loaded;

  
  
  
  info("Creating and removing an iframe.");
  var iframe = content.document.createElement("iframe");
  content.document.body.appendChild(iframe);
  iframe.remove();

  ok(!getNode("iframe", {expectNoMatch: true}), "Iframe has been removed.");

  info("Waiting for markup-view to load.");
  yield inspector.once("markuploaded");

  
  ok(!getNode("iframe", {expectNoMatch: true}), "Iframe has been removed.");
  is(getNode("#yay").textContent, "load", "Load event fired.");

  yield selectNode("#yay", inspector);
});
