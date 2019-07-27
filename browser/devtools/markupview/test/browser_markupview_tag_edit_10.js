



"use strict";



const TEST_URL = "data:text/html;charset=utf-8,<div></div>";

add_task(function*() {
  let {toolbox, inspector} = yield addTab(TEST_URL).then(openInspector);
  yield inspector.markup.expandAll();
  yield selectNode("div", inspector);

  info("Updating the DIV tagname to an invalid value");
  let container = yield getContainerForSelector("div", inspector);
  let onCancelReselect = inspector.markup.once("canceledreselectonremoved");
  let tagEditor = container.editor.tag;
  setEditableFieldValue(tagEditor, "<<<", inspector);
  yield onCancelReselect;
  ok(true, "The markup-view emitted the canceledreselectonremoved event");
  is(inspector.selection.nodeFront, container.node, "The test DIV is still selected");

  info("Updating the DIV tagname to a valid value this time");
  let onReselect = inspector.markup.once("reselectedonremoved");
  setEditableFieldValue(tagEditor, "span", inspector);
  yield onReselect;
  ok(true, "The markup-view emitted the reselectedonremoved event");

  let spanFront = yield getNodeFront("span", inspector);
  is(inspector.selection.nodeFront, spanFront, "The seelected node is now the SPAN");
});
