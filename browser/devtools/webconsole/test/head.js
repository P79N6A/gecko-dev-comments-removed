




"use strict";

let {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
let {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let {require, TargetFactory} = devtools;
let {Utils: WebConsoleUtils} = require("devtools/toolkit/webconsole/utils");
let {Messages} = require("devtools/webconsole/console-output");



let gPendingOutputTest = 0;


const CATEGORY_NETWORK = 0;
const CATEGORY_CSS = 1;
const CATEGORY_JS = 2;
const CATEGORY_WEBDEV = 3;
const CATEGORY_INPUT = 4;
const CATEGORY_OUTPUT = 5;
const CATEGORY_SECURITY = 6;


const SEVERITY_ERROR = 0;
const SEVERITY_WARNING = 1;
const SEVERITY_INFO = 2;
const SEVERITY_LOG = 3;


const GROUP_INDENT = 12;

const WEBCONSOLE_STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let WCU_l10n = new WebConsoleUtils.l10n(WEBCONSOLE_STRINGS_URI);

gDevTools.testing = true;
SimpleTest.registerCleanupFunction(() => {
  gDevTools.testing = false;
});




function asyncTest(generator) {
  return () => Task.spawn(generator).then(null, ok.bind(null, false)).then(finishTest);
}

function log(aMsg)
{
  dump("*** WebConsoleTest: " + aMsg + "\n");
}

function pprint(aObj)
{
  for (let prop in aObj) {
    if (typeof aObj[prop] == "function") {
      log("function " + prop);
    }
    else {
      log(prop + ": " + aObj[prop]);
    }
  }
}

let tab, browser, hudId, hud, hudBox, filterBox, outputNode, cs;

function addTab(aURL)
{
  gBrowser.selectedTab = gBrowser.addTab(aURL);
  tab = gBrowser.selectedTab;
  browser = gBrowser.getBrowserForTab(tab);
}

function loadTab(url) {
  let deferred = promise.defer();

  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  let browser = gBrowser.getBrowserForTab(tab);

  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    deferred.resolve({tab: tab, browser: browser});
  }, true);

  return deferred.promise;
}

function loadBrowser(browser) {
  let deferred = promise.defer();

  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    deferred.resolve(null)
  }, true);

  return deferred.promise;
}

function closeTab(tab) {
  let deferred = promise.defer();

  let container = gBrowser.tabContainer;

  container.addEventListener("TabClose", function onTabClose() {
    container.removeEventListener("TabClose", onTabClose, true);
    deferred.resolve(null);
  }, true);

  gBrowser.removeTab(tab);

  return deferred.promise;
}

function afterAllTabsLoaded(callback, win) {
  win = win || window;

  let stillToLoad = 0;

  function onLoad() {
    this.removeEventListener("load", onLoad, true);
    stillToLoad--;
    if (!stillToLoad)
      callback();
  }

  for (let a = 0; a < win.gBrowser.tabs.length; a++) {
    let browser = win.gBrowser.tabs[a].linkedBrowser;
    if (browser.webProgress.isLoadingDocument) {
      stillToLoad++;
      browser.addEventListener("load", onLoad, true);
    }
  }

  if (!stillToLoad)
    callback();
}

















function testLogEntry(aOutputNode, aMatchString, aMsg, aOnlyVisible,
                      aFailIfFound, aClass)
{
  let selector = ".message";
  
  if (aOnlyVisible) {
    selector += ":not(.filtered-by-type):not(.filtered-by-string)";
  }
  if (aClass) {
    selector += "." + aClass;
  }

  let msgs = aOutputNode.querySelectorAll(selector);
  let found = false;
  for (let i = 0, n = msgs.length; i < n; i++) {
    let message = msgs[i].textContent.indexOf(aMatchString);
    if (message > -1) {
      found = true;
      break;
    }
  }

  is(found, !aFailIfFound, aMsg);
}







function findLogEntry(aString)
{
  testLogEntry(outputNode, aString, "found " + aString);
}













function openConsole(aTab, aCallback = function() { })
{
  let deferred = promise.defer();
  let target = TargetFactory.forTab(aTab || tab);
  gDevTools.showToolbox(target, "webconsole").then(function(toolbox) {
    let hud = toolbox.getCurrentPanel().hud;
    hud.jsterm._lazyVariablesView = false;
    aCallback(hud);
    deferred.resolve(hud);
  });
  return deferred.promise;
}













