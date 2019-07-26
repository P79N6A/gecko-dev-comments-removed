


let DOMUtils = Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);

let doc;
let parentDiv, div, div2;
let inspector;
let ruleview;

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
  parentDiv = doc.createElement("div");
  parentDiv.textContent = "parent div";

  div = doc.createElement("div");
  div.textContent = "test div";

  div2 = doc.createElement("div");
  div2.textContent = "test div2";

  let head = doc.getElementsByTagName('head')[0];
  let style = doc.createElement('style');
  let rules = doc.createTextNode('div { color: red; } div:hover { color: blue; }');

  style.appendChild(rules);
  head.appendChild(style);
  parentDiv.appendChild(div);
  parentDiv.appendChild(div2);
  doc.body.appendChild(parentDiv);

  openInspector(selectNode);
}

function selectNode(aInspector)
{
  inspector = aInspector;

  waitForView("ruleview", () => {
    ruleview = inspector.sidebar.getWindowForTab("ruleview").ruleview.view;
    inspector.sidebar.select("ruleview");
    inspector.selection.setNode(div, "test");
    inspector.once("inspector-updated", performTests);
  });
}

function performTests()
{
  
  inspector.togglePseudoClass(pseudo);

  
  
  inspector.selection.once("pseudoclass", () => {
    inspector.once("rule-view-refreshed", () => {
      testAdded(() => {
        
        inspector.togglePseudoClass(pseudo);
        inspector.selection.once("pseudoclass", () => {
          inspector.once("rule-view-refreshed", () => {
            testRemoved();
            testRemovedFromUI(() => {
              
              inspector.togglePseudoClass(pseudo);
              inspector.selection.once("pseudoclass", () => {
                inspector.once("rule-view-refreshed", () => {
                  testNavigate(() => {
                    
                    finishUp();
                  });
                });
              });
            });
          });
        });
      });
    });
  });
}

function testNavigate(callback)
{
  inspector.selection.setNode(parentDiv, "test");
  inspector.once("inspector-updated", () => {

    
    is(DOMUtils.hasPseudoClassLock(div, pseudo), true,
      "pseudo-class lock is still applied after inspecting ancestor");

    inspector.selection.setNode(div2, "test");
    inspector.selection.once("pseudoclass", () => {
      
      is(DOMUtils.hasPseudoClassLock(div, pseudo), false,
        "pseudo-class lock is removed after inspecting sibling node");

      
      inspector.selection.setNode(div, "test");
      inspector.once("inspector-updated", () => {
        inspector.togglePseudoClass(pseudo);
        inspector.once("computed-view-refreshed", callback);
      });
    });
  });
}

function showPickerOn(node, cb)
{
  let highlighter = inspector.toolbox.highlighter;
  highlighter.showBoxModel(getNodeFront(node)).then(cb);
}

function testAdded(cb)
{
  
  let node = div;
  do {
    is(DOMUtils.hasPseudoClassLock(node, pseudo), true,
      "pseudo-class lock has been applied");
    node = node.parentNode;
  } while (node.parentNode)

  
  let rules = ruleview.element.querySelectorAll(".ruleview-rule.theme-separator");
  is(rules.length, 3, "rule view is showing 3 rules for pseudo-class locked div");
  is(rules[1]._ruleEditor.rule.selectorText, "div:hover", "rule view is showing " + pseudo + " rule");

  
  showPickerOn(div, () => {
    
    let pseudoClassesBox = getHighlighter().querySelector(".highlighter-nodeinfobar-pseudo-classes");
    is(pseudoClassesBox.textContent, pseudo, "pseudo-class in infobar selector");
    inspector.toolbox.highlighter.hideBoxModel().then(cb);
  });
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

function testRemovedFromUI(cb)
{
  
  let rules = ruleview.element.querySelectorAll(".ruleview-rule.theme-separator");
  is(rules.length, 2, "rule view is showing 2 rules after removing lock");

  showPickerOn(div, () => {
    let pseudoClassesBox = getHighlighter().querySelector(".highlighter-nodeinfobar-pseudo-classes");
    is(pseudoClassesBox.textContent, "", "pseudo-class removed from infobar selector");
    inspector.toolbox.highlighter.hideBoxModel().then(cb);
  });
}

function finishUp()
{
  gDevTools.once("toolbox-destroyed", function() {
    testRemoved();
    inspector = ruleview = null;
    doc = div = null;
    gBrowser.removeCurrentTab();
    finish();
  });

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let toolbox = gDevTools.getToolbox(target);
  toolbox.destroy();
}
