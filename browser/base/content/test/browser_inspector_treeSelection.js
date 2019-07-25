






































let doc;
let h1;

function createDocument()
{
  let div = doc.createElement("div");
  let h1 = doc.createElement("h1");
  let p1 = doc.createElement("p");
  let p2 = doc.createElement("p");
  doc.title = "Inspector Tree Selection Test";
  h1.textContent = "Inspector Tree Selection Test";
  p1.textContent = "This is some example text";
  p2.textContent = "Lorem ipsum dolor sit amet, consectetur adipisicing " +
    "elit, sed do eiusmod tempor incididunt ut labore et dolore magna " +
    "aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco " +
    "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure " +
    "dolor in reprehenderit in voluptate velit esse cillum dolore eu " +
    "fugiat nulla pariatur. Excepteur sint occaecat cupidatat non " +
    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
  div.appendChild(h1);
  div.appendChild(p1);
  div.appendChild(p2);
  
  doc.body.appendChild(div);
  setupSelectionTests();
}

function setupSelectionTests()
{
  h1 = doc.querySelectorAll("h1")[0];
  ok(h1, "we have the header node");
  document.addEventListener("popupshown", runSelectionTests, false);
  InspectorUI.openInspectorUI();
}

function runSelectionTests(evt)
{
  if (evt.target.id != "inspector-panel")
    return true;
  document.removeEventListener("popupshown", runSelectionTests, false);
  InspectorUI.stopInspecting();
  document.addEventListener("popupshown", performTestComparisons, false);
  InspectorUI.treeView.selectedNode = h1;
}

function performTestComparisons(evt)
{
  if (evt.target.id != "highlighter-panel")
    return true;
  document.removeEventListener("popupshown", performTestComparisons, false);
  is(h1, InspectorUI.treeView.selectedNode, "selection matches node");
  ok(InspectorUI.highlighter.isHighlighting, "panel is highlighting");
  is(h1, InspectorUI.highlighter.highlitNode, "highlighter highlighting correct node");
  finishUp();
}

function finishUp() {
  InspectorUI.closeInspectorUI();
  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    doc = content.document;
    waitForFocus(createDocument, content);
  }, true);
  
  content.location = "data:text/html,basic tests for inspector";
}

