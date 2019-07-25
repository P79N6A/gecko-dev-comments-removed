





































let doc;
let salutation;

function createDocument()
{
  doc.body.innerHTML = '<div id="first" style="{ margin: 10em; ' +
    'font-size: 14pt; font-family: helvetica, sans-serif; color: #AAA}">\n' +
    '<h1>Some header text</h1>\n' +
    '<p id="salutation" style="{font-size: 12pt}">hi.</p>\n' +
    '<p id="body" style="{font-size: 12pt}">I am a test-case. This text exists ' +
    'solely to provide some things to test the inspector initialization.</p>\n' +
    'If you are reading this, you should go do something else instead. Maybe ' +
    'read a book. Or better yet, write some test-cases for another bit of code. ' +
    '<span style="{font-style: italic}">Maybe more inspector test-cases!</span></p>\n' +
    '<p id="closing">end transmission</p>\n' +
    '</div>';
  doc.title = "Inspector Initialization Test";
  startInspectorTests();
}

function startInspectorTests()
{
  ok(InspectorUI, "InspectorUI variable exists");
  Services.obs.addObserver(runInspectorTests,
    INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.toggleInspectorUI();
}

function runInspectorTests()
{
  Services.obs.removeObserver(runInspectorTests,
    INSPECTOR_NOTIFICATIONS.OPENED, false);
  Services.obs.addObserver(runContextMenuTest,
    INSPECTOR_NOTIFICATIONS.CLOSED, false);

  ok(!InspectorUI.toolbar.hidden, "toolbar is visible");
  let iframe = document.getElementById("inspector-tree-iframe");
  is(InspectorUI.treeIFrame, iframe, "Inspector IFrame matches");
  ok(InspectorUI.inspecting, "Inspector is inspecting");
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.highlighter, "Highlighter is up");

  executeSoon(function() {
    InspectorUI.closeInspectorUI();
  });
}

function runContextMenuTest()
{
  Services.obs.removeObserver(runContextMenuTest, INSPECTOR_NOTIFICATIONS.CLOSED, false);
  Services.obs.addObserver(inspectNodesFromContextTest, INSPECTOR_NOTIFICATIONS.OPENED, false);
  salutation = doc.getElementById("salutation");
  ok(salutation, "hello, context menu test!");
  let eventDeets = { type : "contextmenu", button : 2 };
  let contextMenu = document.getElementById("contentAreaContextMenu");
  ok(contextMenu, "we have the context menu");
  let contextInspectMenuItem = document.getElementById("context-inspect");
  ok(contextInspectMenuItem, "we have the inspect context menu item");
  EventUtils.synthesizeMouse(salutation, 2, 2, eventDeets);
  is(contextMenu.state, "showing", "context menu is open");
  is(contextInspectMenuItem.hidden, !InspectorUI.enabled, "is context menu item enabled?");
  contextMenu.hidePopup();
  executeSoon(function() {
    InspectorUI.openInspectorUI(salutation);
  });
}

function inspectNodesFromContextTest()
{
  Services.obs.removeObserver(inspectNodesFromContextTest, INSPECTOR_NOTIFICATIONS.OPENED, false);
  Services.obs.addObserver(finishInspectorTests, INSPECTOR_NOTIFICATIONS.CLOSED, false);
  ok(!InspectorUI.inspecting, "Inspector is not actively highlighting");
  is(InspectorUI.selection, salutation, "Inspector is highlighting salutation");
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  
  todo(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  todo(InspectorUI.isDOMPanelOpen, "Inspector DOM Panel is open");
  InspectorUI.closeInspectorUI(true);
}

function finishInspectorTests()
{
  Services.obs.removeObserver(finishInspectorTests,
    INSPECTOR_NOTIFICATIONS.CLOSED, false);

  ok(!InspectorUI.highlighter, "Highlighter is gone");
  ok(!InspectorUI.isTreePanelOpen, "Inspector Tree Panel is closed");
  ok(!InspectorUI.inspecting, "Inspector is not inspecting");
  ok(InspectorUI.toolbar.hidden, "toolbar is hidden");

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
    waitForFocus(createDocument, content);
  }, true);

  content.location = "data:text/html,basic tests for inspector";
}

