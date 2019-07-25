







































let div;
let tab1;
let tab2;
let tab1window;

function inspectorTabOpen1()
{
  ok(InspectorUI, "InspectorUI variable exists");
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(InspectorStore.isEmpty(), "InspectorStore is empty");

  document.addEventListener("popupshown", inspectorUIOpen1, false);
  InspectorUI.toggleInspectorUI();
}

function inspectorUIOpen1(evt)
{
  if (evt.target.id != "inspector-style-panel") {
    return true;
  }

  document.removeEventListener(evt.type, arguments.callee, false);

  
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.isPanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  ok(!InspectorStore.isEmpty(), "InspectorStore is not empty");
  is(InspectorStore.length, 1, "InspectorStore.length = 1");

  
  div = content.document.getElementsByTagName("div")[0];
  InspectorUI.inspectNode(div);
  is(InspectorUI.treeView.selectedNode, div,
    "selection matches the div element");

  
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

  
  document.addEventListener("popupshown", inspectorUIOpen2, false);
  InspectorUI.toggleInspectorUI();
}

function inspectorUIOpen2(evt)
{
  if (evt.target.id != "inspector-style-panel") {
    return true;
  }

  document.removeEventListener(evt.type, arguments.callee, false);

  
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.isPanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  is(InspectorStore.length, 2, "InspectorStore.length = 2");

  
  InspectorUI.toggleInspection();
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");

  
  document.addEventListener("popupshown", inspectorFocusTab1, false);
  gBrowser.selectedTab = tab1;
}

function inspectorFocusTab1(evt)
{
  if (evt.target.id != "inspector-style-panel") {
    return true;
  }

  document.removeEventListener(evt.type, arguments.callee, false);

  
  ok(InspectorUI.inspecting, "Inspector is highlighting");
  ok(InspectorUI.isPanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  is(InspectorStore.length, 2, "InspectorStore.length = 2");
  is(InspectorUI.treeView.selectedNode, div,
    "selection matches the div element");

  
  document.addEventListener("popupshown", inspectorFocusTab2, false);
  gBrowser.selectedTab = tab2;
}

function inspectorFocusTab2(evt)
{
  if (evt.target.id != "inspector-style-panel") {
    return true;
  }

  document.removeEventListener(evt.type, arguments.callee, false);

  
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(InspectorUI.isPanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  is(InspectorStore.length, 2, "InspectorStore.length = 2");
  isnot(InspectorUI.treeView.selectedNode, div,
    "selection does not match the div element");

  
  tab1window = gBrowser.getBrowserForTab(tab1).contentWindow;
  tab1window.addEventListener("unload", inspectorTabUnload1, false);
  gBrowser.removeTab(tab1);
}

function inspectorTabUnload1(evt)
{
  tab1window.removeEventListener(evt.type, arguments.callee, false);
  tab1window = tab1 = tab2 = div = null;

  
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  ok(InspectorUI.isPanelOpen, "Inspector Tree Panel is open");
  ok(InspectorUI.isStylePanelOpen, "Inspector Style Panel is open");
  is(InspectorStore.length, 1, "InspectorStore.length = 1");

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

