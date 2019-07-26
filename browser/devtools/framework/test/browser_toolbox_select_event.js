


let toolbox;

function test() {
  addTab("about:blank", function() {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    toolbox = gDevTools.openToolbox(target, "bottom", "webconsole");
    toolbox.once("ready", testSelect);
  });
}

let called = {
  inspector: false,
  webconsole: false,
  styleeditor: false,
  
}

function testSelect() {
  info("Toolbox fired a `ready` event");

  toolbox.on("select", selectCB);

  toolbox.selectTool("inspector");
  toolbox.selectTool("webconsole");
  toolbox.selectTool("styleeditor");
  
}

function selectCB(event, id) {
  called[id] = true;
  info("toolbox-select event from " + id);

  for (let tool in called) {
    if (!called[tool]) {
      return;
    }
  }

  ok(true, "All the tools fired a 'select event'");
  toolbox.off("select", selectCB);

  reselect();
}

function reselect() {
  for (let tool in called) {
    called[tool] = false;
  }

  toolbox.once("inspector-selected", function() {
    tidyUpIfAllCalled("inspector");
  });

  toolbox.once("webconsole-selected", function() {
    tidyUpIfAllCalled("webconsole");
  });

  





  toolbox.once("styleeditor-selected", function() {
    tidyUpIfAllCalled("styleeditor");
  });

  toolbox.selectTool("inspector");
  toolbox.selectTool("webconsole");
  toolbox.selectTool("styleeditor");
  
}

function tidyUpIfAllCalled(id) {
  called[id] = true;
  info("select event from " + id);

  for (let tool in called) {
    if (!called[tool]) {
      return;
    }
  }

  ok(true, "All the tools fired a {id}-selected event");
  tidyUp();
}

function tidyUp() {
  toolbox.destroy();
  gBrowser.removeCurrentTab();

  toolbox = null;
  finish();
}
