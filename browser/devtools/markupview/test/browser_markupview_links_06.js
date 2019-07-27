



"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_markup_links.html";

add_task(function*() {
  let {toolbox, inspector} = yield addTab(TEST_URL).then(openInspector);

  info("Select a node with a cssresource attribute");
  yield selectNode("link", inspector);

  info("Set the popupNode to the node that contains the uri");
  let {editor} = yield getContainerForSelector("link", inspector);
  let popupNode = editor.attrElements.get("href").querySelector(".link");
  inspector.panelDoc.popupNode = popupNode;

  info("Follow the link and wait for the style-editor to open");
  let onStyleEditorReady = toolbox.once("styleeditor-ready");
  inspector.followAttributeLink();
  yield onStyleEditorReady;

  
  
  ok(true, "The style-editor was open");

  info("Switch back to the inspector");
  yield toolbox.selectTool("inspector");

  info("Select a node with a jsresource attribute");
  yield selectNode("script", inspector);

  info("Set the popupNode to the node that contains the uri");
  ({editor} = yield getContainerForSelector("script", inspector));
  popupNode = editor.attrElements.get("src").querySelector(".link");
  inspector.panelDoc.popupNode = popupNode;

  info("Follow the link and wait for the debugger to open");
  let onDebuggerReady = toolbox.once("jsdebugger-ready");
  inspector.followAttributeLink();
  yield onDebuggerReady;

  
  
  ok(true, "The debugger was open");
});
