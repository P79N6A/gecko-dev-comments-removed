


"use strict";



const SCHEMA = "data:text/html;charset=UTF-8,";
const URL_1 = SCHEMA + "<div id='one' style='color:red;'>ONE</div>";
const URL_2 = SCHEMA + "<div id='two' style='color:green;'>TWO</div>";

add_task(function* () {
  let { inspector, toolbox } = yield addTab(URL_1).then(openInspector);

  assertMarkupViewIsLoaded();
  yield selectNode("#one", inspector);

  let willNavigate = toolbox.target.once("will-navigate");
  content.location = URL_2;

  info("Waiting for will-navigate");
  yield willNavigate;

  info("Navigation to page 2 has started, the inspector should be empty");
  assertMarkupViewIsEmpty();

  info("Waiting for new-root");
  yield inspector.once("new-root");

  info("Navigation to page 2 was done, the inspector should be back up");
  assertMarkupViewIsLoaded();

  yield selectNode("#two", inspector);

  function assertMarkupViewIsLoaded() {
    let markupViewBox = inspector.panelDoc.getElementById("markup-box");
    is(markupViewBox.childNodes.length, 1, "The markup-view is loaded");
  }

  function assertMarkupViewIsEmpty() {
    let markupViewBox = inspector.panelDoc.getElementById("markup-box");
    is(markupViewBox.childNodes.length, 0, "The markup-view is unloaded");
  }
});
