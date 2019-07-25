







































let div;
let tab1;
let tab2;
let tab1window;

function inspectorTabOpen1()
{
  ok(window.InspectorUI, "InspectorUI variable exists");
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(InspectorUI.store.isEmpty(), "Inspector.store is empty");

  Services.obs.addObserver(inspectorUIOpen1,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.openInspectorUI();
}

function inspectorUIOpen1()
{
  Services.obs.removeObserver(inspectorUIOpen1,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);

  
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(!InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is not open");
  ok(!InspectorUI.isSidebarOpen, "Inspector Sidebar is not open");
  ok(!InspectorUI.store.isEmpty(), "InspectorUI.store is not empty");
  is(InspectorUI.store.length, 1, "Inspector.store.length = 1");

  
  div = content.document.getElementsByTagName("div")[0];
  InspectorUI.inspectNode(div);
  is(InspectorUI.selection, div, "selection matches the div element");

  
  tab2 = gBrowser.addTab();
  gBrowser.selectedTab = tab2;

  gBrowser.selectedBrowser.addEventListener("load", function(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, arguments.callee,
      true);
    waitForFocus(inspectorTabOpen2, content);
  }, true);

  content.location = "data:text/html,<p>tab 2: the inspector should close now";
}

function inspectorTabOpen2()
{
  
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(!InspectorUI.treePanel, "Inspector Tree Panel is closed");
  ok(!InspectorUI.isSidebarOpen, "Inspector Sidebar is not open");
  is(InspectorUI.store.length, 1, "Inspector.store.length = 1");

  
  executeSoon(function() {
    Services.obs.addObserver(inspectorUIOpen2,
      InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
    InspectorUI.openInspectorUI();
  });
}

function inspectorUIOpen2()
{
  Services.obs.removeObserver(inspectorUIOpen2,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);

  
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(!InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is not open");
  is(InspectorUI.store.length, 2, "Inspector.store.length = 2");

  
  InspectorUI.toggleInspection();
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");

  
  executeSoon(function() {
    Services.obs.addObserver(inspectorFocusTab1,
      InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
    gBrowser.selectedTab = tab1;
  });
}

function inspectorFocusTab1()
{
  Services.obs.removeObserver(inspectorFocusTab1,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);

  
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(!InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is not open");
  is(InspectorUI.store.length, 2, "Inspector.store.length = 2");
  is(InspectorUI.selection, div, "selection matches the div element");

  Services.obs.addObserver(inspectorOpenTreePanelTab1,
    InspectorUI.INSPECTOR_NOTIFICATIONS.TREEPANELREADY, false);

  InspectorUI.treePanel.open();
}

function inspectorOpenTreePanelTab1()
{
  Services.obs.removeObserver(inspectorOpenTreePanelTab1,
    InspectorUI.INSPECTOR_NOTIFICATIONS.TREEPANELREADY);

  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is open");
  is(InspectorUI.store.length, 2, "Inspector.store.length = 2");
  is(InspectorUI.selection, div, "selection matches the div element");

  Services.obs.addObserver(inspectorSidebarStyleView1, "StyleInspector-opened", false);

  executeSoon(function() {
    InspectorUI.showSidebar();
    InspectorUI.toolShow(InspectorUI.stylePanel.registrationObject);
  });
}

function inspectorSidebarStyleView1()
{
  Services.obs.removeObserver(inspectorSidebarStyleView1, "StyleInspector-opened");
  ok(InspectorUI.isSidebarOpen, "Inspector Sidebar is open");
  ok(InspectorUI.stylePanel, "Inspector Has a Style Panel Instance");
  InspectorUI.sidebarTools.forEach(function(aTool) {
    let btn = document.getElementById(InspectorUI.getToolbarButtonId(aTool.id));
    is(btn.hasAttribute("checked"),
      (aTool == InspectorUI.stylePanel.registrationObject),
      "Button " + btn.id + " has correct checked attribute");
  });

  
  Services.obs.addObserver(inspectorFocusTab2,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  gBrowser.selectedTab = tab2;
}

function inspectorFocusTab2()
{
  Services.obs.removeObserver(inspectorFocusTab2,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);

  
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(!InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is not open");
  ok(!InspectorUI.isSidebarOpen, "Inspector Sidebar is not open");
  is(InspectorUI.store.length, 2, "Inspector.store.length is 2");
  isnot(InspectorUI.selection, div, "selection does not match the div element");

  
  EventUtils.synthesizeKey("VK_RETURN", { });

  executeSoon(function() {
    ok(InspectorUI.inspecting, "Inspector is highlighting");
    InspectorUI.toggleInspection();

    
    Services.obs.addObserver(inspectorSecondFocusTab1,
      InspectorUI.INSPECTOR_NOTIFICATIONS.TREEPANELREADY, false);
    gBrowser.selectedTab = tab1;
  });
}

function inspectorSecondFocusTab1()
{
  Services.obs.removeObserver(inspectorSecondFocusTab1,
    InspectorUI.INSPECTOR_NOTIFICATIONS.TREEPANELREADY);

  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is open");
  is(InspectorUI.store.length, 2, "Inspector.store.length = 2");
  is(InspectorUI.selection, div, "selection matches the div element");

  ok(InspectorUI.isSidebarOpen, "Inspector Sidebar is open");
  ok(InspectorUI.stylePanel, "Inspector Has a Style Panel Instance");
  InspectorUI.sidebarTools.forEach(function(aTool) {
    let btn = document.getElementById(InspectorUI.getToolbarButtonId(aTool.id));
    is(btn.hasAttribute("checked"),
      (aTool == InspectorUI.stylePanel.registrationObject),
      "Button " + btn.id + " has correct checked attribute");
  });

  
  Services.obs.addObserver(inspectorSecondFocusTab2,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  gBrowser.selectedTab = tab2;
}

function inspectorSecondFocusTab2()
{
  Services.obs.removeObserver(inspectorSecondFocusTab2,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED);

  
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(!InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is not open");
  ok(!InspectorUI.isSidebarOpen, "Inspector Sidebar is not open");

  is(InspectorUI.store.length, 2, "Inspector.store.length is 2");
  isnot(InspectorUI.selection, div, "selection does not match the div element");

  
  tab1window = gBrowser.getBrowserForTab(tab1).contentWindow;
  tab1window.addEventListener("pagehide", inspectorTabUnload1, false);
  gBrowser.removeTab(tab1);
}

function inspectorTabUnload1(evt)
{
  tab1window.removeEventListener(evt.type, arguments.callee, false);
  tab1window = tab1 = tab2 = div = null;

  
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(!InspectorUI.treePanel.isOpen(), "Inspector Tree Panel is not open");
  is(InspectorUI.store.length, 1, "Inspector.store.length = 1");

  InspectorUI.closeInspectorUI();
  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();
  ignoreAllUncaughtExceptions();

  tab1 = gBrowser.addTab();
  gBrowser.selectedTab = tab1;
  gBrowser.selectedBrowser.addEventListener("load", function(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, arguments.callee,
      true);
    waitForFocus(inspectorTabOpen1, content);
  }, true);

  content.location = "data:text/html,<p>tab switching tests for inspector" +
    "<div>tab 1</div>";
}