function closeConsole(aTab, aCallback = function() { })
{
  let target = TargetFactory.forTab(aTab || tab);
  let toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    let panel = toolbox.getPanel("webconsole");
    if (panel) {
      let hudId = panel.hud.hudId;
      return toolbox.destroy().then(aCallback.bind(null, hudId)).then(null, console.debug);
    }
    return toolbox.destroy().then(aCallback.bind(null));
  }

  aCallback();
  return promise.resolve(null);
}














function waitForContextMenu(aPopup, aButton, aOnShown, aOnHidden)
{
  function onPopupShown() {
    info("onPopupShown");
    aPopup.removeEventListener("popupshown", onPopupShown);

    aOnShown();

    
    aPopup.addEventListener("popuphidden", onPopupHidden);
    executeSoon(() => aPopup.hidePopup());
  }
  function onPopupHidden() {
    info("onPopupHidden");
    aPopup.removeEventListener("popuphidden", onPopupHidden);
    aOnHidden();
  }

  aPopup.addEventListener("popupshown", onPopupShown);

  info("wait for the context menu to open");
  let eventDetails = { type: "contextmenu", button: 2};
  EventUtils.synthesizeMouse(aButton, 2, 2, eventDetails,
                             aButton.ownerDocument.defaultView);
}




function dumpConsoles()
{
  if (gPendingOutputTest) {
    console.log("dumpConsoles start");
    for (let [, hud] of HUDService.consoles) {
      if (!hud.outputNode) {
        console.debug("no output content for", hud.hudId);
        continue;
      }

      console.debug("output content for", hud.hudId);
      for (let elem of hud.outputNode.childNodes) {
        dumpMessageElement(elem);
      }
    }
    console.log("dumpConsoles end");

    gPendingOutputTest = 0;
  }
}







function dumpMessageElement(aMessage)
{
  let text = aMessage.textContent;
  let repeats = aMessage.querySelector(".message-repeats");
  if (repeats) {
    repeats = repeats.getAttribute("value");
  }
  console.debug("id", aMessage.getAttribute("id"),
                "date", aMessage.timestamp,
                "class", aMessage.className,
                "category", aMessage.category,
                "severity", aMessage.severity,
                "repeats", repeats,
                "clipboardText", aMessage.clipboardText,
                "text", text);
}

function finishTest()
{
  browser = hudId = hud = filterBox = outputNode = cs = hudBox = null;

  dumpConsoles();

  let browserConsole = HUDService.getBrowserConsole();
  if (browserConsole) {
    if (browserConsole.jsterm) {
      browserConsole.jsterm.clearOutput(true);
    }
    HUDService.toggleBrowserConsole().then(finishTest);
    return;
  }

  let contentHud = HUDService.getHudByWindow(content);
  if (!contentHud) {
    finish();
    return;
  }

  if (contentHud.jsterm) {
    contentHud.jsterm.clearOutput(true);
  }

  closeConsole(contentHud.target.tab, finish);

  contentHud = null;
}

function tearDown()
{
  dumpConsoles();

  if (HUDService.getBrowserConsole()) {
    HUDService.toggleBrowserConsole();
  }

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.closeToolbox(target);
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
  WCU_l10n = tab = browser = hudId = hud = filterBox = outputNode = cs = null;
}

registerCleanupFunction(tearDown);

waitForExplicitFinish();






















function waitForSuccess(aOptions)
{
  let start = Date.now();
  let timeout = aOptions.timeout || 5000;

  function wait(validatorFn, successFn, failureFn)
  {
    if ((Date.now() - start) > timeout) {
      
      ok(false, "Timed out while waiting for: " + aOptions.name);
      failureFn(aOptions);
      return;
    }

    if (validatorFn(aOptions)) {
      ok(true, aOptions.name);
      successFn();
    }
    else {
      setTimeout(function() wait(validatorFn, successFn, failureFn), 100);
    }
  }

  wait(aOptions.validatorFn, aOptions.successFn, aOptions.failureFn);
}

function openInspector(aCallback, aTab = gBrowser.selectedTab)
{
  let target = TargetFactory.forTab(aTab);
  gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
    aCallback(toolbox.getCurrentPanel());
  });
}

























