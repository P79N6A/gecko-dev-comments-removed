







































let div;
let tab1;
let tab2;
let tab1window;

function inspectorTabOpen1()
{
  ok(InspectorUI, "InspectorUI variable exists");
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(InspectorStore.isEmpty(), "InspectorStore is empty");

  Services.obs.addObserver(inspectorUIOpen1, "inspector-opened", false);
  InspectorUI.openInspectorUI();
}

function inspectorUIOpen1()
{
  Services.obs.removeObserver(inspectorUIOpen1, "inspector-opened", false);

  
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  ok(!InspectorStore.isEmpty(), "InspectorStore is not empty");
  is(InspectorStore.length, 1, "InspectorStore.length = 1");

  
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
  ok(!InspectorUI.isPanelOpen, "Inspector Tree Panel is closed");
  ok(!InspectorUI.isStylePanelOpen, "Inspector Style Panel is closed");
  is(InspectorStore.length, 1, "InspectorStore.length = 1");

  
  Services.obs.addObserver(inspectorUIOpen2, "inspector-opened", false);
  InspectorUI.openInspectorUI();
}

function inspectorUIOpen2()
{
  Services.obs.removeObserver(inspectorUIOpen2, "inspector-opened", false);

  
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  is(InspectorStore.length, 2, "InspectorStore.length = 2");

  
  InspectorUI.toggleInspection();
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");

  
  Services.obs.addObserver(inspectorFocusTab1, "inspector-opened", false);
  gBrowser.selectedTab = tab1;
}

function inspectorFocusTab1()
{
  Services.obs.removeObserver(inspectorFocusTab1, "inspector-opened", false);

  
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  is(InspectorStore.length, 2, "InspectorStore.length = 2");
  is(InspectorUI.selection, div, "selection matches the div element");

  
  Services.obs.addObserver(inspectorFocusTab2, "inspector-opened", false);
  gBrowser.selectedTab = tab2;
}

function inspectorFocusTab2()
{
  Services.obs.removeObserver(inspectorFocusTab2, "inspector-opened", false);

  
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  is(InspectorStore.length, 2, "InspectorStore.length = 2");
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
  ok(InspectorUI.isTreePanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  is(InspectorStore.length, 1, "InspectorStore.length = 1");

  InspectorUI.closeInspectorUI();
  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();

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

