



"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/inspector/" +
                 "test/browser_inspector_infobar_02.html";
const DOORHANGER_ARROW_HEIGHT = 5;



let test = asyncTest(function*() {
  info("Loading the test document and opening the inspector");

  yield addTab(TEST_URI);

  let {inspector} = yield openInspector();

  yield checkInfoBarAboveTop(inspector);
  yield checkInfoBarBelowFindbar(inspector);

  gBrowser.removeCurrentTab();
});

function* checkInfoBarAboveTop(inspector) {
  yield selectAndHighlightNode("#above-top", inspector);

  let positioner = getPositioner();
  let positionerTop = parseInt(positioner.style.top, 10);
  let insideContent = positionerTop >= -DOORHANGER_ARROW_HEIGHT;

  ok(insideContent, "Infobar is inside the content window (top = " +
                    positionerTop + ", content = '" +
                    positioner.textContent +"')");
}

function* checkInfoBarBelowFindbar(inspector) {
  gFindBar.open();

  
  yield once(gFindBar, "transitionend");
  yield selectAndHighlightNode("#below-bottom", inspector);

  let positioner = getPositioner();
  let positionerBottom =
    positioner.getBoundingClientRect().bottom - DOORHANGER_ARROW_HEIGHT;
  let findBarTop = gFindBar.getBoundingClientRect().top;

  let insideContent = positionerBottom <= findBarTop;

  ok(insideContent, "Infobar does not overlap the findbar (findBarTop = " +
                    findBarTop + ", positionerBottom = " + positionerBottom +
                    ", content = '" + positioner.textContent +"')");

  gFindBar.close();
  yield once(gFindBar, "transitionend");
}

function getPositioner() {
  let browser = gBrowser.selectedBrowser;
  let stack = browser.parentNode;

  return stack.querySelector(".highlighter-nodeinfobar-positioner");
}
