









































const TEST_URL = "http://mochi.test:8888/browser/browser/base/content/test/browser_overLinkInLocationBar.html";

var gTestIter;



function smokeTestGenerator() {
  let tab = openTestPage();
  yield;

  let contentDoc = gBrowser.contentDocument;
  let link = contentDoc.getElementById("link");

  mouseover(link);
  yield;
  checkURLBar(true);

  mouseout(link);
  yield;
  checkURLBar(false);

  gBrowser.removeTab(tab);
}

function test() {
  waitForExplicitFinish();
  gTestIter = smokeTestGenerator();
  cont();
}







function cont() {
  try {
    gTestIter.next();
  }
  catch (err if err instanceof StopIteration) {
    finish();
  }
}







function checkURLBar(shouldShowOverLink) {
  let overLink = window.getComputedStyle(gURLBar._overLinkBox, null);
  let origin = window.getComputedStyle(gURLBar._originLabel, null);
  let editLayer = window.getComputedStyle(gURLBar._textboxContainer, null);
  if (shouldShowOverLink) {
    isnot(origin.color, "transparent",
          "Origin color in over-link layer should not be transparent");
    is(overLink.opacity, 1, "Over-link should be opaque");
    is(editLayer.color, "transparent",
       "Edit layer color should be transparent");
  }
  else {
    is(origin.color, "transparent",
       "Origin color in over-link layer should be transparent");
    is(overLink.opacity, 0, "Over-link should be transparent");
    isnot(editLayer.color, "transparent",
          "Edit layer color should not be transparent");
  }
}







function openTestPage() {
  gBrowser.addEventListener("load", function onLoad(event) {
    if (event.target.URL == TEST_URL) {
      gBrowser.removeEventListener("load", onLoad, true);
      cont();
    }
  }, true);
  return gBrowser.loadOneTab(TEST_URL, { inBackground: false });
}









function mouseover(anchorNode) {
  mouseAnchorNode(anchorNode, true);
}









function mouseout(anchorNode) {
  mouseAnchorNode(anchorNode, false);
}











function mouseAnchorNode(node, over) {
  let overLink = gURLBar._overLinkBox;
  overLink.addEventListener("transitionend", function onTrans(event) {
    if (event.target == overLink) {
      overLink.removeEventListener("transitionend", onTrans, false);
      cont();
    }
  }, false);
  let offset = over ? 0 : -1;
  let eventType = over ? "mouseover" : "mouseout";
  EventUtils.synthesizeMouse(node, offset, offset,
                             { type: eventType, clickCount: 0 },
                             node.ownerDocument.defaultView);
}
