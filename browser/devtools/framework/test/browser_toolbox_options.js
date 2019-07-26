


let doc = null, toolbox = null, panelWin = null, index = 0, prefValues = [], prefNodes = [];

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    gDevTools.showToolbox(target).then(testSelectTool);
  }, true);

  content.location = "data:text/html;charset=utf8,test for dynamically registering and unregistering tools";
}

function testSelectTool(aToolbox) {
  toolbox = aToolbox;
  doc = toolbox.doc;
  toolbox.once("options-selected", testOptionsShortcut);
  toolbox.selectTool("options");
}

function testOptionsShortcut() {
  ok(true, "Toolbox selected via selectTool method");
  toolbox.once("options-selected", testOptions);
  toolbox.selectTool("webconsole")
         .then(() => synthesizeKeyFromKeyTag("toolbox-options-key", doc));
}

function testOptions(event, tool) {
  ok(true, "Toolbox selected via button click");
  panelWin = tool.panelWin;
  
  let prefCheckboxes = tool.panelDoc.querySelectorAll("checkbox[data-pref]");
  for (let checkbox of prefCheckboxes) {
    prefNodes.push(checkbox);
    prefValues.push(Services.prefs.getBoolPref(checkbox.getAttribute("data-pref")));
  }
  
  for (let checkbox of prefCheckboxes) {
    prefNodes.push(checkbox);
    prefValues.push(!Services.prefs.getBoolPref(checkbox.getAttribute("data-pref")));
  }
  testMouseClicks();
}

function testMouseClicks() {
  if (index == prefValues.length) {
    checkTools();
    return;
  }
  gDevTools.once("pref-changed", prefChanged);
  info("Click event synthesized for index " + index);
  prefNodes[index].scrollIntoView();

  
  
  executeSoon(function() {
    EventUtils.synthesizeMouseAtCenter(prefNodes[index], {}, panelWin);
  });
}

function prefChanged(event, data) {
  if (data.pref == prefNodes[index].getAttribute("data-pref")) {
    ok(true, "Correct pref was changed");
    is(data.oldValue, prefValues[index], "Previous value is correct");
    is(data.newValue, !prefValues[index], "New value is correct");
    index++;
    testMouseClicks();
    return;
  }
  ok(false, "Pref was not changed correctly");
  cleanup();
}

function checkTools() {
  let toolsPref = panelWin.document.querySelectorAll("#default-tools-box > checkbox:not([unsupported])");
  prefNodes = [];
  index = 0;
  for (let tool of toolsPref) {
    prefNodes.push(tool);
  }
  
  
  prefNodes = prefNodes.sort(() => Math.random() > 0.5 ? 1: -1);

  
  executeSoon(toggleTools);
}

function toggleTools() {
  if (index < prefNodes.length) {
    gDevTools.once("tool-unregistered", checkUnregistered);
    let node = prefNodes[index];
    node.scrollIntoView();
    EventUtils.synthesizeMouseAtCenter(node, {}, panelWin);
  }
  else if (index < 2*prefNodes.length) {
    gDevTools.once("tool-registered", checkRegistered);
    let node = prefNodes[index - prefNodes.length];
    node.scrollIntoView();
    EventUtils.synthesizeMouseAtCenter(node, {}, panelWin);
  }
  else {
    cleanup();
  }
}

function checkUnregistered(event, data) {
  if (data.id == prefNodes[index].getAttribute("id")) {
    ok(true, "Correct tool removed");
    
    ok(!doc.getElementById("toolbox-tab-" + data.id), "Tab removed for " +
       data.id);
    index++;
    
    executeSoon(toggleTools);
    return;
  }
  ok(false, "Something went wrong, " + data.id + " was not unregistered");
  cleanup();
}

function checkRegistered(event, data) {
  if (data == prefNodes[index - prefNodes.length].getAttribute("id")) {
    ok(true, "Correct tool added back");
    
    let radio = doc.getElementById("toolbox-tab-" + data);
    ok(radio, "Tab added back for " + data);
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
    index++;
    
    executeSoon(toggleTools);
    return;
  }
  ok(false, "Something went wrong, " + data + " was not registered back");
  cleanup();
}

function cleanup() {
  toolbox.destroy().then(function() {
    gBrowser.removeCurrentTab();
    toolbox = doc = prefNodes = prefValues = panelWin = null;
    finish();
  });
}
