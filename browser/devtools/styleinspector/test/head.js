



"use strict";

const Cu = Components.utils;
let {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let TargetFactory = devtools.TargetFactory;
let {CssHtmlTree} = devtools.require("devtools/styleinspector/computed-view");
let {CssRuleView, _ElementStyle} = devtools.require("devtools/styleinspector/rule-view");
let {CssLogic, CssSelector} = devtools.require("devtools/styleinspector/css-logic");
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
let {editableField, getInplaceEditorForSpan: inplaceEditor} = devtools.require("devtools/shared/inplace-editor");
let {console} = Components.utils.import("resource://gre/modules/devtools/Console.jsm", {});


waitForExplicitFinish();

const TEST_URL_ROOT = "http://example.com/browser/browser/devtools/styleinspector/test/";
const TEST_URL_ROOT_SSL = "https://example.com/browser/browser/devtools/styleinspector/test/";
const ROOT_TEST_DIR = getRootDirectory(gTestPath);
const FRAME_SCRIPT_URL = ROOT_TEST_DIR + "doc_frame_script.js";


registerCleanupFunction(function*() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});





gDevTools.testing = true;
registerCleanupFunction(() => gDevTools.testing = false);



registerCleanupFunction(() => {
  Services.prefs.clearUserPref("devtools.inspector.activeSidebar");
  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.defaultColorUnit");
});














































function addTab(url) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  window.focus();

  let tab = window.gBrowser.selectedTab = window.gBrowser.addTab(url);
  let browser = tab.linkedBrowser;

  info("Loading the helper frame script " + FRAME_SCRIPT_URL);
  browser.messageManager.loadFrameScript(FRAME_SCRIPT_URL, false);

  browser.addEventListener("load", function onload() {
    browser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");

    def.resolve(tab);
  }, true);

  return def.promise;
}








function getNode(nodeOrSelector) {
  info("Getting the node for '" + nodeOrSelector + "'");
  return typeof nodeOrSelector === "string" ?
    content.document.querySelector(nodeOrSelector) :
    nodeOrSelector;
}








function getNodeFront(selector, {walker}) {
  return walker.querySelector(walker.rootNode, selector);
}









let selectAndHighlightNode = Task.async(function*(selector, inspector) {
  info("Highlighting and selecting the node for " + selector);

  let nodeFront = yield getNodeFront(selector, inspector);
  let updated = inspector.toolbox.once("highlighter-ready");
  inspector.selection.setNodeFront(nodeFront, "test-highlight");
  yield updated;
});














let selectNode = Task.async(function*(data, inspector, reason="test") {
  info("Selecting the node for '" + data + "'");
  let nodeFront = data;
  if (!data._form) {
    nodeFront = yield getNodeFront(data, inspector);
  }
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNodeFront(nodeFront, reason);
  yield updated;
});







function clearCurrentNodeSelection(inspector) {
  info("Clearing the current selection");
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNodeFront(null);
  return updated;
}





let openInspector = Task.async(function*() {
  info("Opening the inspector");
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  let inspector, toolbox;

  
  
  
  toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    inspector = toolbox.getPanel("inspector");
    if (inspector) {
      info("Toolbox and inspector already open");
      return {
        toolbox: toolbox,
        inspector: inspector
      };
    }
  }

  info("Opening the toolbox");
  toolbox = yield gDevTools.showToolbox(target, "inspector");
  yield waitForToolboxFrameFocus(toolbox);
  inspector = toolbox.getPanel("inspector");

  info("Waiting for the inspector to update");
  yield inspector.once("inspector-updated");

  return {
    toolbox: toolbox,
    inspector: inspector
  };
});






function waitForToolboxFrameFocus(toolbox) {
  info("Making sure that the toolbox's frame is focused");
  let def = promise.defer();
  let win = toolbox.frame.contentWindow;
  waitForFocus(def.resolve, win);
  return def.promise;
}







