



let doc;

let inspector;
let view;

let {ELEMENT_STYLE} = devtools.require("devtools/server/actors/styles");

function simpleInherit(aInspector, aRuleView)
{
  inspector = aInspector;
  view = aRuleView;

  let style = '' +
    '#test2 {' +
    '  background-color: green;' +
    '  color: purple;' +
    '}';

  let styleNode = addStyle(doc, style);
  doc.body.innerHTML = '<div id="test2"><div id="test1">Styled Node</div></div>';

  inspector.selection.setNode(doc.getElementById("test1"));
  inspector.once("inspector-updated", () => {
    let elementStyle = view._elementStyle;

    is(elementStyle.rules.length, 2, "Should have 2 rules.");

    let elementRule = elementStyle.rules[0];
    ok(!elementRule.inherited, "Element style attribute should not consider itself inherited.");

    let inheritRule = elementStyle.rules[1];
    is(inheritRule.selectorText, "#test2", "Inherited rule should be the one that includes inheritable properties.");
    ok(!!inheritRule.inherited, "Rule should consider itself inherited.");
    is(inheritRule.textProps.length, 1, "Should only display one inherited style");
    let inheritProp = inheritRule.textProps[0];
    is(inheritProp.name, "color", "color should have been inherited.");

    styleNode.parentNode.removeChild(styleNode);

    emptyInherit();
  }).then(null, console.error);
}

function emptyInherit()
{
  
  let style = '' +
    '#test2 {' +
    '  background-color: green;' +
    '}';

  let styleNode = addStyle(doc, style);
  doc.body.innerHTML = '<div id="test2"><div id="test1">Styled Node</div></div>';

  inspector.selection.setNode(doc.getElementById("test1"));
  inspector.once("inspector-updated", () => {
    let elementStyle = view._elementStyle;

    is(elementStyle.rules.length, 1, "Should have 1 rule.");

    let elementRule = elementStyle.rules[0];
    ok(!elementRule.inherited, "Element style attribute should not consider itself inherited.");

    styleNode.parentNode.removeChild(styleNode);

    elementStyleInherit();
  }).then(null, console.error);
}

function elementStyleInherit()
{
  doc.body.innerHTML = '<div id="test2" style="color: red"><div id="test1">Styled Node</div></div>';

  inspector.selection.setNode(doc.getElementById("test1"));
  inspector.once("inspector-updated", () => {
    let elementStyle = view._elementStyle;

    is(elementStyle.rules.length, 2, "Should have 2 rules.");

    let elementRule = elementStyle.rules[0];
    ok(!elementRule.inherited, "Element style attribute should not consider itself inherited.");

    let inheritRule = elementStyle.rules[1];
    is(inheritRule.domRule.type, ELEMENT_STYLE, "Inherited rule should be an element style, not a rule.");
    ok(!!inheritRule.inherited, "Rule should consider itself inherited.");
    is(inheritRule.textProps.length, 1, "Should only display one inherited style");
    let inheritProp = inheritRule.textProps[0];
    is(inheritProp.name, "color", "color should have been inherited.");

    finishTest();
  }).then(null, console.error);
}

function finishTest()
{
  doc = null;
  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, arguments.callee, true);
    doc = content.document;
    waitForFocus(() => openRuleView(simpleInherit), content);
  }, true);

  content.location = "data:text/html,basic style inspector tests";
}
