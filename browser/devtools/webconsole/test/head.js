




"use strict";

let {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
let {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let {require, TargetFactory} = devtools;
let {Utils: WebConsoleUtils} = require("devtools/toolkit/webconsole/utils");
let {Messages} = require("devtools/webconsole/console-output");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const asyncStorage = require("devtools/toolkit/shared/async-storage");



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

const WEBCONSOLE_STRINGS_URI = "chrome://browser/locale/devtools/" +
                               "webconsole.properties";
let WCUL10n = new WebConsoleUtils.l10n(WEBCONSOLE_STRINGS_URI);

DevToolsUtils.testing = true;

function asyncTest(generator) {
  return () => {
    Task.spawn(generator).then(finishTest);
  };
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
    deferred.resolve(null);
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
    if (!stillToLoad) {
      callback();
    }
  }

  for (let a = 0; a < win.gBrowser.tabs.length; a++) {
    let browser = win.gBrowser.tabs[a].linkedBrowser;
    if (browser.webProgress.isLoadingDocument) {
      stillToLoad++;
      browser.addEventListener("load", onLoad, true);
    }
  }

  if (!stillToLoad) {
    callback();
  }
}

















function testLogEntry(outputNode, matchString, msg, onlyVisible,
                      failIfFound, cssClass) {
  let selector = ".message";
  
  if (onlyVisible) {
    selector += ":not(.filtered-by-type):not(.filtered-by-string)";
  }
  if (cssClass) {
    selector += "." + aClass;
  }

  let msgs = outputNode.querySelectorAll(selector);
  let found = false;
  for (let i = 0, n = msgs.length; i < n; i++) {
    let message = msgs[i].textContent.indexOf(matchString);
    if (message > -1) {
      found = true;
      break;
    }
  }

  is(found, !failIfFound, msg);
}







function findLogEntry(str) {
  testLogEntry(outputNode, str, "found " + str);
}













let openConsole = function(tab) {
  let webconsoleOpened = promise.defer();
  let target = TargetFactory.forTab(tab || gBrowser.selectedTab);
  gDevTools.showToolbox(target, "webconsole").then(toolbox => {
    let hud = toolbox.getCurrentPanel().hud;
    hud.jsterm._lazyVariablesView = false;
    webconsoleOpened.resolve(hud);
  });
  return webconsoleOpened.promise;
};













let closeConsole = Task.async(function* (tab) {
  let target = TargetFactory.forTab(tab || gBrowser.selectedTab);
  let toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    yield toolbox.destroy();
  }
});

















function waitForContextMenu(popup, button, onShown, onHidden) {
  let deferred = promise.defer();

  function onPopupShown() {
    info("onPopupShown");
    popup.removeEventListener("popupshown", onPopupShown);

    onShown && onShown();

    
    popup.addEventListener("popuphidden", onPopupHidden);
    executeSoon(() => popup.hidePopup());
  }
  function onPopupHidden() {
    info("onPopupHidden");
    popup.removeEventListener("popuphidden", onPopupHidden);

    onHidden && onHidden();

    deferred.resolve(popup);
  }

  popup.addEventListener("popupshown", onPopupShown);

  info("wait for the context menu to open");
  let eventDetails = {type: "contextmenu", button: 2};
  EventUtils.synthesizeMouse(button, 2, 2, eventDetails,
                             button.ownerDocument.defaultView);
  return deferred.promise;
}






let waitForTab = Task.async(function*() {
  info("Waiting for a tab to open");
  yield once(gBrowser.tabContainer, "TabOpen");
  let tab = gBrowser.selectedTab;
  let browser = tab.linkedBrowser;
  yield once(browser, "load", true);
  info("The tab load completed");
  return tab;
});