let openInspectorSideBar = Task.async(function*(id) {
  let {toolbox, inspector} = yield openInspector();

  if (!hasSideBarTab(inspector, id)) {
    info("Waiting for the " + id + " sidebar to be ready");
    yield inspector.sidebar.once(id + "-ready");
  }

  info("Selecting the " + id + " sidebar");
  inspector.sidebar.select(id);

  return {
    toolbox: toolbox,
    inspector: inspector,
    view: inspector.sidebar.getWindowForTab(id)[id].view
  };
});







function openComputedView() {
  return openInspectorSideBar("computedview");
}







function openRuleView() {
  return openInspectorSideBar("ruleview");
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







function wait(ms) {
  let def = promise.defer();
  content.setTimeout(def.resolve, ms);
  return def.promise;
}








function waitForContentMessage(name) {
  info("Expecting message " + name + " from content");

  let mm = gBrowser.selectedBrowser.messageManager;

  let def = promise.defer();
  mm.addMessageListener(name, function onMessage(msg) {
    mm.removeMessageListener(name, onMessage);
    def.resolve(msg.data);
  });
  return def.promise;
}













function executeInContent(name, data={}, objects={}, expectResponse=true) {
  info("Sending message " + name + " to content");
  let mm = gBrowser.selectedBrowser.messageManager;

  mm.sendAsyncMessage(name, data, objects);
  if (expectResponse) {
    return waitForContentMessage(name);
  } else {
    return promise.resolve();
  }
}








function* getComputedStyleProperty(selector, pseudo, propName) {
 return yield executeInContent("Test:GetComputedStylePropertyValue",
                               {selector,
                                pseudo,
                                name: propName});
}










function* waitForComputedStyleProperty(selector, pseudo, name, expected) {
 return yield executeInContent("Test:WaitForComputedStylePropertyValue",
                               {selector,
                                pseudo,
                                expected,
                                name});
}






let focusEditableField = Task.async(function*(editable, xOffset=1, yOffset=1, options={}) {
  let onFocus = once(editable.parentNode, "focus", true);

  info("Clicking on editable field to turn to edit mode");
  EventUtils.synthesizeMouse(editable, xOffset, yOffset, options,
    editable.ownerDocument.defaultView);
  let event = yield onFocus;

  info("Editable field gained focus, returning the input field now");
  return inplaceEditor(editable.ownerDocument.activeElement);
});








function isHoverTooltipTarget(tooltip, target) {
  if (!tooltip._basedNode || !tooltip.panel) {
    return promise.reject(new Error(
      "The tooltip passed isn't set to toggle on hover or is not a tooltip"));
  }
  return tooltip.isValidHoverTarget(target);
}






function assertHoverTooltipOn(tooltip, element) {
  return isHoverTooltipTarget(tooltip, element).then(() => {
    ok(true, "A tooltip is defined on hover of the given element");
  }, () => {
    ok(false, "No tooltip is defined on hover of the given element");
  });
}






function assertNoHoverTooltipOn(tooltip, element) {
  return isHoverTooltipTarget(tooltip, element).then(() => {
    ok(false, "A tooltip is defined on hover of the given element");
  }, () => {
    ok(true, "No tooltip is defined on hover of the given element");
  });
}







function waitForWindow() {
  let def = promise.defer();

  info("Waiting for a window to open");
  Services.ww.registerNotification(function onWindow(subject, topic) {
    if (topic != "domwindowopened") {
      return;
    }
    info("A window has been opened");
    let win = subject.QueryInterface(Ci.nsIDOMWindow);
    once(win, "load").then(() => {
      info("The window load completed");
      Services.ww.unregisterNotification(onWindow);
      def.resolve(win);
    });
  });

  return def.promise;
}









function waitForClipboard(setup, expected) {
  let def = promise.defer();
  SimpleTest.waitForClipboard(expected, setup, def.resolve, def.reject);
  return def.promise;
}




function fireCopyEvent(element) {
  let evt = element.ownerDocument.createEvent("Event");
  evt.initEvent("copy", true, true);
  element.dispatchEvent(evt);
}












function waitForSuccess(validatorFn, name="untitled") {
  let def = promise.defer();

  function wait(validatorFn) {
    if (validatorFn()) {
      ok(true, "Validator function " + name + " returned true");
      def.resolve();
    } else {
      setTimeout(() => wait(validatorFn), 200);
    }
  }
  wait(validatorFn);

  return def.promise;
}








function addStyle(doc, style) {
  info("Adding a new style tag to the document with style content: " +
    style.substring(0, 50));
  let node = doc.createElement('style');
  node.setAttribute("type", "text/css");
  node.textContent = style;
  doc.getElementsByTagName("head")[0].appendChild(node);
  return node;
}








function hasSideBarTab(inspector, id) {
  return !!inspector.sidebar.getWindowForTab(id);
}








let getFontFamilyDataURL = Task.async(function*(font, nodeFront) {
  let fillStyle = (Services.prefs.getCharPref("devtools.theme") === "light") ?
      "black" : "white";

  let {data} = yield nodeFront.getFontFamilyDataURL(font, fillStyle);
  let dataURL = yield data.string();
  return dataURL;
});






function synthesizeKeys(input, win) {
  for (let key of input.split("")) {
     EventUtils.synthesizeKey(key, {}, win);
  }
}
















function getRuleViewRule(view, selectorText) {
  let rule;
  for (let r of view.doc.querySelectorAll(".ruleview-rule")) {
    let selector = r.querySelector(".ruleview-selector, .ruleview-selector-matched");
    if (selector && selector.textContent === selectorText) {
      rule = r;
      break;
    }
  }

  return rule;
}










function getRuleViewProperty(view, selectorText, propertyName) {
  let prop;

  let rule = getRuleViewRule(view, selectorText);
  if (rule) {
    
    for (let p of rule.querySelectorAll(".ruleview-property")) {
      let nameSpan = p.querySelector(".ruleview-propertyname");
      let valueSpan = p.querySelector(".ruleview-propertyvalue");

      if (nameSpan.textContent === propertyName) {
        prop = {nameSpan: nameSpan, valueSpan: valueSpan};
        break;
      }
    }
  }
  return prop;
}










function getRuleViewPropertyValue(view, selectorText, propertyName) {
  return getRuleViewProperty(view, selectorText, propertyName)
    .valueSpan.textContent;
}








function getRuleViewSelector(view, selectorText) {
  let rule = getRuleViewRule(view, selectorText);
  return rule.querySelector(".ruleview-selector, .ruleview-selector-matched");
}








function getRuleViewSelectorHighlighterIcon(view, selectorText) {
  let rule = getRuleViewRule(view, selectorText);
  return rule.querySelector(".ruleview-selectorhighlighter");
}













let simulateColorPickerChange = Task.async(function*(colorPicker, newRgba, expectedChange) {
  info("Getting the spectrum colorpicker object");
  let spectrum = yield colorPicker.spectrum;
  info("Setting the new color");
  spectrum.rgb = newRgba;
  info("Applying the change");
  spectrum.updateUI();
  spectrum.onChange();

  if (expectedChange) {
    info("Waiting for the style to be applied on the page");
    yield waitForSuccess(() => {
      let {element, name, value} = expectedChange;
      return content.getComputedStyle(element)[name] === value;
    }, "Color picker change applied on the page");
  }
});







function getRuleViewLinkByIndex(view, index) {
  let links = view.doc.querySelectorAll(".ruleview-rule-source");
  return links[index];
}







function getRuleViewLinkTextByIndex(view, index) {
  let link = getRuleViewLinkByIndex(view, index);
  return link.querySelector(".source-link-label").value;
}








function getRuleViewRuleEditor(view, childrenIndex, nodeIndex) {
  return nodeIndex !== undefined ?
    view.element.children[childrenIndex].childNodes[nodeIndex]._ruleEditor :
    view.element.children[childrenIndex]._ruleEditor;
}








let focusNewRuleViewProperty = Task.async(function*(ruleEditor) {
  info("Clicking on a close ruleEditor brace to start editing a new property");
  ruleEditor.closeBrace.scrollIntoView();
  let editor = yield focusEditableField(ruleEditor.closeBrace);

  is(inplaceEditor(ruleEditor.newPropSpan), editor, "Focused editor is the new property editor.");
  is(ruleEditor.rule.textProps.length,  0, "Starting with one new text property.");
  is(ruleEditor.propertyList.children.length, 1, "Starting with two property editors.");

  return editor;
});













let createNewRuleViewProperty = Task.async(function*(ruleEditor, inputValue) {
  info("Creating a new property editor");
  let editor = yield focusNewRuleViewProperty(ruleEditor);

  info("Entering the value " + inputValue);
  editor.input.value = inputValue;

  info("Submitting the new value and waiting for value field focus");
  let onFocus = once(ruleEditor.element, "focus", true);
  EventUtils.synthesizeKey("VK_RETURN", {},
    ruleEditor.element.ownerDocument.defaultView);
  yield onFocus;
});















function getComputedViewProperty(view, name) {
  let prop;
  for (let property of view.styleDocument.querySelectorAll(".property-view")) {
    let nameSpan = property.querySelector(".property-name");
    let valueSpan = property.querySelector(".property-value");

    if (nameSpan.textContent === name) {
      prop = {nameSpan: nameSpan, valueSpan: valueSpan};
      break;
    }
  }
  return prop;
}







function getComputedViewPropertyView(view, name) {
  let propView;
  for (let propertyView of view.propertyViews) {
    if (propertyView._propertyInfo.name === name) {
      propView = propertyView;
      break;
    }
  }
  return propView;
}













let getComputedViewMatchedRules = Task.async(function*(view, name) {
  let expander;
  let propertyContent;
  for (let property of view.styleDocument.querySelectorAll(".property-view")) {
    let nameSpan = property.querySelector(".property-name");
    if (nameSpan.textContent === name) {
      expander = property.querySelector(".expandable");
      propertyContent = property.nextSibling;
      break;
    }
  }

  if (!expander.hasAttribute("open")) {
    
    let onExpand = view.inspector.once("computed-view-property-expanded");
    expander.click();
    yield onExpand;
  }

  return propertyContent;
});








function getComputedViewPropertyValue(view, name, propertyName) {
  return getComputedViewProperty(view, name, propertyName)
    .valueSpan.textContent;
}









function expandComputedViewPropertyByIndex(view, index) {
  info("Expanding property " + index + " in the computed view");
  let expandos = view.styleDocument.querySelectorAll(".expandable");
  if (!expandos.length || !expandos[index]) {
    return promise.reject();
  }

  let onExpand = view.inspector.once("computed-view-property-expanded");
  expandos[index].click();
  return onExpand;
}







function getComputedViewLinkByIndex(view, index) {
  let links = view.styleDocument.querySelectorAll(".rule-link .link");
  return links[index];
}

















function waitForStyleEditor(toolbox, href) {
  let def = promise.defer();

  info("Waiting for the toolbox to switch to the styleeditor");
  toolbox.once("styleeditor-selected").then(() => {
    let panel = toolbox.getCurrentPanel();
    ok(panel && panel.UI, "Styleeditor panel switched to front");

    
    
    let gotEditor = (event, editor) => {
      let currentHref = editor.styleSheet.href;
      if (!href || (href && currentHref.endsWith(href))) {
        info("Stylesheet editor selected");
        panel.UI.off("editor-selected", gotEditor);

        editor.getSourceEditor().then(editor => {
          info("Stylesheet editor fully loaded");
          def.resolve(editor);
        });

        return true;
      }

      info("The editor was incorrect. Waiting for editor-selected event.");
      return false;
    };

    
    
    
    if (!gotEditor("styleeditor-selected", panel.UI.selectedEditor)) {
      
      panel.UI.on("editor-selected", gotEditor);
    }
  });

  return def.promise;
}
