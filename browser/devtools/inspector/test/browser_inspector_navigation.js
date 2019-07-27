




"use strict";



const TEST_URL_FILE = "browser/browser/devtools/inspector/test/" +
  "doc_inspector_breadcrumbs.html";

const TEST_URL_1 = "http://test1.example.org/" + TEST_URL_FILE;
const TEST_URL_2 = "http://test2.example.org/" + TEST_URL_FILE;

add_task(function* () {
  let { inspector } = yield openInspectorForURL(TEST_URL_1);
  let markuploaded = inspector.once("markuploaded");

  yield selectNode("#i1", inspector);

  info("Navigating to a different page.");
  yield navigateTo(toolbox, TEST_URL_2);

  info("Waiting for markup view to load after navigation.");
  yield markuploaded;

  ok(true, "New page loaded");
  yield selectNode("#i1", inspector);

  markuploaded = inspector.once("markuploaded");

  info("Going back in history");
  content.history.go(-1);

  info("Waiting for markup view to load after going back in history.");
  yield markuploaded;

  ok(true, "Old page loaded");
  is(content.location.href, TEST_URL_1, "URL is correct.");

  yield selectNode("#i1", inspector);
});
