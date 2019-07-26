



let tempScope = {}
Cu.import("resource:///modules/devtools/CssRuleView.jsm", tempScope);
let CssRuleView = tempScope.CssRuleView;
let _ElementStyle = tempScope._ElementStyle;

let doc;
let ruleDialog;
let ruleView;
let testElement;

function startTest()
{
  let style = '' +
    '#testid {' +
    '  background-color: blue;' +
    '} ' +
    '.testclass {' +
    '  background-color: green;' +
    '}';

  let styleNode = addStyle(doc, style);
  doc.body.innerHTML = '<div id="testid" class="testclass">Styled Node</div>';
  testElement = doc.getElementById("testid");

  let elementStyle = 'margin-top: 1px; padding-top: 5px;'
  testElement.setAttribute("style", elementStyle);

  ruleDialog = openDialog("chrome://browser/content/devtools/cssruleview.xhtml",
                          "cssruleviewtest",
                          "width=200,height=350");
  ruleDialog.addEventListener("load", function onLoad(evt) {
    ruleDialog.removeEventListener("load", onLoad);
    let doc = ruleDialog.document;
    ruleView = new CssRuleView(doc);
    doc.documentElement.appendChild(ruleView.element);
    ruleView.highlight(testElement);
    waitForFocus(testRuleChanges, ruleDialog);
  }, true);
}

function testRuleChanges()
{
  let selectors = ruleView.doc.querySelectorAll(".ruleview-selector");
  is(selectors.length, 3, "Three rules visible.");
  is(selectors[0].textContent.indexOf("element"), 0, "First item is inline style.");
  is(selectors[1].textContent.indexOf("#testid"), 0, "Second item is id rule.");
  is(selectors[2].textContent.indexOf(".testclass"), 0, "Third item is class rule.");

  
  testElement.setAttribute("id", "differentid");
  ruleView.nodeChanged();

  let selectors = ruleView.doc.querySelectorAll(".ruleview-selector");
  is(selectors.length, 2, "Three rules visible.");
  is(selectors[0].textContent.indexOf("element"), 0, "First item is inline style.");
  is(selectors[1].textContent.indexOf(".testclass"), 0, "Second item is class rule.");

  testElement.setAttribute("id", "testid");
  ruleView.nodeChanged();

  
  let selectors = ruleView.doc.querySelectorAll(".ruleview-selector");
  is(selectors.length, 3, "Three rules visible.");
  is(selectors[0].textContent.indexOf("element"), 0, "First item is inline style.");
  is(selectors[1].textContent.indexOf("#testid"), 0, "Second item is id rule.");
  is(selectors[2].textContent.indexOf(".testclass"), 0, "Third item is class rule.");

  testPropertyChanges();
}

function validateTextProp(aProp, aEnabled, aName, aValue, aDesc)
{
  is(aProp.enabled, aEnabled, aDesc + ": enabled.");
  is(aProp.name, aName, aDesc + ": name.");
  is(aProp.value, aValue, aDesc + ": value.");

  is(aProp.editor.enable.hasAttribute("checked"), aEnabled, aDesc + ": enabled checkbox.");
  is(aProp.editor.nameSpan.textContent, aName, aDesc + ": name span.");
  is(aProp.editor.valueSpan.textContent, aValue, aDesc + ": value span.");
}

function testPropertyChanges()
{
  
  let ruleEditor = ruleView._elementStyle.rules[0].editor;
  ruleEditor.addProperty("margin-top", "5px", "");

  let rule = ruleView._elementStyle.rules[0];

  
  testElement.setAttribute("style", "margin-top: 1px; padding-top: 5px");
  ruleView.nodeChanged();
  is(rule.editor.element.querySelectorAll(".ruleview-property").length, 3, "Correct number of properties");
  validateTextProp(rule.textProps[0], true, "margin-top", "1px", "First margin property re-enabled");
  validateTextProp(rule.textProps[2], false, "margin-top", "5px", "Second margin property disabled");

  
  testElement.setAttribute("style", "margin-top: 5px; padding-top: 5px;");
  ruleView.nodeChanged();
  is(rule.editor.element.querySelectorAll(".ruleview-property").length, 3, "Correct number of properties");
  validateTextProp(rule.textProps[0], false, "margin-top", "1px", "First margin property re-enabled");
  validateTextProp(rule.textProps[2], true, "margin-top", "5px", "Second margin property disabled");

  
  
  testElement.setAttribute("style", "margin-top: 15px; padding-top: 5px;");
  ruleView.nodeChanged();
  is(rule.editor.element.querySelectorAll(".ruleview-property").length, 3, "Correct number of properties");
  validateTextProp(rule.textProps[0], false, "margin-top", "1px", "First margin property re-enabled");
  validateTextProp(rule.textProps[2], true, "margin-top", "15px", "Second margin property disabled");

  
  testElement.setAttribute("style", "margin-top: 5px;");
  ruleView.nodeChanged();
  is(rule.editor.element.querySelectorAll(".ruleview-property").length, 3, "Correct number of properties");
  validateTextProp(rule.textProps[1], false, "padding-top", "5px", "Padding property disabled");

  
  testElement.setAttribute("style", "margin-top: 5px; padding-top: 25px");
  ruleView.nodeChanged();
  is(rule.editor.element.querySelectorAll(".ruleview-property").length, 3, "Correct number of properties");
  validateTextProp(rule.textProps[1], true, "padding-top", "25px", "Padding property enabled");

  
  testElement.setAttribute("style", "margin-top: 5px; padding-top: 25px; padding-left: 20px;");
  ruleView.nodeChanged();
  is(rule.editor.element.querySelectorAll(".ruleview-property").length, 4, "Added a property");
  validateTextProp(rule.textProps[3], true, "padding-left", "20px", "Padding property enabled");

  finishTest();
}

function finishTest()
{
  ruleView.clear();
  ruleDialog.close();
  ruleDialog = ruleView = null;
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
    waitForFocus(startTest, content);
  }, true);

  content.location = "data:text/html,basic style inspector tests";
}
