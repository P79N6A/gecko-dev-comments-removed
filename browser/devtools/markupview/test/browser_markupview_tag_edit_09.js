



"use strict";



const TEST_URL = TEST_URL_ROOT + "doc_markup_svg_attributes.html";

let test = asyncTest(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  yield inspector.markup.expandAll();
  yield selectNode("svg", inspector);

  yield testWellformedMixedCase(inspector);
  yield testMalformedMixedCase(inspector);
});

function* testWellformedMixedCase(inspector) {
  info("Modifying a mixed-case attribute, " +
    "expecting the attribute's case to be preserved");

  info("Listening to markup mutations");
  let onMutated = inspector.once("markupmutation");

  info("Focusing the viewBox attribute editor");
  let {editor} = yield getContainerForSelector("svg", inspector);
  let attr = editor.attrs["viewBox"].querySelector(".editable");
  attr.focus();
  EventUtils.sendKey("return", inspector.panelWin);

  info("Editing the attribute value and waiting for the mutation event");
  let input = inplaceEditor(attr).input;
  input.value = "viewBox=\"0 0 1 1\"";
  EventUtils.sendKey("return", inspector.panelWin);
  yield onMutated;

  assertAttributes("svg", {
    "viewBox": "0 0 1 1",
    "width": "200",
    "height": "200"
  });
}

function* testMalformedMixedCase(inspector) {
  info("Modifying a mixed-case attribute, making sure to generate a parsing" +
    "error, and  expecting the attribute's case to NOT be preserved");
  
  
  

  info("Listening to markup mutations");
  let onMutated = inspector.once("markupmutation");

  info("Focusing the viewBox attribute editor");
  let {editor} = yield getContainerForSelector("svg", inspector);
  let attr = editor.attrs["viewBox"].querySelector(".editable");
  attr.focus();
  EventUtils.sendKey("return", inspector.panelWin);

  info("Editing the attribute value and waiting for the mutation event");
  let input = inplaceEditor(attr).input;
  input.value = "viewBox=\"<>\"";
  EventUtils.sendKey("return", inspector.panelWin);
  yield onMutated;

  assertAttributes("svg", {
    "viewbox": "<>",
    "width": "200",
    "height": "200"
  });
}
