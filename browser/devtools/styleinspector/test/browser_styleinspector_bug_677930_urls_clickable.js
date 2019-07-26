



"use strict";



const TEST_URI = TEST_URL_ROOT +
                 "browser_styleinspector_bug_677930_urls_clickable.html";
const TEST_IMAGE = TEST_URL_ROOT + "test-image.png";
const BASE_64_URL = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAAHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU5ErkJggg==";

let test = asyncTest(function*() {
  yield addTab(TEST_URI);
  let {toolbox, inspector, view} = yield openRuleView();
  yield selectNodes(inspector, view);
});

function* selectNodes(inspector, ruleView) {
  let relative1 = ".relative1";
  let relative2 = ".relative2";
  let absolute = ".absolute";
  let inline = ".inline";
  let base64 = ".base64";
  let noimage = ".noimage";
  let inlineresolved = ".inline-resolved";

  yield selectNode(relative1, inspector);
  let relativeLink = ruleView.doc.querySelector(".ruleview-propertycontainer a");
  ok(relativeLink, "Link exists for relative1 node");
  is(relativeLink.getAttribute("href"), TEST_IMAGE, "href matches");

  yield selectNode(relative2, inspector);
  let relativeLink = ruleView.doc.querySelector(".ruleview-propertycontainer a");
  ok(relativeLink, "Link exists for relative2 node");
  is(relativeLink.getAttribute("href"), TEST_IMAGE, "href matches");

  yield selectNode(absolute, inspector);
  let absoluteLink = ruleView.doc.querySelector(".ruleview-propertycontainer a");
  ok(absoluteLink, "Link exists for absolute node");
  is(absoluteLink.getAttribute("href"), TEST_IMAGE, "href matches");

  yield selectNode(inline, inspector);
  let inlineLink = ruleView.doc.querySelector(".ruleview-propertycontainer a");
  ok(inlineLink, "Link exists for inline node");
  is(inlineLink.getAttribute("href"), TEST_IMAGE, "href matches");

  yield selectNode(base64, inspector);
  let base64Link = ruleView.doc.querySelector(".ruleview-propertycontainer a");
  ok(base64Link, "Link exists for base64 node");
  is(base64Link.getAttribute("href"), BASE_64_URL, "href matches");

  yield selectNode(inlineresolved, inspector);
  let inlineResolvedLink = ruleView.doc.querySelector(".ruleview-propertycontainer a");
  ok(inlineResolvedLink, "Link exists for style tag node");
  is(inlineResolvedLink.getAttribute("href"), TEST_IMAGE, "href matches");

  yield selectNode(noimage, inspector);
  let noimageLink = ruleView.doc.querySelector(".ruleview-propertycontainer a");
  ok(!noimageLink, "There is no link for the node with no background image");
}
