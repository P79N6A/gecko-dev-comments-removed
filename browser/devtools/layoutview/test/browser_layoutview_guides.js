


"use strict";







const STYLE = "div { position: absolute; top: 50px; left: 50px; height: 10px; " +
              "width: 10px; border: 10px solid black; padding: 10px; margin: 10px;}";
const HTML = "<style>" + STYLE + "</style><div></div>";
const TEST_URL = "data:text/html;charset=utf-8," + encodeURIComponent(HTML);

let highlightedNodeFront, highlighterOptions;

add_task(function*() {
  yield addTab(TEST_URL);
  let {toolbox, inspector, view} = yield openLayoutView();
  yield selectNode("div", inspector);

  
  toolbox.highlighter.showBoxModel = function(nodeFront, options) {
    highlightedNodeFront = nodeFront;
    highlighterOptions = options;
  }

  let elt = view.doc.getElementById("margins");
  yield testGuideOnLayoutHover(elt, "margin", inspector, view);

  elt = view.doc.getElementById("borders");
  yield testGuideOnLayoutHover(elt, "border", inspector, view);

  elt = view.doc.getElementById("padding");
  yield testGuideOnLayoutHover(elt, "padding", inspector, view);

  elt = view.doc.getElementById("content");
  yield testGuideOnLayoutHover(elt, "content", inspector, view);
});

function* testGuideOnLayoutHover(elt, expectedRegion, inspector, view) {
  info("Synthesizing mouseover on the layout-view");
  EventUtils.synthesizeMouse(elt, 2, 2, {type:'mouseover'},
    elt.ownerDocument.defaultView);

  info("Waiting for the node-highlight event from the toolbox");
  yield inspector.toolbox.once("node-highlight");

  is(highlightedNodeFront, inspector.selection.nodeFront,
    "The right nodeFront was highlighted");
  is(highlighterOptions.region, expectedRegion,
    "Region " + expectedRegion + " was highlighted");
}
