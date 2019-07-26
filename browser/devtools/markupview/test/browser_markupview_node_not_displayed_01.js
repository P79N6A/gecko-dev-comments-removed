



"use strict";









const TEST_URL = TEST_URL_ROOT + "doc_markup_not_displayed.html";
const TEST_DATA = [
  {selector: "#normal-div", isDisplayed: true},
  {selector: "head", isDisplayed: false},
  {selector: "#display-none", isDisplayed: false},
  {selector: "#hidden-true", isDisplayed: false},
  {selector: "#visibility-hidden", isDisplayed: true}
];

let test = asyncTest(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  for (let {selector, isDisplayed} of TEST_DATA) {
    info("Getting node " + selector);
    let container = getContainerForRawNode(selector, inspector);
    is(!container.elt.classList.contains("not-displayed"), isDisplayed,
      "The container for " + selector + " is marked as displayed " + isDisplayed);
  }
});