function dumpConsoles() {
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







function dumpMessageElement(message) {
  let text = message.textContent;
  let repeats = message.querySelector(".message-repeats");
  if (repeats) {
    repeats = repeats.getAttribute("value");
  }
  console.debug("id", message.getAttribute("id"),
                "date", message.timestamp,
                "class", message.className,
                "category", message.category,
                "severity", message.severity,
                "repeats", repeats,
                "clipboardText", message.clipboardText,
                "text", text);
}

let finishTest = Task.async(function* () {
  dumpConsoles();

  let browserConsole = HUDService.getBrowserConsole();
  if (browserConsole) {
    if (browserConsole.jsterm) {
      browserConsole.jsterm.clearOutput(true);
    }
    yield HUDService.toggleBrowserConsole();
  }

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  finish();
});

registerCleanupFunction(function*() {
  DevToolsUtils.testing = false;

  
  yield asyncStorage.removeItem("webConsoleHistory");

  dumpConsoles();

  if (HUDService.getBrowserConsole()) {
    HUDService.toggleBrowserConsole();
  }

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});

waitForExplicitFinish();




















function waitForSuccess(options) {
  let deferred = promise.defer();
  let start = Date.now();
  let timeout = options.timeout || 5000;
  let {validator} = options;

  function wait() {
    if ((Date.now() - start) > timeout) {
      
      ok(false, "Timed out while waiting for: " + options.name);
      deferred.reject(null);
      return;
    }

    if (validator(options)) {
      ok(true, options.name);
      deferred.resolve(null);
    } else {
      setTimeout(wait, 100);
    }
  }

  setTimeout(wait, 100);

  return deferred.promise;
}

let openInspector = Task.async(function* (tab = gBrowser.selectedTab) {
  let target = TargetFactory.forTab(tab);
  let toolbox = yield gDevTools.showToolbox(target, "inspector");
  return toolbox.getCurrentPanel();
});

























function findVariableViewProperties(view, rules, options) {
  
  function init() {
    
    
    let expandRules = [];
    let filterRules = rules.filter((rule) => {
      if (typeof rule.name == "string" && rule.name.indexOf(".") > -1) {
        expandRules.push(rule);
        return false;
      }
      return true;
    });

    
    
    
    let outstanding = [];
    finder(filterRules, view, outstanding);

    
    let lastStep = processExpandRules.bind(null, expandRules);

    
    let returnResults = onAllRulesMatched.bind(null, rules);

    return promise.all(outstanding).then(lastStep).then(returnResults);
  }

  function onMatch(prop, rule, matched) {
    if (matched && !rule.matchedProp) {
      rule.matchedProp = prop;
    }
  }

  function finder(rules, vars, promises) {
    for (let [, prop] of vars) {
      for (let rule of rules) {
        let matcher = matchVariablesViewProperty(prop, rule, options);
        promises.push(matcher.then(onMatch.bind(null, prop, rule)));
      }
    }
  }

  function processExpandRules(rules) {
    let rule = rules.shift();
    if (!rule) {
      return promise.resolve(null);
    }

    let deferred = promise.defer();
    let expandOptions = {
      rootVariable: view,
      expandTo: rule.name,
      webconsole: options.webconsole,
    };

    variablesViewExpandTo(expandOptions).then(function onSuccess(prop) {
      let name = rule.name;
      let lastName = name.split(".").pop();
      rule.name = lastName;

      let matched = matchVariablesViewProperty(prop, rule, options);
      return matched.then(onMatch.bind(null, prop, rule)).then(function() {
        rule.name = name;
      });
    }, function onFailure() {
      return promise.resolve(null);
    }).then(processExpandRules.bind(null, rules)).then(function() {
      deferred.resolve(null);
    });

    return deferred.promise;
  }

  function onAllRulesMatched(rules) {
    for (let rule of rules) {
      let matched = rule.matchedProp;
      if (matched && !rule.dontMatch) {
        ok(true, "rule " + rule.name + " matched for property " + matched.name);
      } else if (matched && rule.dontMatch) {
        ok(false, "rule " + rule.name + " should not match property " +
           matched.name);
      } else {
        ok(rule.dontMatch, "rule " + rule.name + " did not match any property");
      }
    }
    return rules;
  }

  return init();
}

















function matchVariablesViewProperty(prop, rule, options) {
  function resolve(result) {
    return promise.resolve(result);
  }

  if (rule.name) {
    let match = rule.name instanceof RegExp ?
                rule.name.test(prop.name) :
                prop.name == rule.name;
    if (!match) {
      return resolve(false);
    }
  }

  if (rule.value) {
    let displayValue = prop.displayValue;
    if (prop.displayValueClassName == "token-string") {
      displayValue = displayValue.substring(1, displayValue.length - 1);
    }

    let match = rule.value instanceof RegExp ?
                rule.value.test(displayValue) :
                displayValue == rule.value;
    if (!match) {
      info("rule " + rule.name + " did not match value, expected '" +
           rule.value + "', found '" + displayValue + "'");
      return resolve(false);
    }
  }

  if ("isGetter" in rule) {
    let isGetter = !!(prop.getter && prop.get("get"));
    if (rule.isGetter != isGetter) {
      info("rule " + rule.name + " getter test failed");
      return resolve(false);
    }
  }

  if ("isGenerator" in rule) {
    let isGenerator = prop.displayValue == "Generator";
    if (rule.isGenerator != isGenerator) {
      info("rule " + rule.name + " generator test failed");
      return resolve(false);
    }
  }

  let outstanding = [];

  if ("isIterator" in rule) {
    let isIterator = isVariableViewPropertyIterator(prop, options.webconsole);
    outstanding.push(isIterator.then((result) => {
      if (result != rule.isIterator) {
        info("rule " + rule.name + " iterator test failed");
      }
      return result == rule.isIterator;
    }));
  }

  outstanding.push(promise.resolve(true));

  return promise.all(outstanding).then(function _onMatchDone(results) {
    let ruleMatched = results.indexOf(false) == -1;
    return resolve(ruleMatched);
  });
}













function isVariableViewPropertyIterator(prop, webConsole) {
  if (prop.displayValue == "Iterator") {
    return promise.resolve(true);
  }

  let deferred = promise.defer();

  variablesViewExpandTo({
    rootVariable: prop,
    expandTo: "__proto__.__iterator__",
    webconsole: webConsole,
  }).then(function onSuccess() {
    deferred.resolve(true);
  }, function onFailure() {
    deferred.resolve(false);
  });

  return deferred.promise;
}


















function variablesViewExpandTo(options) {
  let root = options.rootVariable;
  let expandTo = options.expandTo.split(".");
  let jsterm = (options.webconsole || {}).jsterm;
  let lastDeferred = promise.defer();

  function fetch(prop) {
    if (!prop.onexpand) {
      ok(false, "property " + prop.name + " cannot be expanded: !onexpand");
      return promise.reject(prop);
    }

    let deferred = promise.defer();

    if (prop._fetched || !jsterm) {
      executeSoon(function() {
        deferred.resolve(prop);
      });
    } else {
      jsterm.once("variablesview-fetched", function _onFetchProp() {
        executeSoon(() => deferred.resolve(prop));
      });
    }

    prop.expand();

    return deferred.promise;
  }

  function getNext(prop) {
    let name = expandTo.shift();
    let newProp = prop.get(name);

    if (expandTo.length > 0) {
      ok(newProp, "found property " + name);
      if (newProp) {
        fetch(newProp).then(getNext, fetchError);
      } else {
        lastDeferred.reject(prop);
      }
    } else if (newProp) {
      lastDeferred.resolve(newProp);
    } else {
      lastDeferred.reject(prop);
    }
  }

  function fetchError(prop) {
    lastDeferred.reject(prop);
  }

  if (!root._fetched) {
    fetch(root).then(getNext, fetchError);
  } else {
    getNext(root);
  }

  return lastDeferred.promise;
}















let updateVariablesViewProperty = Task.async(function* (options) {
  let view = options.property._variablesView;
  view.window.focus();
  options.property.focus();

  switch (options.field) {
    case "name":
      EventUtils.synthesizeKey("VK_RETURN", { shiftKey: true }, view.window);
      break;
    case "value":
      EventUtils.synthesizeKey("VK_RETURN", {}, view.window);
      break;
    default:
      throw new Error("options.field is incorrect");
  }

  let deferred = promise.defer();

  executeSoon(() => {
    EventUtils.synthesizeKey("A", { accelKey: true }, view.window);

    for (let c of options.string) {
      EventUtils.synthesizeKey(c, {}, view.window);
    }

    if (options.webconsole) {
      options.webconsole.jsterm.once("variablesview-fetched")
        .then((varView) => deferred.resolve(varView));
    }

    EventUtils.synthesizeKey("VK_RETURN", {}, view.window);

    if (!options.webconsole) {
      executeSoon(() => {
        deferred.resolve(null);
      });
    }
  });

  return deferred.promise;
});
















function openDebugger(options = {}) {
  if (!options.tab) {
    options.tab = gBrowser.selectedTab;
  }

  let deferred = promise.defer();

  let target = TargetFactory.forTab(options.tab);
  let toolbox = gDevTools.getToolbox(target);
  let dbgPanelAlreadyOpen = toolbox && toolbox.getPanel("jsdebugger");

  gDevTools.showToolbox(target, "jsdebugger").then(function onSuccess(tool) {
    let panel = tool.getCurrentPanel();
    let panelWin = panel.panelWin;

    panel._view.Variables.lazyEmpty = false;

    let resolveObject = {
      target: target,
      toolbox: tool,
      panel: panel,
      panelWin: panelWin,
    };

    if (dbgPanelAlreadyOpen) {
      deferred.resolve(resolveObject);
    } else {
      panelWin.once(panelWin.EVENTS.SOURCES_ADDED, () => {
        deferred.resolve(resolveObject);
      });
    }
  }, function onFailure(reason) {
    console.debug("failed to open the toolbox for 'jsdebugger'", reason);
    deferred.reject(reason);
  });

  return deferred.promise;
}









function isDebuggerCaretPos(panel, line, col = 1) {
  let editor = panel.panelWin.DebuggerView.editor;
  let cursor = editor.getCursor();

  
  info("Current editor caret position: " + (cursor.line + 1) + ", " +
    (cursor.ch + 1));
  return cursor.line == (line - 1) && cursor.ch == (col - 1);
}







































































function waitForMessages(options) {
  info("Waiting for messages...");

  gPendingOutputTest++;
  let webconsole = options.webconsole;
  let rules = WebConsoleUtils.cloneObject(options.messages, true);
  let rulesMatched = 0;
  let listenerAdded = false;
  let deferred = promise.defer();
  options.matchCondition = options.matchCondition || "all";

  function checkText(rule, text) {
    let result = false;
    if (Array.isArray(rule)) {
      result = rule.every((s) => checkText(s, text));
    } else if (typeof rule == "string") {
      result = text.indexOf(rule) > -1;
    } else if (rule instanceof RegExp) {
      result = rule.test(text);
    } else {
      result = rule == text;
    }
    return result;
  }

  function checkConsoleTable(rule, element) {
    let elemText = element.textContent;

    if (!checkText("console.table():", elemText)) {
      return false;
    }

    rule.category = CATEGORY_WEBDEV;
    rule.severity = SEVERITY_LOG;
    rule.type = Messages.ConsoleTable;

    return true;
  }

  function checkConsoleTrace(rule, element) {
    let elemText = element.textContent;
    let trace = rule.consoleTrace;

    if (!checkText("console.trace():", elemText)) {
      return false;
    }

    rule.category = CATEGORY_WEBDEV;
    rule.severity = SEVERITY_LOG;
    rule.type = Messages.ConsoleTrace;

    if (!rule.stacktrace && typeof trace == "object" && trace !== true) {
      if (Array.isArray(trace)) {
        rule.stacktrace = trace;
      } else {
        rule.stacktrace = [trace];
      }
    }

    return true;
  }

  function checkConsoleTime(rule, element) {
    let elemText = element.textContent;
    let time = rule.consoleTime;

    if (!checkText(time + ": timer started", elemText)) {
      return false;
    }

    rule.category = CATEGORY_WEBDEV;
    rule.severity = SEVERITY_LOG;

    return true;
  }

  function checkConsoleTimeEnd(rule, element) {
    let elemText = element.textContent;
    let time = rule.consoleTimeEnd;
    let regex = new RegExp(time + ": -?\\d+([,.]\\d+)?ms");

    if (!checkText(regex, elemText)) {
      return false;
    }

    rule.category = CATEGORY_WEBDEV;
    rule.severity = SEVERITY_LOG;

    return true;
  }

  function checkConsoleDir(rule, element) {
    if (!element.classList.contains("inlined-variables-view")) {
      return false;
    }

    let elemText = element.textContent;
    if (!checkText(rule.consoleDir, elemText)) {
      return false;
    }

    let iframe = element.querySelector("iframe");
    if (!iframe) {
      ok(false, "console.dir message has no iframe");
      return false;
    }

    return true;
  }

  function checkConsoleGroup(rule) {
    if (!isNaN(parseInt(rule.consoleGroup, 10))) {
      rule.groupDepth = rule.consoleGroup;
    }
    rule.category = CATEGORY_WEBDEV;
    rule.severity = SEVERITY_LOG;

    return true;
  }

  function checkSource(rule, element) {
    let location = element.querySelector(".message-location");
    if (!location) {
      return false;
    }

    if (!checkText(rule.source.url, location.getAttribute("title"))) {
      return false;
    }

    if ("line" in rule.source && location.sourceLine != rule.source.line) {
      return false;
    }

    return true;
  }

  function checkCollapsible(rule, element) {
    let msg = element._messageObject;
    if (!msg || !!msg.collapsible != rule.collapsible) {
      return false;
    }

    return true;
  }

  function checkStacktrace(rule, element) {
    let stack = rule.stacktrace;
    let frames = element.querySelectorAll(".stacktrace > li");
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
          displayErrorContext(rule, element);
          return false;
        }
      }

      if (expected.fn) {
        let fn = frame.querySelector(".function").textContent;
        if (!checkText(expected.fn, fn)) {
          ok(false, "frame #" + i + " does not match the function name: " +
                    expected.fn);
          displayErrorContext(rule, element);
          return false;
        }
      }

      if (expected.line) {
        let line = frame.querySelector(".message-location").sourceLine;
        if (!checkText(expected.line, line)) {
          ok(false, "frame #" + i + " does not match the line number: " +
                    expected.line);
          displayErrorContext(rule, element);
          return false;
        }
      }
    }

    return true;
  }

  function hasXhrLabel(element) {
    let xhr = element.querySelector(".xhr");
    if (!xhr) {
      return false;
    }
    return true;
  }

  function checkMessage(rule, element) {
    let elemText = element.textContent;

    if (rule.text && !checkText(rule.text, elemText)) {
      return false;
    }

    if (rule.noText && checkText(rule.noText, elemText)) {
      return false;
    }

    if (rule.consoleTable && !checkConsoleTable(rule, element)) {
      return false;
    }

    if (rule.consoleTrace && !checkConsoleTrace(rule, element)) {
      return false;
    }

    if (rule.consoleTime && !checkConsoleTime(rule, element)) {
      return false;
    }

    if (rule.consoleTimeEnd && !checkConsoleTimeEnd(rule, element)) {
      return false;
    }

    if (rule.consoleDir && !checkConsoleDir(rule, element)) {
      return false;
    }

    if (rule.consoleGroup && !checkConsoleGroup(rule, element)) {
      return false;
    }

    if (rule.source && !checkSource(rule, element)) {
      return false;
    }

    if ("collapsible" in rule && !checkCollapsible(rule, element)) {
      return false;
    }

    if (rule.isXhr && !hasXhrLabel(element)) {
      return false;
    }

    if (!rule.isXhr && hasXhrLabel(element)) {
      return false;
    }

    let partialMatch = !!(rule.consoleTrace || rule.consoleTime ||
                          rule.consoleTimeEnd);

    
    
    if (rule.type) {
      if (!element._messageObject ||
          !(element._messageObject instanceof rule.type)) {
        if (partialMatch) {
          ok(false, "message type for rule: " + displayRule(rule));
          displayErrorContext(rule, element);
        }
        return false;
      }
      partialMatch = true;
    }

    if ("category" in rule && element.category != rule.category) {
      if (partialMatch) {
        is(element.category, rule.category,
           "message category for rule: " + displayRule(rule));
        displayErrorContext(rule, element);
      }
      return false;
    }

    if ("severity" in rule && element.severity != rule.severity) {
      if (partialMatch) {
        is(element.severity, rule.severity,
           "message severity for rule: " + displayRule(rule));
        displayErrorContext(rule, element);
      }
      return false;
    }

    if (rule.text) {
      partialMatch = true;
    }

    if (rule.stacktrace && !checkStacktrace(rule, element)) {
      if (partialMatch) {
        ok(false, "failed to match stacktrace for rule: " + displayRule(rule));
        displayErrorContext(rule, element);
      }
      return false;
    }

    if (rule.category == CATEGORY_NETWORK && "url" in rule &&
        !checkText(rule.url, element.url)) {
      return false;
    }

    if ("repeats" in rule) {
      let repeats = element.querySelector(".message-repeats");
      if (!repeats || repeats.getAttribute("value") != rule.repeats) {
        return false;
      }
    }

    if ("groupDepth" in rule) {
      let indentNode = element.querySelector(".indent");
      let indent = (GROUP_INDENT * rule.groupDepth) + "px";
      if (!indentNode || indentNode.style.width != indent) {
        is(indentNode.style.width, indent,
           "group depth check failed for message rule: " + displayRule(rule));
        return false;
      }
    }

    if ("longString" in rule) {
      let longStrings = element.querySelectorAll(".longStringEllipsis");
      if (rule.longString != !!longStrings[0]) {
        if (partialMatch) {
          is(!!longStrings[0], rule.longString,
             "long string existence check failed for message rule: " +
             displayRule(rule));
          displayErrorContext(rule, element);
        }
        return false;
      }
      rule.longStrings = longStrings;
    }

    if ("objects" in rule) {
      let clickables = element.querySelectorAll(".message-body a");
      if (rule.objects != !!clickables[0]) {
        if (partialMatch) {
          is(!!clickables[0], rule.objects,
             "objects existence check failed for message rule: " +
             displayRule(rule));
          displayErrorContext(rule, element);
        }
        return false;
      }
      rule.clickableElements = clickables;
    }

    if ("prefix" in rule) {
      let prefixNode = element.querySelector(".prefix");
      is(prefixNode && prefixNode.textContent, rule.prefix, "Check prefix");
    }

    let count = rule.count || 1;
    if (!rule.matched) {
      rule.matched = new Set();
    }
    rule.matched.add(element);

    return rule.matched.size == count;
  }

  function onMessagesAdded(event, newMessages) {
    for (let msg of newMessages) {
      let elem = msg.node;
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

  function allRulesMatched() {
    return options.matchCondition == "all" && rulesMatched == rules.length ||
           options.matchCondition == "any" && rulesMatched > 0;
  }

  function maybeDone() {
    if (allRulesMatched()) {
      if (listenerAdded) {
        webconsole.ui.off("new-messages", onMessagesAdded);
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
      webconsole.ui.off("new-messages", onMessagesAdded);
    }

    for (let rule of rules) {
      if (!rule._ruleMatched) {
        ok(false, "failed to match rule: " + displayRule(rule));
      }
    }
  }

  function displayRule(rule) {
    return rule.name || rule.text;
  }

  function displayErrorContext(rule, element) {
    console.log("error occured during rule " + displayRule(rule));
    console.log("while checking the following message");
    dumpMessageElement(element);
  }

  executeSoon(() => {
    let messages = [];
    for (let elem of webconsole.outputNode.childNodes) {
      messages.push({
        node: elem,
        update: false,
      });
    }

    onMessagesAdded("new-messages", messages);

    if (!allRulesMatched()) {
      listenerAdded = true;
      registerCleanupFunction(testCleanup);
      webconsole.ui.on("new-messages", onMessagesAdded);
    }
  });

  return deferred.promise;
}

function whenDelayedStartupFinished(win, callback) {
  Services.obs.addObserver(function observer(subject, topic) {
    if (win == subject) {
      Services.obs.removeObserver(observer, topic);
      executeSoon(callback);
    }
  }, "browser-delayed-startup-finished", false);
}










































function checkOutputForInputs(hud, inputTests) {
  let container = gBrowser.tabContainer;

  function* runner() {
    for (let [i, entry] of inputTests.entries()) {
      info("checkInput(" + i + "): " + entry.input);
      yield checkInput(entry);
    }
    container = null;
  }

  function* checkInput(entry) {
    yield checkConsoleLog(entry);
    yield checkPrintOutput(entry);
    yield checkJSEval(entry);
  }

  function* checkConsoleLog(entry) {
    info("Logging: " + entry.input);
    hud.jsterm.clearOutput();
    hud.jsterm.execute("console.log(" + entry.input + ")");

    let consoleOutput = "consoleOutput" in entry ?
                        entry.consoleOutput : entry.output;

    let [result] = yield waitForMessages({
      webconsole: hud,
      messages: [{
        name: "console.log() output: " + consoleOutput,
        text: consoleOutput,
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      }],
    });

    if (typeof entry.inspectorIcon == "boolean") {
      let msg = [...result.matched][0];
      info("Checking Inspector Link: " + entry.input);
      yield checkLinkToInspector(entry.inspectorIcon, msg);
    }
  }

  function checkPrintOutput(entry) {
    info("Printing: " + entry.input);
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

  function* checkJSEval(entry) {
    info("Evaluating: " + entry.input);
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
      info("Checking Inspector Link: " + entry.input);
      yield checkLinkToInspector(entry.inspectorIcon, msg);
    }
  }

  function* checkObjectClick(entry, msg) {
    info("Clicking: " + entry.input);
    let body = msg.querySelector(".message-body a") ||
               msg.querySelector(".message-body");
    ok(body, "the message body");

    let deferredVariablesView = promise.defer();
    entry._onVariablesViewOpen = onVariablesViewOpen.bind(null, entry,
                                                          deferredVariablesView);
    hud.jsterm.on("variablesview-open", entry._onVariablesViewOpen);

    let deferredTab = promise.defer();
    entry._onTabOpen = onTabOpen.bind(null, entry, deferredTab);
    container.addEventListener("TabOpen", entry._onTabOpen, true);

    body.scrollIntoView();
    EventUtils.synthesizeMouse(body, 2, 2, {}, hud.iframeWindow);

    if (entry.inspectable) {
      info("message body tagName '" + body.tagName + "' className '" +
           body.className + "'");
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

  function onVariablesViewOpen(entry, {resolve, reject}, event, view, options) {
    info("Variables view opened: " + entry.input);
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

  function onTabOpen(entry, {resolve, reject}, event) {
    container.removeEventListener("TabOpen", entry._onTabOpen, true);
    entry._onTabOpen = null;

    let tab = event.target;
    let browser = gBrowser.getBrowserForTab(tab);
    loadBrowser(browser).then(() => {
      let uri = content.location.href;
      ok(entry.expectedTab && entry.expectedTab == uri,
         "opened tab '" + uri + "', expected tab '" + entry.expectedTab + "'");
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








function checkLinkToInspector(hasLinkToInspector, msg) {
  let elementNodeWidget = [...msg._messageObject.widgets][0];
  if (!elementNodeWidget) {
    ok(!hasLinkToInspector, "The message has no ElementNode widget");
    return true;
  }

  return elementNodeWidget.linkToInspector().then(() => {
    
    if (hasLinkToInspector) {
      ok(msg.querySelectorAll(".open-inspector").length,
        "The ElementNode widget is linked to the inspector");
    } else {
      ok(!msg.querySelectorAll(".open-inspector").length,
        "The ElementNode widget isn't linked to the inspector");
    }
  }, () => {
    
    ok(!hasLinkToInspector,
       "The ElementNode widget isn't linked to the inspector");
  });
}

function getSourceActor(sources, URL) {
  let item = sources.getItemForAttachment(a => a.source.url === URL);
  return item && item.value;
}





function simulateMessageLinkClick(element, expectedLink) {
  let deferred = promise.defer();

  
  
  let oldOpenUILinkIn = window.openUILinkIn;
  window.openUILinkIn = function(link) {
    if (link == expectedLink) {
      ok(true, "Clicking the message link opens the desired page");
      window.openUILinkIn = oldOpenUILinkIn;
      deferred.resolve();
    }
  };

  let event = new MouseEvent("click", {
    detail: 1,
    button: 0,
    bubbles: true,
    cancelable: true
  });
  element.dispatchEvent(event);

  return deferred.promise;
}