function findVariableViewProperties(aView, aRules, aOptions)
{
  
  function init()
  {
    
    
    let expandRules = [];
    let rules = aRules.filter((aRule) => {
      if (typeof aRule.name == "string" && aRule.name.indexOf(".") > -1) {
        expandRules.push(aRule);
        return false;
      }
      return true;
    });

    
    
    
    let outstanding = [];
    finder(rules, aView, outstanding);

    
    let lastStep = processExpandRules.bind(null, expandRules);

    
    let returnResults = onAllRulesMatched.bind(null, aRules);

    return promise.all(outstanding).then(lastStep).then(returnResults);
  }

  function onMatch(aProp, aRule, aMatched)
  {
    if (aMatched && !aRule.matchedProp) {
      aRule.matchedProp = aProp;
    }
  }

  function finder(aRules, aVar, aPromises)
  {
    for (let [id, prop] of aVar) {
      for (let rule of aRules) {
        let matcher = matchVariablesViewProperty(prop, rule, aOptions);
        aPromises.push(matcher.then(onMatch.bind(null, prop, rule)));
      }
    }
  }

  function processExpandRules(aRules)
  {
    let rule = aRules.shift();
    if (!rule) {
      return promise.resolve(null);
    }

    let deferred = promise.defer();
    let expandOptions = {
      rootVariable: aView,
      expandTo: rule.name,
      webconsole: aOptions.webconsole,
    };

    variablesViewExpandTo(expandOptions).then(function onSuccess(aProp) {
      let name = rule.name;
      let lastName = name.split(".").pop();
      rule.name = lastName;

      let matched = matchVariablesViewProperty(aProp, rule, aOptions);
      return matched.then(onMatch.bind(null, aProp, rule)).then(function() {
        rule.name = name;
      });
    }, function onFailure() {
      return promise.resolve(null);
    }).then(processExpandRules.bind(null, aRules)).then(function() {
      deferred.resolve(null);
    });

    return deferred.promise;
  }

  function onAllRulesMatched(aRules)
  {
    for (let rule of aRules) {
      let matched = rule.matchedProp;
      if (matched && !rule.dontMatch) {
        ok(true, "rule " + rule.name + " matched for property " + matched.name);
      }
      else if (matched && rule.dontMatch) {
        ok(false, "rule " + rule.name + " should not match property " +
           matched.name);
      }
      else {
        ok(rule.dontMatch, "rule " + rule.name + " did not match any property");
      }
    }
    return aRules;
  }

  return init();
}

















function matchVariablesViewProperty(aProp, aRule, aOptions)
{
  function resolve(aResult) {
    return promise.resolve(aResult);
  }

  if (aRule.name) {
    let match = aRule.name instanceof RegExp ?
                aRule.name.test(aProp.name) :
                aProp.name == aRule.name;
    if (!match) {
      return resolve(false);
    }
  }

  if (aRule.value) {
    let displayValue = aProp.displayValue;
    if (aProp.displayValueClassName == "token-string") {
      displayValue = displayValue.substring(1, displayValue.length - 1);
    }

    let match = aRule.value instanceof RegExp ?
                aRule.value.test(displayValue) :
                displayValue == aRule.value;
    if (!match) {
      info("rule " + aRule.name + " did not match value, expected '" +
           aRule.value + "', found '" + displayValue  + "'");
      return resolve(false);
    }
  }

  if ("isGetter" in aRule) {
    let isGetter = !!(aProp.getter && aProp.get("get"));
    if (aRule.isGetter != isGetter) {
      info("rule " + aRule.name + " getter test failed");
      return resolve(false);
    }
  }

  if ("isGenerator" in aRule) {
    let isGenerator = aProp.displayValue == "Generator";
    if (aRule.isGenerator != isGenerator) {
      info("rule " + aRule.name + " generator test failed");
      return resolve(false);
    }
  }

  let outstanding = [];

  if ("isIterator" in aRule) {
    let isIterator = isVariableViewPropertyIterator(aProp, aOptions.webconsole);
    outstanding.push(isIterator.then((aResult) => {
      if (aResult != aRule.isIterator) {
        info("rule " + aRule.name + " iterator test failed");
      }
      return aResult == aRule.isIterator;
    }));
  }

  outstanding.push(promise.resolve(true));

  return promise.all(outstanding).then(function _onMatchDone(aResults) {
    let ruleMatched = aResults.indexOf(false) == -1;
    return resolve(ruleMatched);
  });
}













