



let temp = [];
Cu.import("resource:///modules/devtools/Target.jsm", temp);
let TargetFactory = temp.TargetFactory;

function test() {
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    openAllTools();
  }, true);

  function openAllTools() {
    let target = TargetFactory.forTab(gBrowser.selectedTab);

    let tools = gDevTools.getToolDefinitions();
    let expectedCallbacksCount = tools.size;

    let firstTool = null;
    
    for (let [id] of tools) {
      if (!firstTool)
        firstTool = id;
      tools.set(id, false);
    }

    let toolbox = gDevTools.openToolbox(target, undefined, firstTool);

    
    for (let [toolId] of tools) {
      let id = toolId;
      info("Registering listener for " + id);
      tools.set(id, false);
      toolbox.on(id + "-ready", function(event, panel) {
        expectedCallbacksCount--;
        info("Got event "  + event);
        is(toolbox.getToolPanels().get(id), panel, "Got the right tool panel for " + id);
        tools.set(id, true);
        if (expectedCallbacksCount == 0) {
          
          
          executeSoon(theEnd);
        }
        if (expectedCallbacksCount < 0) {
          ok(false, "we are receiving too many events");
        }
      });
    }

    toolbox.once("ready", function() {
      
      for (let [id] of tools) {
        if (id != firstTool) {
          toolbox.selectTool(id);
        }
      }
    });

    function theEnd() {
      for (let [id, called] of tools) {
        ok(called, "Tool " + id + " has fired its ready event");
      }
      toolbox.destroy();
      gBrowser.removeCurrentTab();
      finish();
    }
  }
}
