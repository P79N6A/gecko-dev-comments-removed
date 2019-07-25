






































let doc;
let h1;
let p2;
let toolsLength = 0;
let toolEvents = 0;
let tool1;
let tool2;
let tool3;
let initToolsMethod = InspectorUI.initTools;

function createDocument()
{
  let div = doc.createElement("div");
  h1 = doc.createElement("h1");
  let p1 = doc.createElement("p");
  p2 = doc.createElement("p");
  let div2 = doc.createElement("div");
  let p3 = doc.createElement("p");
  doc.title = "Inspector Tree Selection Test";
  h1.textContent = "Inspector Tree Selection Test";
  p1.textContent = "This is some example text";
  p2.textContent = "Lorem ipsum dolor sit amet, consectetur adipisicing " +
    "elit, sed do eiusmod tempor incididunt ut labore et dolore magna " +
    "aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco " +
    "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure " +
    "dolor in reprehenderit in voluptate velit esse cillum dolore eu " +
    "fugiat nulla pariatur. Excepteur sint occaecat cupidatat non " +
    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
  p3.textContent = "Lorem ipsum dolor sit amet, consectetur adipisicing " +
    "elit, sed do eiusmod tempor incididunt ut labore et dolore magna " +
    "aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco " +
    "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure " +
    "dolor in reprehenderit in voluptate velit esse cillum dolore eu " +
    "fugiat nulla pariatur. Excepteur sint occaecat cupidatat non " +
    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
  div.appendChild(h1);
  div.appendChild(p1);
  div.appendChild(p2);
  div2.appendChild(p3);
  doc.body.appendChild(div);
  doc.body.appendChild(div2);
  setupHighlighterTests();
}

function setupHighlighterTests()
{
  ok(h1, "we have the header node");
  Services.obs.addObserver(inspectorOpen, INSPECTOR_NOTIFICATIONS.OPENED, false);
  registerTools();
  InspectorUI.toggleInspectorUI();
}

function inspectorOpen()
{
  info("we received the inspector-opened notification");
  Services.obs.removeObserver(inspectorOpen, INSPECTOR_NOTIFICATIONS.OPENED);
  toolsLength = InspectorUI.tools.length;
  toolEvents = InspectorUI.toolEvents.length;
  info("tools registered");
  Services.obs.addObserver(startToolTests, INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, false);
  InspectorUI.inspectNode(h1);
}

function startToolTests(evt)
{
  Services.obs.removeObserver(startToolTests, INSPECTOR_NOTIFICATIONS.HIGHLIGHTING);
  InspectorUI.stopInspecting();
  info("Getting InspectorUI.tools");
  let tools = InspectorUI.tools;

  tool1 = InspectorUI.tools["tool_1"];
  tool2 = InspectorUI.tools["tool_2"];
  tool3 = InspectorUI.tools["tool_3"];

  info("Checking panel states 1");
  ok(!tool1.isOpen, "Panel 1 is closed");
  ok(!tool2.isOpen, "Panel 2 is closed");
  ok(!tool3.isOpen, "Panel 3 is closed");

  info("Calling show method for all tools");
  InspectorUI.toolShow(tool1);
  InspectorUI.toolShow(tool2);
  InspectorUI.toolShow(tool3);

  info("Checking panel states 2");
  ok(tool1.isOpen, "Panel 1 is open");
  ok(tool2.isOpen, "Panel 2 is open");
  ok(tool3.isOpen, "Panel 3 is open");

  info("Calling selectNode method for all tools, should see 3 selects");
  InspectorUI.inspectNode(p2);

  info("Calling hide method for all tools");
  InspectorUI.toolHide(tool1);
  InspectorUI.toolHide(tool2);
  InspectorUI.toolHide(tool3);
  
  info("Checking panel states 3");
  ok(!tool1.isOpen, "Panel 1 is closed");
  ok(!tool2.isOpen, "Panel 2 is closed");
  ok(!tool3.isOpen, "Panel 3 is closed");

  info("Showing tools 1 & 3");
  InspectorUI.toolShow(tool1);
  InspectorUI.toolShow(tool3);

  info("Checking panel states 4");
  ok(tool1.isOpen, "Panel 1 is open");
  ok(!tool2.isOpen, "Panel 2 is closed");
  ok(tool3.isOpen, "Panel 3 is open");

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    waitForFocus(testSecondTab, content);
  }, true);

  content.location = "data:text/html,registertool new tab test for inspector";
}

