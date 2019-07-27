



"use strict";




const TEST_URL = "data:text/html;charset=utf-8,<div></div>";

add_task(function*() {
  let isEditTagNameCalled = false;

  let {toolbox, inspector} = yield addTab(TEST_URL).then(openInspector);

  
  
  inspector.walker.editTagName = function() { isEditTagNameCalled = true; }

  yield selectNode("div", inspector);
  let container = yield getContainerForSelector("div", inspector);
  let tagEditor = container.editor.tag;

  info("Blurring the tagname field");
  tagEditor.blur();
  is(isEditTagNameCalled, false, "The editTagName method wasn't called");

  info("Updating the tagname to uppercase");
  setEditableFieldValue(tagEditor, "DIV", inspector);
  is(isEditTagNameCalled, false, "The editTagName method wasn't called");

  info("Updating the tagname to a different value");
  setEditableFieldValue(tagEditor, "SPAN", inspector);
  is(isEditTagNameCalled, true, "The editTagName method was called");
});