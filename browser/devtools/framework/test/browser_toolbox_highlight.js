


let Toolbox = devtools.Toolbox;
let temp = {};
Cu.import("resource://gre/modules/Services.jsm", temp);
let Services = temp.Services;
temp = null;
let toolbox = null;

function test() {
  const URL = "data:text/plain;charset=UTF-8,Nothing to see here, move along";

  const TOOL_ID_1 = "jsdebugger";
  const TOOL_ID_2 = "webconsole";

  addTab(URL).then(() => {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    gDevTools.showToolbox(target, TOOL_ID_1, Toolbox.HostType.BOTTOM)
             .then(aToolbox => {
                toolbox = aToolbox;
                
                toolbox.selectTool(TOOL_ID_2)
                       
                       .then(highlightTab.bind(null, TOOL_ID_1))
                       
                       .then(checkHighlighted.bind(null, TOOL_ID_1))
                       
                       .then(() => toolbox.selectTool(TOOL_ID_1))
                       
                       
                       .then(checkNoHighlightWhenSelected.bind(null, TOOL_ID_1))
                       
                       .then(() => toolbox.selectTool(TOOL_ID_2))
                       
                       .then(checkHighlighted.bind(null, TOOL_ID_1))
                       
                       .then(unhighlightTab.bind(null, TOOL_ID_1))
                       
                       .then(checkNoHighlight.bind(null, TOOL_ID_1))
                       
                       .then(() => executeSoon(() => {
                          toolbox.destroy()
                                 .then(() => {
                                   toolbox = null;
                                   gBrowser.removeCurrentTab();
                                   finish();
                                 });
                        }));
              });
  });
}

function highlightTab(toolId) {
  info("Highlighting tool " + toolId + "'s tab.");
  toolbox.highlightTool(toolId);
}

function unhighlightTab(toolId) {
  info("Unhighlighting tool " + toolId + "'s tab.");
  toolbox.unhighlightTool(toolId);
}

function checkHighlighted(toolId) {
  let tab = toolbox.doc.getElementById("toolbox-tab-" + toolId);
  ok(tab.hasAttribute("highlighted"), "The highlighted attribute is present");
  ok(!tab.hasAttribute("selected") || tab.getAttribute("selected") != "true",
     "The tab is not selected");
}

function checkNoHighlightWhenSelected(toolId) {
  let tab = toolbox.doc.getElementById("toolbox-tab-" + toolId);
  ok(tab.hasAttribute("highlighted"), "The highlighted attribute is present");
  ok(tab.hasAttribute("selected") && tab.getAttribute("selected") == "true",
     "and the tab is selected, so the orange glow will not be present.");
}

function checkNoHighlight(toolId) {
  let tab = toolbox.doc.getElementById("toolbox-tab-" + toolId);
  ok(!tab.hasAttribute("highlighted"),
     "The highlighted attribute is not present");
}
