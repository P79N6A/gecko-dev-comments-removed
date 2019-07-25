


let DOMUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);

let doc;
let div;

let pseudo = ":hover";

function test()
{
  waitForExplicitFinish();
  ignoreAllUncaughtExceptions();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    doc = content.document;
    waitForFocus(createDocument, content);
  }, true);

  content.location = "data:text/html,pseudo-class lock tests";
}

function createDocument()
{
  div = doc.createElement("div");
  div.textContent = "test div";

  let head = doc.getElementsByTagName('head')[0];
  let style = doc.createElement('style');
  let rules = doc.createTextNode('div { color: red; } div:hover { color: blue; }');

  style.appendChild(rules);
  head.appendChild(style);
  doc.body.appendChild(div);

  setupTests();
}

function setupTests()
{
  Services.obs.addObserver(selectNode,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.openInspectorUI();
}

function selectNode()
{
  Services.obs.removeObserver(selectNode,
    InspectorUI.INSPECTOR_NOTIFICATIONS.OPENED);

  executeSoon(function() {
    InspectorUI.highlighter.addListener("locked", openRuleView);
    InspectorUI.inspectNode(div);
    InspectorUI.stopInspecting();
  });
}

function openRuleView()
{
  InspectorUI.sidebar.show();
  InspectorUI.currentInspector.once("sidebaractivated-ruleview", performTests);
  InspectorUI.sidebar.activatePanel("ruleview");
}

function performTests()
{
  InspectorUI.highlighter.removeListener("locked", performTests);

  
  InspectorUI.highlighter.pseudoClassLockToggled(pseudo);

  testAdded();

  
  InspectorUI.highlighter.pseudoClassLockToggled(pseudo);

  testRemoved();
  testRemovedFromUI();

  
  InspectorUI.highlighter.pseudoClassLockToggled(pseudo);  

  
  Services.obs.addObserver(testInspectorClosed,
    InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED, false);
  InspectorUI.closeInspectorUI();
}

function testAdded()
{
  
  let node = div;
  do {
    is(DOMUtils.hasPseudoClassLock(node, pseudo), true,
       "pseudo-class lock has been applied");
    node = node.parentNode;
  } while (node.parentNode)

  
  let pseudoClassesBox = document.getElementById("highlighter-nodeinfobar-pseudo-classes");
  is(pseudoClassesBox.textContent, pseudo, "pseudo-class in infobar selector");
  
  
  is(ruleView().element.children.length, 3,
     "rule view is showing 3 rules for pseudo-class locked div");
     
  is(ruleView().element.children[1]._ruleEditor.rule.selectorText,
     "div:hover", "rule view is showing " + pseudo + " rule");
}

function testRemoved()
{
  
  let node = div;
  do {
    is(DOMUtils.hasPseudoClassLock(node, pseudo), false,
       "pseudo-class lock has been removed");
    node = node.parentNode;
  } while (node.parentNode)
}

function testRemovedFromUI()
{
  
  let pseudoClassesBox = document.getElementById("highlighter-nodeinfobar-pseudo-classes");
  is(pseudoClassesBox.textContent, "", "pseudo-class removed from infobar selector");    

  
  is(ruleView().element.children.length, 2,
     "rule view is showing 2 rules after removing lock");    
}

function testInspectorClosed()
{
  Services.obs.removeObserver(testInspectorClosed,
    InspectorUI.INSPECTOR_NOTIFICATIONS.CLOSED);

  testRemoved();

  finishUp();  
}

function finishUp()
{
  doc = div = null;
  gBrowser.removeCurrentTab();
  finish();
}
