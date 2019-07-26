


let temp = {};
Cu.import("resource:///modules/devtools/Toolbox.jsm", temp);
let Toolbox = temp.Toolbox;
temp = null;

let toolbox, toolIDs, idIndex;

function test() {
  waitForExplicitFinish();

  if (window.navigator.userAgent.indexOf("Mac OS X 10.8") != -1 ||
      window.navigator.userAgent.indexOf("Windows NT 5.1") != -1) {
    info("Skipping Mac OSX 10.8 and Windows xp, see bug 838069");
    finish();
    return;
  }
  addTab("about:blank", function() {
    toolIDs = [];
    for (let [id, definition] of gDevTools._tools) {
      
      if (definition.key && id != "jsprofiler") {
        toolIDs.push(id);
      }
    }
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    idIndex = 0;
    gDevTools.showToolbox(target, toolIDs[0], Toolbox.HostType.WINDOW)
             .then(testShortcuts);
  });
}

function testShortcuts(aToolbox, aIndex) {
  if (aIndex == toolIDs.length) {
    tidyUp();
    return;
  }

  toolbox = aToolbox;
  info("Toolbox fired a `ready` event");

  toolbox.once("select", selectCB);

  if (aIndex != null) {
    
    
    

    let key = gDevTools._tools.get(toolIDs[aIndex]).key;
    let toolModifiers = gDevTools._tools.get(toolIDs[aIndex]).modifiers;
    let modifiers = {
      accelKey: toolModifiers.contains("accel"),
      altKey: toolModifiers.contains("alt"),
      shiftKey: toolModifiers.contains("shift"),
    };
    idIndex = aIndex;
    info("Testing shortcut for tool " + aIndex + ":" + toolIDs[aIndex] +
         " using key " + key);
    EventUtils.synthesizeKey(key, modifiers, toolbox.doc.defaultView.parent);
  }
}

function selectCB(event, id) {
  info("toolbox-select event from " + id);

  is(toolIDs.indexOf(id), idIndex,
     "Correct tool is selected on pressing the shortcut for " + id);

  testShortcuts(toolbox, idIndex + 1);
}

function tidyUp() {
  toolbox.destroy().then(function() {
    gBrowser.removeCurrentTab();

    toolbox = toolIDs = idIndex = Toolbox = null;
    finish();
  });
}