function isVariableViewPropertyIterator(aProp, aWebConsole)
{
  if (aProp.displayValue == "Iterator") {
    return promise.resolve(true);
  }

  let deferred = promise.defer();

  variablesViewExpandTo({
    rootVariable: aProp,
    expandTo: "__proto__.__iterator__",
    webconsole: aWebConsole,
  }).then(function onSuccess(aProp) {
    deferred.resolve(true);
  }, function onFailure() {
    deferred.resolve(false);
  });

  return deferred.promise;
}



















function variablesViewExpandTo(aOptions)
{
  let root = aOptions.rootVariable;
  let expandTo = aOptions.expandTo.split(".");
  let jsterm = (aOptions.webconsole || {}).jsterm;
  let lastDeferred = promise.defer();

  function fetch(aProp)
  {
    if (!aProp.onexpand) {
      ok(false, "property " + aProp.name + " cannot be expanded: !onexpand");
      return promise.reject(aProp);
    }

    let deferred = promise.defer();

    if (aProp._fetched || !jsterm) {
      executeSoon(function() {
        deferred.resolve(aProp);
      });
    }
    else {
      jsterm.once("variablesview-fetched", function _onFetchProp() {
        executeSoon(() => deferred.resolve(aProp));
      });
    }

    aProp.expand();

    return deferred.promise;
  }

  function getNext(aProp)
  {
    let name = expandTo.shift();
    let newProp = aProp.get(name);

    if (expandTo.length > 0) {
      ok(newProp, "found property " + name);
      if (newProp) {
        fetch(newProp).then(getNext, fetchError);
      }
      else {
        lastDeferred.reject(aProp);
      }
    }
    else {
      if (newProp) {
        lastDeferred.resolve(newProp);
      }
      else {
        lastDeferred.reject(aProp);
      }
    }
  }

  function fetchError(aProp)
  {
    lastDeferred.reject(aProp);
  }

  if (!root._fetched) {
    fetch(root).then(getNext, fetchError);
  }
  else {
    getNext(root);
  }

  return lastDeferred.promise;
}















function updateVariablesViewProperty(aOptions)
{
  let view = aOptions.property._variablesView;
  view.window.focus();
  aOptions.property.focus();

  switch (aOptions.field) {
    case "name":
      EventUtils.synthesizeKey("VK_RETURN", { shiftKey: true }, view.window);
      break;
    case "value":
      EventUtils.synthesizeKey("VK_RETURN", {}, view.window);
      break;
    default:
      throw new Error("options.field is incorrect");
      return;
  }

  executeSoon(() => {
    EventUtils.synthesizeKey("A", { accelKey: true }, view.window);

    for (let c of aOptions.string) {
      EventUtils.synthesizeKey(c, {}, gVariablesView.window);
    }

    if (aOptions.webconsole) {
      aOptions.webconsole.jsterm.once("variablesview-fetched", aOptions.callback);
    }

    EventUtils.synthesizeKey("VK_RETURN", {}, view.window);

    if (!aOptions.webconsole) {
      executeSoon(aOptions.callback);
    }
  });
}
















function openDebugger(aOptions = {})
{
  if (!aOptions.tab) {
    aOptions.tab = gBrowser.selectedTab;
  }

  let deferred = promise.defer();

  let target = TargetFactory.forTab(aOptions.tab);
  let toolbox = gDevTools.getToolbox(target);
  let dbgPanelAlreadyOpen = toolbox.getPanel("jsdebugger");

  gDevTools.showToolbox(target, "jsdebugger").then(function onSuccess(aToolbox) {
    let panel = aToolbox.getCurrentPanel();
    let panelWin = panel.panelWin;

    panel._view.Variables.lazyEmpty = false;

    let resolveObject = {
      target: target,
      toolbox: aToolbox,
      panel: panel,
      panelWin: panelWin,
    };

    if (dbgPanelAlreadyOpen) {
      deferred.resolve(resolveObject);
    }
    else {
      panelWin.once(panelWin.EVENTS.SOURCES_ADDED, () => {
        deferred.resolve(resolveObject);
      });
    }
  }, function onFailure(aReason) {
    console.debug("failed to open the toolbox for 'jsdebugger'", aReason);
    deferred.reject(aReason);
  });

  return deferred.promise;
}






































































