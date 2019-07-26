




let tempScope = {};
Cu.import("resource:///modules/HUDService.jsm", tempScope);
let HUDService = tempScope.HUDService;
Cu.import("resource://gre/modules/devtools/WebConsoleUtils.jsm", tempScope);
let WebConsoleUtils = tempScope.WebConsoleUtils;
Cu.import("resource:///modules/devtools/gDevTools.jsm", tempScope);
let gDevTools = tempScope.gDevTools;
let TargetFactory = tempScope.devtools.TargetFactory;
Components.utils.import("resource://gre/modules/devtools/Console.jsm", tempScope);
let console = tempScope.console;
let Promise = Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", {}).Promise;

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

const WEBCONSOLE_STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";
let WCU_l10n = new WebConsoleUtils.l10n(WEBCONSOLE_STRINGS_URI);

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
    if (browser.contentDocument.readyState != "complete") {
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
  let selector = ".hud-msg-node";
  
  if (aOnlyVisible) {
    selector += ":not(.hud-filtered-by-type)";
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

    
    let labels = msgs[i].querySelectorAll("label");
    for (let j = 0; j < labels.length; j++) {
      if (labels[j].getAttribute("value").indexOf(aMatchString) > -1) {
        found = true;
        break;
      }
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
  let target = TargetFactory.forTab(aTab || tab);
  gDevTools.showToolbox(target, "webconsole").then(function(toolbox) {
    let hud = toolbox.getCurrentPanel().hud;
    hud.jsterm._lazyVariablesView = false;
    aCallback(hud);
  });
}











function closeConsole(aTab, aCallback = function() { })
{
  let target = TargetFactory.forTab(aTab || tab);
  let toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    let panel = toolbox.getPanel("webconsole");
    if (panel) {
      let hudId = panel.hud.hudId;
      toolbox.destroy().then(aCallback.bind(null, hudId)).then(null, console.debug);
    }
    else {
      toolbox.destroy().then(aCallback.bind(null));
    }
  }
  else {
    aCallback();
  }
}
















function waitForOpenContextMenu(aContextMenu, aOptions) {
  let start = Date.now();
  let timeout = aOptions.timeout || 5000;
  let targetElement = aOptions.target;

  if (!aContextMenu) {
    ok(false, "Can't get a context menu.");
    aOptions.failureFn();
    return;
  }
  if (!targetElement) {
    ok(false, "Can't get a target element.");
    aOptions.failureFn();
    return;
  }

  function onPopupShown() {
    aContextMenu.removeEventListener("popupshown", onPopupShown);
    clearTimeout(onTimeout);
    aOptions.successFn();
  }


  aContextMenu.addEventListener("popupshown", onPopupShown);

  let onTimeout = setTimeout(function(){
    aContextMenu.removeEventListener("popupshown", onPopupShown);
    aOptions.failureFn();
  }, timeout);

  
  let eventDetails = { type : "contextmenu", button : 2};
  EventUtils.synthesizeMouse(targetElement, 2, 2,
                             eventDetails, targetElement.ownerDocument.defaultView);
}




function dumpConsoles()
{
  if (gPendingOutputTest) {
    console.log("dumpConsoles start");
    for each (let hud in HUDService.hudReferences) {
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
  let text = getMessageElementText(aMessage);
  let repeats = aMessage.querySelector(".webconsole-msg-repeat");
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
  browser = hudId = hud = filterBox = outputNode = cs = null;

  dumpConsoles();

  if (HUDConsoleUI.browserConsole) {
    let hud = HUDConsoleUI.browserConsole;

    if (hud.jsterm) {
      hud.jsterm.clearOutput(true);
    }

    HUDConsoleUI.toggleBrowserConsole().then(finishTest);
    return;
  }

  let hud = HUDService.getHudByWindow(content);
  if (!hud) {
    finish();
    return;
  }

  if (hud.jsterm) {
    hud.jsterm.clearOutput(true);
  }

  closeConsole(hud.target.tab, finish);

  hud = null;
}

function tearDown()
{
  dumpConsoles();

  if (HUDConsoleUI.browserConsole) {
    HUDConsoleUI.toggleBrowserConsole();
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

    return Promise.all(outstanding).then(lastStep).then(returnResults);
  }

  function onMatch(aProp, aRule, aMatched)
  {
    if (aMatched && !aRule.matchedProp) {
      aRule.matchedProp = aProp;
    }
  }

  function finder(aRules, aVar, aPromises)
  {
    for (let [id, prop] in aVar) {
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
      return Promise.resolve(null);
    }

    let deferred = Promise.defer();
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
      return Promise.resolve(null);
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
    return Promise.resolve(aResult);
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
    let isGenerator = aProp.displayValue == "[object Generator]";
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

  outstanding.push(Promise.resolve(true));

  return Promise.all(outstanding).then(function _onMatchDone(aResults) {
    let ruleMatched = aResults.indexOf(false) == -1;
    return resolve(ruleMatched);
  });
}













function isVariableViewPropertyIterator(aProp, aWebConsole)
{
  if (aProp.displayValue == "[object Iterator]") {
    return Promise.resolve(true);
  }

  let deferred = Promise.defer();

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
  let lastDeferred = Promise.defer();

  function fetch(aProp)
  {
    if (!aProp.onexpand) {
      ok(false, "property " + aProp.name + " cannot be expanded: !onexpand");
      return Promise.reject(aProp);
    }

    let deferred = Promise.defer();

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
      EventUtils.synthesizeKey("VK_ENTER", { shiftKey: true }, view.window);
      break;
    case "value":
      EventUtils.synthesizeKey("VK_ENTER", {}, view.window);
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

    EventUtils.synthesizeKey("VK_ENTER", {}, view.window);

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

  let deferred = Promise.defer();

  let target = TargetFactory.forTab(aOptions.tab);
  let toolbox = gDevTools.getToolbox(target);
  let dbgPanelAlreadyOpen = toolbox.getPanel("jsdebugger");

  gDevTools.showToolbox(target, "jsdebugger").then(function onSuccess(aToolbox) {
    let panel = aToolbox.getCurrentPanel();
    let panelWin = panel.panelWin;

    panel._view.Variables.lazyEmpty = false;
    panel._view.Variables.lazyAppend = false;

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
      panelWin.addEventListener("Debugger:AfterSourcesAdded",
        function onAfterSourcesAdded() {
          panelWin.removeEventListener("Debugger:AfterSourcesAdded",
                                       onAfterSourcesAdded);
          deferred.resolve(resolveObject);
        });
    }
  }, function onFailure(aReason) {
    console.debug("failed to open the toolbox for 'jsdebugger'", aReason);
    deferred.reject(aReason);
  });

  return deferred.promise;
}









function getMessageElementText(aElement)
{
  let text = aElement.textContent;
  let labels = aElement.querySelectorAll("label");
  for (let label of labels) {
    text += " " + label.getAttribute("value");
  }
  return text;
}






















function waitForMessages(aOptions)
{
  gPendingOutputTest++;
  let webconsole = aOptions.webconsole;
  let rules = WebConsoleUtils.cloneObject(aOptions.messages, true);
  let rulesMatched = 0;
  let listenerAdded = false;
  let deferred = Promise.defer();

  function checkText(aRule, aText)
  {
    let result;
    if (typeof aRule == "string") {
      result = aText.indexOf(aRule) > -1;
    }
    else if (aRule instanceof RegExp) {
      result = aRule.test(aText);
    }
    return result;
  }

  function checkConsoleTrace(aRule, aElement)
  {
    let elemText = getMessageElementText(aElement);
    let trace = aRule.consoleTrace;

    if (!checkText("Stack trace from ", elemText)) {
      return false;
    }

    let clickable = aElement.querySelector(".hud-clickable");
    if (!clickable) {
      ok(false, "console.trace() message is missing .hud-clickable");
      displayErrorContext(aRule, aElement);
      return false;
    }
    aRule.clickableElements = [clickable];

    if (trace.file &&
        !checkText("from " + trace.file + ", ", elemText)) {
      ok(false, "console.trace() message is missing the file name: " +
                trace.file);
      displayErrorContext(aRule, aElement);
      return false;
    }

    if (trace.fn &&
        !checkText(", function " + trace.fn + ", ", elemText)) {
      ok(false, "console.trace() message is missing the function name: " +
                trace.fn);
      displayErrorContext(aRule, aElement);
      return false;
    }

    if (trace.line &&
        !checkText(", line " + trace.line + ".", elemText)) {
      ok(false, "console.trace() message is missing the line number: " +
                trace.line);
      displayErrorContext(aRule, aElement);
      return false;
    }

    aRule.category = CATEGORY_WEBDEV;
    aRule.severity = SEVERITY_LOG;

    return true;
  }

  function checkConsoleTime(aRule, aElement)
  {
    let elemText = getMessageElementText(aElement);
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
    let elemText = getMessageElementText(aElement);
    let time = aRule.consoleTimeEnd;
    let regex = new RegExp(time + ": \\d+ms");

    if (!checkText(regex, elemText)) {
      return false;
    }

    aRule.category = CATEGORY_WEBDEV;
    aRule.severity = SEVERITY_LOG;

    return true;
  }

  function checkConsoleDir(aRule, aElement)
  {
    if (!aElement.classList.contains("webconsole-msg-inspector")) {
      return false;
    }

    let elemText = getMessageElementText(aElement);
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

  function checkMessage(aRule, aElement)
  {
    let elemText = getMessageElementText(aElement);

    if (aRule.text && !checkText(aRule.text, elemText)) {
      return false;
    }

    if (aRule.noText && checkText(aRule.noText, elemText)) {
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

    let partialMatch = !!(aRule.consoleTrace || aRule.consoleTime ||
                          aRule.consoleTimeEnd);

    if (aRule.category && aElement.category != aRule.category) {
      if (partialMatch) {
        is(aElement.category, aRule.category,
           "message category for rule: " + displayRule(aRule));
        displayErrorContext(aRule, aElement);
      }
      return false;
    }

    if (aRule.severity && aElement.severity != aRule.severity) {
      if (partialMatch) {
        is(aElement.severity, aRule.severity,
           "message severity for rule: " + displayRule(aRule));
        displayErrorContext(aRule, aElement);
      }
      return false;
    }

    if (aRule.repeats) {
      let repeats = aElement.querySelector(".webconsole-msg-repeat");
      if (!repeats || repeats.getAttribute("value") != aRule.repeats) {
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
      let clickables = aElement.querySelectorAll(".hud-clickable");
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
      let location = elem.querySelector(".webconsole-location");
      if (location) {
        let url = location.getAttribute("title");
        
        
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

  function maybeDone()
  {
    if (rulesMatched == rules.length) {
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
    if (rulesMatched == rules.length) {
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
    if (rulesMatched != rules.length) {
      listenerAdded = true;
      registerCleanupFunction(testCleanup);
      webconsole.ui.on("messages-added", onMessagesAdded);
      webconsole.ui.on("messages-updated", onMessagesAdded);
    }
  });

  return deferred.promise;
}








function scrollOutputToNode(aNode)
{
  let richListBoxNode = aNode.parentNode;
  while (richListBoxNode.tagName != "richlistbox") {
    richListBoxNode = richListBoxNode.parentNode;
  }

  let boxObject = richListBoxNode.scrollBoxObject;
  let nsIScrollBoxObject = boxObject.QueryInterface(Ci.nsIScrollBoxObject);
  nsIScrollBoxObject.ensureElementIsVisible(aNode);
}
