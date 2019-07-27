



"use strict";





const server = createTestHTTPServer();



server.registerContentType("gif", "image/gif");
server.registerPathHandler("/slow.gif", function (metadata, response) {
  info ("Image has been requested");
  response.processAsync();
  setTimeout(() => {
    info ("Image is responding");
    response.finish();
  }, 500);
});


const TEST_URL = "data:text/html," +
  "<!DOCTYPE html>" +
  "<head><meta charset='utf-8' /></head>" +
  "<body>" +
  "<p>Slow script</p>" +
  "<img src='http://localhost:" + server.identity.primaryPort + "/slow.gif' /></script>" +
  "</body>" +
  "</html>";

add_task(function*() {
  let tab = yield addTab(TEST_URL);
  let {inspector} = yield openInspector();
  let domContentLoaded = waitForLinkedBrowserEvent(tab, "DOMContentLoaded");
  let pageLoaded = waitForLinkedBrowserEvent(tab, "load");

  ok (inspector.markup, "There is a markup view");

  
  reloadTab();
  yield domContentLoaded;
  yield chooseWithInspectElementContextMenu("img");
  yield pageLoaded;

  yield inspector.once("markuploaded");
  ok (inspector.markup, "There is a markup view");
  is (inspector.markup._elt.children.length, 1, "The markup view is rendering");
});

function* chooseWithInspectElementContextMenu(selector) {
  yield executeInContent("Test:SynthesizeMouse", {
    center: true,
    selector: selector,
    options: {type: "contextmenu", button: 2}
  });
  executeInContent("Test:SynthesizeKey", {key: "Q", options: {}});
}

function waitForLinkedBrowserEvent(tab, event) {
  let def = promise.defer();
  tab.linkedBrowser.addEventListener(event, function cb() {
    tab.linkedBrowser.removeEventListener(event, cb, true);
    def.resolve();
  }, true);
  return def.promise;
}
