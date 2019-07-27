let testPage = "data:text/html,<body><input id='input1' value='value'><select size=2><option val=1>One</select></body>";

let browser;

function test() {
  waitForExplicitFinish();

  var tab = gBrowser.addTab();
  browser = gBrowser.getBrowserForTab(tab);
  gBrowser.selectedTab = tab;

  function runFirstTest(event) {
    browser.removeEventListener("load", runFirstTest, true);
    addEventListener("commandupdate", checkTest, false);
    doTest();
  }

  browser.addEventListener("load", runFirstTest, true);
  browser.contentWindow.location = testPage;

  gURLBar.focus();
}

let currentTest;

let tests = [
  
  
  { name: "focus body", test: function() { browser.focus() }, skipUpdate: gMultiProcessBrowser ? 0 : 1,
    commands: { "cmd_copy" : false, "cmd_paste": false, "cmd_selectAll" : true, "cmd_undo" : false, "cmd_redo": false } },

  
  { name: "focus input", test: function() { EventUtils.synthesizeKey("VK_TAB", {}) }, skipUpdate: 2,
    commands: { "cmd_copy" : true, "cmd_paste": true, "cmd_selectAll" : true, "cmd_undo" : false, "cmd_redo": false } },

  
  { name: "cursor right", test: function() { EventUtils.synthesizeKey("VK_RIGHT", {}) },
    commands: { "cmd_copy" : false, "cmd_paste": true, "cmd_selectAll" : true, "cmd_undo" : false, "cmd_redo": false } },

  
  { name: "select all", test: function() { EventUtils.synthesizeKey("a", { accelKey: true }) },
    commands: { "cmd_copy" : true, "cmd_paste": true, "cmd_selectAll" : true, "cmd_undo" : false, "cmd_redo": false } },

  
  { name: "change value", test: function() { EventUtils.synthesizeKey("c", {}) },
    commands: { "cmd_copy" : false, "cmd_paste": true, "cmd_selectAll" : true, "cmd_undo" : true, "cmd_redo": false } },

  
  { name: "undo", test: function() { EventUtils.synthesizeKey("z", {accelKey: true }) },
    commands: { "cmd_copy" : true, "cmd_paste": true, "cmd_selectAll" : true, "cmd_undo" : false, "cmd_redo": true  } },

  
  { name: "focus select", test: function() { EventUtils.synthesizeKey("VK_TAB", {}) }, skipUpdate: 1,
    commands: { "cmd_copy" : false, "cmd_paste": false, "cmd_selectAll" : true, "cmd_undo" : false, "cmd_redo": false } },
];

function doTest()
{
  if (!tests.length) {
    removeEventListener("commandupdate", checkTest, false);
    gBrowser.removeCurrentTab();
    finish();
    return;
  }

  currentTest = tests.shift();
  currentTest.test();
}

function checkTest(event)
{
  
  if (event.target != document.getElementById("editMenuCommandSetAll")) {
    return;
  }

  
  if ("skipUpdate" in currentTest && currentTest["skipUpdate"] > 0) {
    currentTest["skipUpdate"]--;
    return;
  }

  for (let command in currentTest.commands) {
    is(document.getElementById(command).getAttribute("disabled") != "true", currentTest.commands[command],
       currentTest["name"] + " " + command);
  }

  SimpleTest.executeSoon(doTest);
}
