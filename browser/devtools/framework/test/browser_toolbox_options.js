


let doc = null, toolbox = null, panelWin = null, modifiedPrefs = [];

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    gDevTools.showToolbox(target)
      .then(testSelectTool)
      .then(testOptionsShortcut)
      .then(testOptions)
      .then(testToggleTools)
      .then(cleanup, errorHandler);
  }, true);

  content.location = "data:text/html;charset=utf8,test for dynamically registering and unregistering tools";
}

function testSelectTool(aToolbox) {
  let deferred = promise.defer();

  toolbox = aToolbox;
  doc = toolbox.doc;
  toolbox.once("options-selected", () => {
    ok(true, "Toolbox selected via selectTool method");
    deferred.resolve();
  });
  toolbox.selectTool("options");

  return deferred.promise;
}

function testOptionsShortcut() {
  let deferred = promise.defer();

  toolbox.once("options-selected", (event, tool) => {
    ok(true, "Toolbox selected via shortcut key");
    deferred.resolve(tool);
  });
  toolbox.selectTool("webconsole")
         .then(() => synthesizeKeyFromKeyTag("toolbox-options-key", doc));

  return deferred.promise;
}

function testOptions(tool) {
  panelWin = tool.panelWin;
  let prefNodes = tool.panelDoc.querySelectorAll("checkbox[data-pref]");

  
  for (let node of prefNodes) {
    let pref = node.getAttribute("data-pref");
    modifiedPrefs.push(pref);
  }

  
  let p = promise.resolve();
  for (let node of prefNodes) {
    let prefValue = Services.prefs.getBoolPref(node.getAttribute("data-pref"));
    p = p.then(testMouseClick.bind(null, node, prefValue));
  }
  
  for (let node of prefNodes) {
    let prefValue = !Services.prefs.getBoolPref(node.getAttribute("data-pref"));
    p = p.then(testMouseClick.bind(null, node, prefValue));
  }

  return p;
}

function testMouseClick(node, prefValue) {
  let deferred = promise.defer();

  let pref = node.getAttribute("data-pref");
  gDevTools.once("pref-changed", (event, data) => {
    if (data.pref == pref) {
      ok(true, "Correct pref was changed");
      is(data.oldValue, prefValue, "Previous value is correct");
      is(data.newValue, !prefValue, "New value is correct");
    } else {
      ok(false, "Pref " + pref + " was not changed correctly");
    }
    deferred.resolve();
  });

  node.scrollIntoView();

  
  
  executeSoon(function() {
    info("Click event synthesized for pref " + pref);
    EventUtils.synthesizeMouseAtCenter(node, {}, panelWin);
  });

  return deferred.promise;
}

function testToggleTools() {
  let toolNodes = panelWin.document.querySelectorAll("#default-tools-box > checkbox:not([unsupported])");
  let enabledTools = Array.prototype.filter.call(toolNodes, node => node.checked);

  
  for (let tool of gDevTools.getDefaultTools()) {
    let pref = tool.visibilityswitch;
    modifiedPrefs.push(pref);
  }

  
  let p = promise.resolve();
  for (let node of toolNodes) {
    p = p.then(toggleTool.bind(null, node));
  }
  
  for (let node of toolNodes) {
    p = p.then(toggleTool.bind(null, node));
  }

  
  
  for (let node of enabledTools) {
    p = p.then(toggleTool.bind(null, node));
  }
  
  for (let node of enabledTools) {
    p = p.then(toggleTool.bind(null, node));
  }

  
  
  let firstTool  = toolNodes[0],
      middleTool = toolNodes[(toolNodes.length / 2) | 0],
      lastTool   = toolNodes[toolNodes.length - 1];

  p = p.then(toggleTool.bind(null, firstTool))
       .then(toggleTool.bind(null, firstTool))
       .then(toggleTool.bind(null, middleTool))
       .then(toggleTool.bind(null, middleTool))
       .then(toggleTool.bind(null, lastTool))
       .then(toggleTool.bind(null, lastTool));

  return p;
}

function toggleTool(node) {
  let deferred = promise.defer();

  let toolId = node.getAttribute("id");
  if (node.checked) {
    gDevTools.once("tool-unregistered", checkUnregistered.bind(null, toolId, deferred));
  } else {
    gDevTools.once("tool-registered", checkRegistered.bind(null, toolId, deferred));
  }
  node.scrollIntoView();
  EventUtils.synthesizeMouseAtCenter(node, {}, panelWin);

  return deferred.promise;
}

function checkUnregistered(toolId, deferred, event, data) {
  if (data.id == toolId) {
    ok(true, "Correct tool removed");
    
    ok(!doc.getElementById("toolbox-tab-" + toolId), "Tab removed for " + toolId);
  } else {
    ok(false, "Something went wrong, " + toolId + " was not unregistered");
  }
  deferred.resolve();
}

function checkRegistered(toolId, deferred, event, data) {
  if (data == toolId) {
    ok(true, "Correct tool added back");
    
    let radio = doc.getElementById("toolbox-tab-" + toolId);
    ok(radio, "Tab added back for " + toolId);
    if (radio.previousSibling) {
      ok(+radio.getAttribute("ordinal") >=
         +radio.previousSibling.getAttribute("ordinal"),
         "Inserted tab's ordinal is greater than equal to its previous tab." +
         "Expected " + radio.getAttribute("ordinal") + " >= " +
         radio.previousSibling.getAttribute("ordinal"));
    }
    if (radio.nextSibling) {
      ok(+radio.getAttribute("ordinal") <
         +radio.nextSibling.getAttribute("ordinal"),
         "Inserted tab's ordinal is less than its next tab. Expected " +
         radio.getAttribute("ordinal") + " < " +
         radio.nextSibling.getAttribute("ordinal"));
    }
  } else {
    ok(false, "Something went wrong, " + toolId + " was not registered");
  }
  deferred.resolve();
}

function cleanup() {
  toolbox.destroy().then(function() {
    gBrowser.removeCurrentTab();
    for (let pref of modifiedPrefs) {
      Services.prefs.clearUserPref(pref);
    }
    toolbox = doc = panelWin = modifiedPrefs = null;
    finish();
  });
}

function errorHandler(error) {
  ok(false, "Unexpected error: " + error);
  cleanup();
}