function testSecondTab()
{
  info("Opened second tab");
  info("Checking panel states 5");

  let tools = InspectorUI.tools;
  ok(!(tool1 in tools), "Panel 1 not in tools");
  ok(!(tool2 in tools), "Panel 2 not in tools");
  ok(!(tool3 in tools), "Panel 3 not in tools");

  info("Closing current tab");
  Services.obs.addObserver(testOriginalTab, INSPECTOR_NOTIFICATIONS.OPENED, false);
  gBrowser.removeCurrentTab();
}

function testOriginalTab()
{
  Services.obs.removeObserver(testOriginalTab, INSPECTOR_NOTIFICATIONS.OPENED);
  info("Checking panel states 6");

  info("Tools: " + InspectorUI.tools);
  
  tool1 = InspectorUI.tools["tool_1"];
  tool2 = InspectorUI.tools["tool_2"];
  tool3 = InspectorUI.tools["tool_3"];

  ok(tool1.isOpen, "Panel 1 is open after reactivation");
  ok(!tool2.isOpen, "Panel 2 is closed after reactivation");
  ok(tool3.isOpen, "Panel 3 is open after reactivation");

  Services.obs.addObserver(unregisterTools, INSPECTOR_NOTIFICATIONS.CLOSED, false);
  InspectorUI.closeInspectorUI(true);
}

function unregisterTools()
{
  Services.obs.removeObserver(unregisterTools, INSPECTOR_NOTIFICATIONS.CLOSED);
  let tools = InspectorUI.tools;

  ok(!(tool1 in tools), "Tool 1 removed");
  ok(!(tool2 in tools), "Tool 2 removed");
  ok(!(tool3 in tools), "Tool 3 removed");
  is(tools.length, toolsLength, "Number of Registered Tools matches original");
  is(InspectorUI.toolEvents.length, toolEvents, "Number of tool events matches original");
  finishUp();
}

function finishUp() {
  gBrowser.removeCurrentTab();
  InspectorUI.initTools = initToolsMethod;
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

  content.location = "data:text/html,registertool tests for inspector";
}

function registerTools()
{
  InspectorUI.initTools = function() {
    info("(re)registering tools");
    registerTool(new testTool("tool_1", "Tool 1", "Tool 1 tooltip", "I"));
    registerTool(new testTool("tool_2", "Tool 2", "Tool 2 tooltip", "J"));
    registerTool(new testTool("tool_3", "Tool 3", "Tool 3 tooltip", "K"));
  }
}

function registerTool(aTool)
{
  InspectorUI.registerTool({
    id: aTool.id,
    label: aTool.label,
    tooltiptext: aTool.tooltip,
    accesskey: aTool.accesskey,
    context: aTool,
    get isOpen() aTool.isOpen(),
    onSelect: aTool.selectNode,
    show: aTool.show,
    hide: aTool.hide,
    unregister: aTool.destroy,
  });
}


function testTool(aToolId, aLabel, aTooltip, aAccesskey)
{
  this.id = aToolId;
  this.label = aLabel;
  this.tooltip = aTooltip;
  this.accesskey = aAccesskey;
  this._isOpen = false;
}

testTool.prototype = {
  isOpen: function BIR_isOpen() {
    return this._isOpen;
  },

  selectNode: function BIR_selectNode(aNode) {
    is(InspectorUI.selection, aNode,
       "selectNode: currently selected node was passed: " + this.id);
  },

  show: function BIR_show(aNode) {
    this._isOpen = true;
    is(InspectorUI.selection, aNode,
       "show: currently selected node was passed: " + this.id);
  },

  hide: function BIR_hide() {
    info(this.id + " hide");
    this._isOpen = false;
  },

  destroy: function BIR_destroy() {
    info("tool destroyed " + this.id);
    if (this.isOpen())
      this.hide();
    delete this.id;
    delete this.label;
    delete this.tooltip;
    delete this.accesskey;
  },
};