function waitForMessages(aOptions)
{
  gPendingOutputTest++;
  let webconsole = aOptions.webconsole;
  let rules = WebConsoleUtils.cloneObject(aOptions.messages, true);
  let rulesMatched = 0;
  let listenerAdded = false;
  let deferred = promise.defer();
  aOptions.matchCondition = aOptions.matchCondition || "all";

  function checkText(aRule, aText)
  {
    let result = false;
    if (Array.isArray(aRule)) {
      result = aRule.every((s) => checkText(s, aText));
    }
    else if (typeof aRule == "string") {
      result = aText.indexOf(aRule) > -1;
    }
    else if (aRule instanceof RegExp) {
      result = aRule.test(aText);
    }
    else {
      result = aRule == aText;
    }
    return result;
  }

  function checkConsoleTable(aRule, aElement)
  {
    let elemText = aElement.textContent;
    let table = aRule.consoleTable;

    if (!checkText("console.table():", elemText)) {
      return false;
    }

    aRule.category = CATEGORY_WEBDEV;
    aRule.severity = SEVERITY_LOG;
    aRule.type = Messages.ConsoleTable;

    return true;
  }

  function checkConsoleTrace(aRule, aElement)
  {
    let elemText = aElement.textContent;
    let trace = aRule.consoleTrace;

    if (!checkText("console.trace():", elemText)) {
      return false;
    }

    aRule.category = CATEGORY_WEBDEV;
    aRule.severity = SEVERITY_LOG;
    aRule.type = Messages.ConsoleTrace;

    if (!aRule.stacktrace && typeof trace == "object" && trace !== true) {
      if (Array.isArray(trace)) {
        aRule.stacktrace = trace;
      } else {
        aRule.stacktrace = [trace];
      }
    }

    return true;
  }

  function checkConsoleTime(aRule, aElement)
  {
    let elemText = aElement.textContent;
    let time = aRule.consoleTime;

    if (!checkText(time + ": timer started", elemText)) {
      return false;
    }

    aRule.category = CATEGORY_WEBDEV;
    aRule.severity = SEVERITY_LOG;

    return true;
  }

  function checkConsoleTimeEnd(aRule, aElement)
  {
    let elemText = aElement.textContent;
    let time = aRule.consoleTimeEnd;
    let regex = new RegExp(time + ": -?\\d+([,.]\\d+)?ms");

    if (!checkText(regex, elemText)) {
      return false;
    }

    aRule.category = CATEGORY_WEBDEV;
    aRule.severity = SEVERITY_LOG;

    return true;
  }

  function checkConsoleDir(aRule, aElement)
  {
    if (!aElement.classList.contains("inlined-variables-view")) {
      return false;
    }

    let elemText = aElement.textContent;
    if (!checkText(aRule.consoleDir, elemText)) {
      return false;
    }

    let iframe = aElement.querySelector("iframe");
    if (!iframe) {
      ok(false, "console.dir message has no iframe");
      return false;
    }

    return true;
  }

  function checkConsoleGroup(aRule, aElement)
  {
    if (!isNaN(parseInt(aRule.consoleGroup))) {
      aRule.groupDepth = aRule.consoleGroup;
    }
    aRule.category = CATEGORY_WEBDEV;
    aRule.severity = SEVERITY_LOG;

    return true;
  }

  function checkSource(aRule, aElement)
  {
    let location = aElement.querySelector(".message-location");
    if (!location) {
      return false;
    }

    if (!checkText(aRule.source.url, location.getAttribute("title"))) {
      return false;
    }

    if ("line" in aRule.source && location.sourceLine != aRule.source.line) {
      return false;
    }

    return true;
  }

  function checkCollapsible(aRule, aElement)
  {
    let msg = aElement._messageObject;
    if (!msg || !!msg.collapsible != aRule.collapsible) {
      return false;
    }

    return true;
  }

  function checkStacktrace(aRule, aElement)
  {
    let stack = aRule.stacktrace;
    let frames = aElement.querySelectorAll(".stacktrace > li");
    if (!frames.length) {
      return false;
    }

    for (let i = 0; i < stack.length; i++) {
      let frame = frames[i];
      let expected = stack[i];
      if (!frame) {
        ok(false, "expected frame #" + i + " but didnt find it");
        return false;
      }

      if (expected.file) {
        let file = frame.querySelector(".message-location").title;
        if (!checkText(expected.file, file)) {
          ok(false, "frame #" + i + " does not match file name: " +
                    expected.file);
          displayErrorContext(aRule, aElement);
          return false;
        }
      }

      if (expected.fn) {
        let fn = frame.querySelector(".function").textContent;
        if (!checkText(expected.fn, fn)) {
          ok(false, "frame #" + i + " does not match the function name: " +
                    expected.fn);
          displayErrorContext(aRule, aElement);
          return false;
        }
      }

      if (expected.line) {
        let line = frame.querySelector(".message-location").sourceLine;
        if (!checkText(expected.line, line)) {
          ok(false, "frame #" + i + " does not match the line number: " +
                    expected.line);
          displayErrorContext(aRule, aElement);
          return false;
        }
      }
    }

    return true;
  }

  function checkMessage(aRule, aElement)
  {
    let elemText = aElement.textContent;

    if (aRule.text && !checkText(aRule.text, elemText)) {
      return false;
    }

    if (aRule.noText && checkText(aRule.noText, elemText)) {
      return false;
    }

    if (aRule.consoleTable && !checkConsoleTable(aRule, aElement)) {
      return false;
    }

    if (aRule.consoleTrace && !checkConsoleTrace(aRule, aElement)) {
      return false;
    }

    if (aRule.consoleTime && !checkConsoleTime(aRule, aElement)) {
      return false;
    }

    if (aRule.consoleTimeEnd && !checkConsoleTimeEnd(aRule, aElement)) {
      return false;
    }

    if (aRule.consoleDir && !checkConsoleDir(aRule, aElement)) {
      return false;
    }

    if (aRule.consoleGroup && !checkConsoleGroup(aRule, aElement)) {
      return false;
    }

    if (aRule.source && !checkSource(aRule, aElement)) {
      return false;
    }

    if ("collapsible" in aRule && !checkCollapsible(aRule, aElement)) {
      return false;
    }

    let partialMatch = !!(aRule.consoleTrace || aRule.consoleTime ||
                          aRule.consoleTimeEnd);

    
    
    if (aRule.type) {
      if (!aElement._messageObject ||
          !(aElement._messageObject instanceof aRule.type)) {
        if (partialMatch) {
          ok(false, "message type for rule: " + displayRule(aRule));
          displayErrorContext(aRule, aElement);
        }
        return false;
      }
      partialMatch = true;
    }

    if ("category" in aRule && aElement.category != aRule.category) {
      if (partialMatch) {
        is(aElement.category, aRule.category,
           "message category for rule: " + displayRule(aRule));
        displayErrorContext(aRule, aElement);
      }
      return false;
    }

    if ("severity" in aRule && aElement.severity != aRule.severity) {
      if (partialMatch) {
        is(aElement.severity, aRule.severity,
           "message severity for rule: " + displayRule(aRule));
        displayErrorContext(aRule, aElement);
      }
      return false;
    }

    if (aRule.text) {
      partialMatch = true;
    }

    if (aRule.stacktrace && !checkStacktrace(aRule, aElement)) {
      if (partialMatch) {
        ok(false, "failed to match stacktrace for rule: " + displayRule(aRule));
        displayErrorContext(aRule, aElement);
      }
      return false;
    }

    if (aRule.category == CATEGORY_NETWORK && "url" in aRule &&
        !checkText(aRule.url, aElement.url)) {
      return false;
    }

    if ("repeats" in aRule) {
      let repeats = aElement.querySelector(".message-repeats");
      if (!repeats || repeats.getAttribute("value") != aRule.repeats) {
        return false;
      }
    }

    if ("groupDepth" in aRule) {
      let indentNode = aElement.querySelector(".indent");
      let indent = (GROUP_INDENT * aRule.groupDepth)  + "px";
      if (!indentNode || indentNode.style.width != indent) {
        is(indentNode.style.width, indent,
           "group depth check failed for message rule: " + displayRule(aRule));
        return false;
      }
    }

    if ("longString" in aRule) {
      let longStrings = aElement.querySelectorAll(".longStringEllipsis");
      if (aRule.longString != !!longStrings[0]) {
        if (partialMatch) {
          is(!!longStrings[0], aRule.longString,
             "long string existence check failed for message rule: " +
             displayRule(aRule));
          displayErrorContext(aRule, aElement);
        }
        return false;
      }
      aRule.longStrings = longStrings;
    }

    if ("objects" in aRule) {
      let clickables = aElement.querySelectorAll(".message-body a");
      if (aRule.objects != !!clickables[0]) {
        if (partialMatch) {
          is(!!clickables[0], aRule.objects,
             "objects existence check failed for message rule: " +
             displayRule(aRule));
          displayErrorContext(aRule, aElement);
        }
        return false;
      }
      aRule.clickableElements = clickables;
    }

    let count = aRule.count || 1;
    if (!aRule.matched) {
      aRule.matched = new Set();
    }
    aRule.matched.add(aElement);

    return aRule.matched.size == count;
  }

  function onMessagesAdded(aEvent, aNewElements)
  {
    for (let elem of aNewElements) {
      let location = elem.querySelector(".message-location");
      if (location) {
        let url = location.title;
        
        
        if (url.indexOf("browser/devtools/webconsole/test/head.js") != -1) {
          continue;
        }
      }

      for (let rule of rules) {
        if (rule._ruleMatched) {
          continue;
        }

        let matched = checkMessage(rule, elem);
        if (matched) {
          rule._ruleMatched = true;
          rulesMatched++;
          ok(1, "matched rule: " + displayRule(rule));
          if (maybeDone()) {
            return;
          }
        }
      }
    }
  }

  function allRulesMatched()
  {
    return aOptions.matchCondition == "all" && rulesMatched == rules.length ||
           aOptions.matchCondition == "any" && rulesMatched > 0;
  }

  function maybeDone()
  {
    if (allRulesMatched()) {
      if (listenerAdded) {
        webconsole.ui.off("messages-added", onMessagesAdded);
        webconsole.ui.off("messages-updated", onMessagesAdded);
      }
      gPendingOutputTest--;
      deferred.resolve(rules);
      return true;
    }
    return false;
  }

  function testCleanup() {
    if (allRulesMatched()) {
      return;
    }

    if (webconsole.ui) {
      webconsole.ui.off("messages-added", onMessagesAdded);
    }

    for (let rule of rules) {
      if (!rule._ruleMatched) {
        ok(false, "failed to match rule: " + displayRule(rule));
      }
    }
  }

  function displayRule(aRule)
  {
    return aRule.name || aRule.text;
  }

  function displayErrorContext(aRule, aElement)
  {
    console.log("error occured during rule " + displayRule(aRule));
    console.log("while checking the following message");
    dumpMessageElement(aElement);
  }

  executeSoon(() => {
    onMessagesAdded("messages-added", webconsole.outputNode.childNodes);
    if (!allRulesMatched()) {
      listenerAdded = true;
      registerCleanupFunction(testCleanup);
      webconsole.ui.on("messages-added", onMessagesAdded);
      webconsole.ui.on("messages-updated", onMessagesAdded);
    }
  });

  return deferred.promise;
}

