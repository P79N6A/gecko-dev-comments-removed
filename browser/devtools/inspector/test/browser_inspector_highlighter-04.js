



"use strict";



const TEST_URL = "data:text/html;charset=utf-8,<div>test</div>";


const ELEMENTS = ["box-model-root",
                  "box-model-elements",
                  "box-model-margin",
                  "box-model-border",
                  "box-model-padding",
                  "box-model-content",
                  "box-model-guide-top",
                  "box-model-guide-right",
                  "box-model-guide-bottom",
                  "box-model-guide-left",
                  "box-model-nodeinfobar-container",
                  "box-model-nodeinfobar-tagname",
                  "box-model-nodeinfobar-id",
                  "box-model-nodeinfobar-classes",
                  "box-model-nodeinfobar-pseudo-classes",
                  "box-model-nodeinfobar-dimensions"];

add_task(function*() {
  let {inspector, toolbox} = yield openInspectorForURL(TEST_URL);

  info("Show the box-model highlighter");
  let divFront = yield getNodeFront("div", inspector);
  yield toolbox.highlighter.showBoxModel(divFront);

  for (let id of ELEMENTS) {
    let {data: foundId} = yield executeInContent("Test:GetHighlighterAttribute", {
      nodeID: id,
      name: "id",
      actorID: getHighlighterActorID(toolbox)
    });
    is(foundId, id, "Element " + id + " found");
  }

  info("Hide the box-model highlighter");
  yield toolbox.highlighter.hideBoxModel();
});
