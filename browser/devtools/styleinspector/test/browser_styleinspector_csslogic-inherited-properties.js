



"use strict";



let test = asyncTest(function*() {
  yield addTab("data:text/html;charset=utf-8,selector text test, bug 692400");

  content.document.body.innerHTML = '<div style="margin-left:10px; font-size: 5px"><div id="innerdiv">Inner div</div></div>';
  content.document.title = "Style Inspector Inheritance Test";

  let cssLogic = new CssLogic();
  cssLogic.highlight(content.document.getElementById("innerdiv"));

  let marginProp = cssLogic.getPropertyInfo("margin-left");
  is(marginProp.matchedRuleCount, 0, "margin-left should not be included in matched selectors.");

  let fontSizeProp = cssLogic.getPropertyInfo("font-size");
  is(fontSizeProp.matchedRuleCount, 1, "font-size should be included in matched selectors.");
});