function whenDelayedStartupFinished(aWindow, aCallback)
{
  Services.obs.addObserver(function observer(aSubject, aTopic) {
    if (aWindow == aSubject) {
      Services.obs.removeObserver(observer, aTopic);
      executeSoon(aCallback);
    }
  }, "browser-delayed-startup-finished", false);
}







































function checkOutputForInputs(hud, inputTests)
{
  let container = gBrowser.tabContainer;

  function* runner()
  {
    for (let [i, entry] of inputTests.entries()) {
      info("checkInput(" + i + "): " + entry.input);
      yield checkInput(entry);
    }
    container = null;
  }

  function* checkInput(entry)
  {
    yield checkConsoleLog(entry);
    yield checkPrintOutput(entry);
    yield checkJSEval(entry);
  }

  function* checkConsoleLog(entry)
  {
    hud.jsterm.clearOutput();
    hud.jsterm.execute("console.log(" + entry.input + ")");

    let [result] = yield waitForMessages({
      webconsole: hud,
      messages: [{
        name: "console.log() output: " + entry.output,
        text: entry.output,
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      }],
    });

    if (typeof entry.inspectorIcon == "boolean") {
      let msg = [...result.matched][0];
      yield checkLinkToInspector(entry, msg);
    }
  }

  function checkPrintOutput(entry)
  {
    hud.jsterm.clearOutput();
    hud.jsterm.execute("print(" + entry.input + ")");

    let printOutput = entry.printOutput || entry.output;

    return waitForMessages({
      webconsole: hud,
      messages: [{
        name: "print() output: " + printOutput,
        text: printOutput,
        category: CATEGORY_OUTPUT,
      }],
    });
  }

  function* checkJSEval(entry)
  {
    hud.jsterm.clearOutput();
    hud.jsterm.execute(entry.input);

    let [result] = yield waitForMessages({
      webconsole: hud,
      messages: [{
        name: "JS eval output: " + entry.output,
        text: entry.output,
        category: CATEGORY_OUTPUT,
      }],
    });

    let msg = [...result.matched][0];
    if (!entry.noClick) {
      yield checkObjectClick(entry, msg);
    }
    if (typeof entry.inspectorIcon == "boolean") {
      yield checkLinkToInspector(entry, msg);
    }
  }

  function* checkObjectClick(entry, msg)
  {
    let body = msg.querySelector(".message-body a") ||
               msg.querySelector(".message-body");
    ok(body, "the message body");

    let deferredVariablesView = promise.defer();
    entry._onVariablesViewOpen = onVariablesViewOpen.bind(null, entry, deferredVariablesView);
    hud.jsterm.on("variablesview-open", entry._onVariablesViewOpen);

    let deferredTab = promise.defer();
    entry._onTabOpen = onTabOpen.bind(null, entry, deferredTab);
    container.addEventListener("TabOpen", entry._onTabOpen, true);

    body.scrollIntoView();
    EventUtils.synthesizeMouse(body, 2, 2, {}, hud.iframeWindow);

    if (entry.inspectable) {
      info("message body tagName '" + body.tagName +  "' className '" + body.className + "'");
      yield deferredVariablesView.promise;
    } else {
      hud.jsterm.off("variablesview-open", entry._onVariablesView);
      entry._onVariablesView = null;
    }

    if (entry.expectedTab) {
      yield deferredTab.promise;
    } else {
      container.removeEventListener("TabOpen", entry._onTabOpen, true);
      entry._onTabOpen = null;
    }

    yield promise.resolve(null);
  }

  function checkLinkToInspector(entry, msg)
  {
    let elementNodeWidget = [...msg._messageObject.widgets][0];
    if (!elementNodeWidget) {
      ok(!entry.inspectorIcon, "The message has no ElementNode widget");
      return;
    }

    return elementNodeWidget.linkToInspector().then(() => {
      
      if (entry.inspectorIcon) {
        ok(msg.querySelectorAll(".open-inspector").length,
          "The ElementNode widget is linked to the inspector");
      } else {
        ok(!msg.querySelectorAll(".open-inspector").length,
          "The ElementNode widget isn't linked to the inspector");
      }
    }, () => {
      
      ok(!entry.inspectorIcon, "The ElementNode widget isn't linked to the inspector");
    });
  }

  function onVariablesViewOpen(entry, {resolve, reject}, event, view, options)
  {
    let label = entry.variablesViewLabel || entry.output;
    if (typeof label == "string" && options.label != label) {
      return;
    }
    if (label instanceof RegExp && !label.test(options.label)) {
      return;
    }

    hud.jsterm.off("variablesview-open", entry._onVariablesViewOpen);
    entry._onVariablesViewOpen = null;
    ok(entry.inspectable, "variables view was shown");

    resolve(null);
  }

  function onTabOpen(entry, {resolve, reject}, event)
  {
    container.removeEventListener("TabOpen", entry._onTabOpen, true);
    entry._onTabOpen = null;

    let tab = event.target;
    let browser = gBrowser.getBrowserForTab(tab);
    loadBrowser(browser).then(() => {
      let uri = content.location.href;
      ok(entry.expectedTab && entry.expectedTab == uri,
        "opened tab '" + uri +  "', expected tab '" + entry.expectedTab + "'");
      return closeTab(tab);
    }).then(resolve, reject);
  }

  return Task.spawn(runner);
}









function once(target, eventName, useCapture=false) {
  info("Waiting for event: '" + eventName + "' on " + target + ".");

  let deferred = promise.defer();

  for (let [add, remove] of [
    ["addEventListener", "removeEventListener"],
    ["addListener", "removeListener"],
    ["on", "off"]
  ]) {
    if ((add in target) && (remove in target)) {
      target[add](eventName, function onEvent(...aArgs) {
        target[remove](eventName, onEvent, useCapture);
        deferred.resolve.apply(deferred, aArgs);
      }, useCapture);
      break;
    }
  }

  return deferred.promise;
}

