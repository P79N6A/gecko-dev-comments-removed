







let doc;
let div;
let editorTestSteps;

function doNextStep() {
  editorTestSteps.next();
}

function setupEditorTests()
{
  div = doc.createElement("div");
  div.setAttribute("id", "foobar");
  div.setAttribute("class", "barbaz");
  doc.body.appendChild(div);

  Services.obs.addObserver(setupHTMLPanel, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.toggleInspectorUI();
}

function setupHTMLPanel()
{
  Services.obs.removeObserver(setupHTMLPanel, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED);
  Services.obs.addObserver(runEditorTests, InspectorUI.INSPECTOR_NOTIFICATIONS.TREEPANELREADY, false);
  InspectorUI.toggleHTMLPanel();
}

function runEditorTests()
{
  Services.obs.removeObserver(runEditorTests, InspectorUI.INSPECTOR_NOTIFICATIONS.TREEPANELREADY);
  InspectorUI.stopInspecting();
  InspectorUI.inspectNode(doc.body, true);

  
  editorTestSteps = doEditorTestSteps();

  
  Services.obs.addObserver(doNextStep, InspectorUI.INSPECTOR_NOTIFICATIONS.EDITOR_OPENED, false);
  Services.obs.addObserver(doNextStep, InspectorUI.INSPECTOR_NOTIFICATIONS.EDITOR_CLOSED, false);
  Services.obs.addObserver(doNextStep, InspectorUI.INSPECTOR_NOTIFICATIONS.EDITOR_SAVED, false);

  
  doNextStep();
}

function highlighterTrap()
{
  
  InspectorUI.highlighter.removeListener("nodeselected", highlighterTrap);
  ok(false, "Highlighter moved. Shouldn't be here!");
  finishUp();
}

function doEditorTestSteps()
{
  let treePanel = InspectorUI.treePanel;
  let editor = treePanel.treeBrowserDocument.getElementById("attribute-editor");
  let editorInput = treePanel.treeBrowserDocument.getElementById("attribute-editor-input");

  
  let nodes = treePanel.treeBrowserDocument.querySelectorAll(".nodeName.editable");
  let attrNameNode_id = nodes[0]
  let attrNameNode_class = nodes[1];

  is(attrNameNode_id.innerHTML, "id", "Step 1: we have the correct `id` attribute-name node in the HTML panel");
  is(attrNameNode_class.innerHTML, "class", "we have the correct `class` attribute-name node in the HTML panel");

  
  executeSoon(function() {
    
    EventUtils.synthesizeMouse(attrNameNode_id, 2, 2, {clickCount: 2}, attrNameNode_id.ownerDocument.defaultView);
  });

  yield; 


  
  ok(InspectorUI.treePanel.editingContext, "Step 2: editor session started");
  let selection = InspectorUI.selection;

  ok(selection, "Selection is: " + selection);

  let editorVisible = editor.classList.contains("editing");
  ok(editorVisible, "editor popup visible");

  
  let editorDims = editor.getBoundingClientRect();
  let attrNameNodeDims = attrNameNode_id.getBoundingClientRect();
  let editorPositionOK = (editorDims.left >= (attrNameNodeDims.left - editorDims.width - 5)) &&
                          (editorDims.right <= (attrNameNodeDims.right + editorDims.width + 5)) &&
                          (editorDims.top >= (attrNameNodeDims.top - editorDims.height - 5)) &&
                          (editorDims.bottom <= (attrNameNodeDims.bottom + editorDims.height + 5));

  ok(editorPositionOK, "editor position acceptable");

  
  let attrNameNodeHighlighted = attrNameNode_id.classList.contains("editingAttributeValue");
  ok(attrNameNodeHighlighted, "`id` attribute-name node is editor-highlighted");

  is(treePanel.editingContext.repObj, div, "editor session has correct reference to div");
  is(treePanel.editingContext.attrObj, attrNameNode_id, "editor session has correct reference to `id` attribute-name node in HTML panel");
  is(treePanel.editingContext.attrName, "id", "editor session knows correct attribute-name");

  editorInput.value = "burp";
  editorInput.focus();

  InspectorUI.highlighter.addListener("nodeselected", highlighterTrap);

  
  executeSoon(function() {
    
    EventUtils.synthesizeKey("VK_LEFT", {}, attrNameNode_id.ownerDocument.defaultView);
    EventUtils.synthesizeKey("VK_RETURN", {}, attrNameNode_id.ownerDocument.defaultView);
  });

  
  yield;
  yield; 

  
  InspectorUI.highlighter.removeListener("nodeselected", highlighterTrap);

  
  ok(!treePanel.editingContext, "Step 3: editor session ended");
  editorVisible = editor.classList.contains("editing");
  ok(!editorVisible, "editor popup hidden");
  attrNameNodeHighlighted = attrNameNode_id.classList.contains("editingAttributeValue");
  ok(!attrNameNodeHighlighted, "`id` attribute-value node is no longer editor-highlighted");
  is(div.getAttribute("burp"), "foobar", "`id` attribute-name successfully updated");
  is(attrNameNode_id.innerHTML, "burp", "attribute-name node in HTML panel successfully updated");

  
  executeSoon(function() {
    
    EventUtils.synthesizeMouse(attrNameNode_class, 2, 2, {clickCount: 2}, attrNameNode_class.ownerDocument.defaultView);
  });

  yield; 


  
  ok(treePanel.editingContext, "Step 4: editor session started");
  editorVisible = editor.classList.contains("editing");
  ok(editorVisible, "editor popup visible");

  is(treePanel.editingContext.attrObj, attrNameNode_class, "editor session has correct reference to `class` attribute-name node in HTML panel");
  is(treePanel.editingContext.attrName, "class", "editor session knows correct attribute-name");

  editorInput.value = "Hello World";
  editorInput.focus();

  
  executeSoon(function() {
    EventUtils.synthesizeKey("VK_ESCAPE", {}, attrNameNode_class.ownerDocument.defaultView);
  });

  yield; 


  
  ok(!treePanel.editingContext, "Step 5: editor session ended");
  editorVisible = editor.classList.contains("editing");
  ok(!editorVisible, "editor popup hidden");
  is(div.getAttribute("class"), "barbaz", "`class` attribute-name *not* updated");
  is(attrNameNode_class.innerHTML, "class", "attribute-name node in HTML panel *not* updated");

  
  executeSoon(function() {
    
    EventUtils.synthesizeMouse(attrNameNode_id, 2, 2, {clickCount: 2}, attrNameNode_id.ownerDocument.defaultView);
  });

  yield; 


  
  ok(treePanel.editingContext, "Step 6: editor session started");
  editorVisible = editor.classList.contains("editing");
  ok(editorVisible, "editor popup visible");

  
  executeSoon(function() {
    
    EventUtils.synthesizeMouse(editorInput, 2, 2, {clickCount: 2}, editorInput.ownerDocument.defaultView);

    
    
    executeSoon(function() {
      doNextStep();
    });
  });

  yield; 


  
  
  ok(treePanel.editingContext, "Step 7: editor session still going");
  editorVisible = editor.classList.contains("editing");
  ok(editorVisible, "editor popup still visible");

  editorInput.value = "all your base are belong to us";

  
  executeSoon(function() {
    EventUtils.synthesizeMouse(attrNameNode_class, 2, 2, {}, attrNameNode_class.ownerDocument.defaultView);
  });

  yield; 


  
  ok(!treePanel.editingContext, "Step 8: editor session ended");
  editorVisible = editor.classList.contains("editing");
  ok(!editorVisible, "editor popup hidden");
  is(div.getAttribute("burp"), "foobar", "`id` attribute-name *not* updated");
  is(attrNameNode_id.innerHTML, "burp", "attribute-value node in HTML panel *not* updated");

  
  executeSoon(finishUp);
}

function finishUp() {
  
  Services.obs.removeObserver(doNextStep, InspectorUI.INSPECTOR_NOTIFICATIONS.EDITOR_OPENED, false);
  Services.obs.removeObserver(doNextStep, InspectorUI.INSPECTOR_NOTIFICATIONS.EDITOR_CLOSED, false);
  Services.obs.removeObserver(doNextStep, InspectorUI.INSPECTOR_NOTIFICATIONS.EDITOR_SAVED, false);
  doc = div = null;
  InspectorUI.closeInspectorUI();
  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    doc = content.document;
    waitForFocus(setupEditorTests, content);
  }, true);

  content.location = "data:text/html,basic tests for html panel attribute-value editor";
}

