





































let doc;
let salutation;
let closing;

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
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.toggleInspectorUI();
}

function runInspectorTests()
{
  Services.obs.removeObserver(runInspectorTests,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED);
  Services.obs.addObserver(treePanelTests,
    InspectorUI.INSPECTOR_NOTIFICATIONS.TREEPANELREADY, false);

  ok(InspectorUI.toolbar, "we have the toolbar.");
  ok(!InspectorUI.toolbar.hidden, "toolbar is visible");
  ok(InspectorUI.inspecting, "Inspector is inspecting");
  ok(!InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is not open");
  ok(!InspectorUI.isSidebarOpen, "Inspector Sidebar is not open");
  ok(InspectorUI.highlighter, "Highlighter is up");
  InspectorUI.inspectNode(doc.body);
  InspectorUI.stopInspecting();

  InspectorUI.treePanel.open();
}

function treePanelTests()
{
  Services.obs.removeObserver(treePanelTests,
    InspectorUI.INSPECTOR_NOTIFICATIONS.TREEPANELREADY);
  Services.obs.addObserver(stylePanelTests,
    "StyleInspector-opened", false);

  ok(InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is open");

  executeSoon(function() {
    InspectorUI.showSidebar();
    document.getElementById(InspectorUI.getToolbarButtonId("styleinspector")).click();
  });
}

function stylePanelTests()
{
  Services.obs.removeObserver(stylePanelTests, "StyleInspector-opened");

  ok(InspectorUI.isSidebarOpen, "Inspector Sidebar is open");
  ok(InspectorUI.stylePanel.cssHtmlTree, "Style Panel has a cssHtmlTree");

  InspectorUI.ruleButton.click();
  executeSoon(function() {
    ruleViewTests();
  });
}

function ruleViewTests()
{
  Services.obs.addObserver(runContextMenuTest,
      InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);

  ok(InspectorUI.isRuleViewOpen(), "Rule View is open");
  ok(InspectorUI.ruleView, "InspectorUI has a cssRuleView");

  executeSoon(function() {
    InspectorUI.closeInspectorUI();
  });
}

function runContextMenuTest()
{
  Services.obs.removeObserver(runContextMenuTest, InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
  Services.obs.addObserver(inspectNodesFromContextTest, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  salutation = doc.getElementById("salutation");
  ok(salutation, "hello, context menu test!");
  let eventDeets = { type : "contextmenu", button : 2 };
  let contextMenu = document.getElementById("contentAreaContextMenu");
  ok(contextMenu, "we have the context menu");
  let contextInspectMenuItem = document.getElementById("context-inspect");
  ok(contextInspectMenuItem, "we have the inspect context menu item");
  EventUtils.synthesizeMouse(salutation, 2, 2, eventDeets);
  is(contextMenu.state, "showing", "context menu is open");
  is(!contextInspectMenuItem.hidden, gPrefService.getBoolPref("devtools.inspector.enabled"), "is context menu item enabled?");
  contextMenu.hidePopup();
  executeSoon(function() {
    InspectorUI.openInspectorUI(salutation);
  });
}

function inspectNodesFromContextTest()
{
  Services.obs.removeObserver(inspectNodesFromContextTest, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  Services.obs.addObserver(openInspectorForContextTest, InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
  ok(!InspectorUI.inspecting, "Inspector is not actively highlighting");
  is(InspectorUI.selection, salutation, "Inspector is highlighting salutation");
  ok(!InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is closed");
  ok(!InspectorUI.stylePanel.isOpen(), "Inspector Style Panel is closed");
  executeSoon(function() {
    InspectorUI.closeInspectorUI(true);
  });
}

function openInspectorForContextTest()
{
  Services.obs.removeObserver(openInspectorForContextTest, InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED);
  Services.obs.addObserver(inspectNodesFromContextTestWhileOpen, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  executeSoon(function() {
    InspectorUI.openInspectorUI(salutation);
  });
}

function inspectNodesFromContextTestWhileOpen()
{
  Services.obs.removeObserver(inspectNodesFromContextTestWhileOpen, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED);
  Services.obs.addObserver(inspectNodesFromContextTestTrap, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  Services.obs.addObserver(inspectNodesFromContextTestHighlight, InspectorUI.INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, false);
  is(InspectorUI.selection, salutation, "Inspector is highlighting salutation");
  closing = doc.getElementById("closing");
  ok(closing, "we have the closing statement");
  executeSoon(function() {
    InspectorUI.openInspectorUI(closing);
  });
}

function inspectNodesFromContextTestHighlight()
{
  Services.obs.removeObserver(inspectNodesFromContextTestHighlight, InspectorUI.INSPECTOR_NOTIFICATIONS.HIGHLIGHTING);
  Services.obs.addObserver(finishInspectorTests, InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
  is(InspectorUI.selection, closing, "InspectorUI.selection is header");
  executeSoon(function() {
    InspectorUI.closeInspectorUI(true);
  });
}

function inspectNodesFromContextTestTrap()
{
  Services.obs.removeObserver(inspectNodesFromContextTestTrap, InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED);
  ok(false, "Inspector UI has been opened again. We Should Not Be Here!");
}

function finishInspectorTests()
{
  Services.obs.removeObserver(finishInspectorTests,
    InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED);

  ok(!InspectorUI.highlighter, "Highlighter is gone");
  ok(!InspectorUI.treePanel, "Inspector Tree Panel is closed");
  ok(!InspectorUI.inspecting, "Inspector is not inspecting");
  ok(!InspectorUI.isSidebarOpen, "Inspector Sidebar is closed");
  ok(!InspectorUI.stylePanel, "Inspector Style Panel is gone");
  ok(!InspectorUI.ruleView, "Inspector Rule View is gone");
  is(InspectorUI.sidebarToolbar.children.length, 0, "No items in the Sidebar toolbar");
  is(InspectorUI.sidebarDeck.children.length, 0, "No items in the Sidebar deck");
  ok(!InspectorUI.toolbar, "toolbar is hidden");

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

