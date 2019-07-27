




"use strict";



const IFRAME_SRC = "data:text/html;charset=utf-8," +
  "<div style='height:500px; width:500px; border:1px solid gray;'>" +
    "big div" +
  "</div>";

const TEST_URI = "data:text/html;charset=utf-8," +
  "<p>browser_inspector_scrolling.js</p>" +
  "<iframe src=\"" + IFRAME_SRC + "\" />";

add_task(function* () {
  let { inspector, toolbox } = yield openInspectorForURL(TEST_URI);

  let iframe = getNode("iframe");
  let div = getNode("div", { document: iframe.contentDocument });
  let divFront = yield getNodeFrontInFrame("div", "iframe", inspector);

  info("Waiting for highlighter box model to appear.");
  yield toolbox.highlighter.showBoxModel(divFront);

  let scrolled = once(gBrowser.selectedBrowser, "scroll");

  info("Scrolling iframe.");
  EventUtils.synthesizeWheel(div, 10, 10,
    { deltaY: 50.0, deltaMode: WheelEvent.DOM_DELTA_PIXEL },
    iframe.contentWindow);

  info("Waiting for scroll event");
  yield scrolled;

  let isRetina = devicePixelRatio === 2;
  is(iframe.contentDocument.body.scrollTop,
    isRetina ? 25 : 50, "inspected iframe scrolled");

  info("Hiding box model.");
  yield toolbox.highlighter.hideBoxModel();
});
