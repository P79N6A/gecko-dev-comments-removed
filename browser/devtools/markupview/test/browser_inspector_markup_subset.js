






Services.prefs.setIntPref("devtools.markup.pagesize", 5);

const TEST_URL = TEST_URL_ROOT + "browser_inspector_markup_subset.html";
const TEST_DATA = [{
  desc: "Select the last item",
  selector: "#z",
  expected: "*more*vwxyz"
}, {
  desc: "Select the first item",
  selector: "#a",
  expected: "abcde*more*"
}, {
  desc: "Select the last item",
  selector: "#z",
  expected: "*more*vwxyz"
}, {
  desc: "Select an already-visible item",
  selector: "#v",
  
  
  expected: "*more*vwxyz"
}, {
  desc: "Verify childrenDirty reloads the page",
  selector: "#w",
  forceReload: true,
  
  
  expected: "*more*uvwxy*more*"
}];

let test = asyncTest(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  info("Start iterating through the test data");
  for (let step of TEST_DATA) {
    info("Start test: " + step.desc);

    if (step.forceReload) {
      forceReload(inspector);
    }
    info("Selecting the node that corresponds to " + step.selector);
    yield selectNode(step.selector, inspector);

    info("Checking that the right nodes are shwon");
    assertChildren(step.expected, inspector);
  }

  info("Checking that clicking the more button loads everything");
  clickShowMoreNodes(inspector);
  yield inspector.markup._waitForChildren();
  assertChildren("abcdefghijklmnopqrstuvwxyz", inspector);
});

function assertChildren(expected, inspector) {
  let container = getContainerForRawNode("body", inspector);
  let found = "";
  for (let child of container.children.children) {
    if (child.classList.contains("more-nodes")) {
      found += "*more*";
    } else {
      found += child.container.node.getAttribute("id");
    }
  }
  is(found, expected, "Got the expected children.");
}

function forceReload(inspector) {
  let container = getContainerForRawNode("body", inspector);
  container.childrenDirty = true;
}

function clickShowMoreNodes(inspector) {
  let container = getContainerForRawNode("body", inspector);
  let button = container.elt.querySelector("button");
  let win = button.ownerDocument.defaultView;
  EventUtils.sendMouseEvent({type: "click"}, button, win);
}
