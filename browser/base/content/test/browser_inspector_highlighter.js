






































let doc;
let h1;

function createDocument()
{
  let div = doc.createElement("div");
  let h1 = doc.createElement("h1");
  let p1 = doc.createElement("p");
  let p2 = doc.createElement("p");
  let div2 = doc.createElement("div");
  let p3 = doc.createElement("p");
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
  p3.textContent = "Lorem ipsum dolor sit amet, consectetur adipisicing " +
    "elit, sed do eiusmod tempor incididunt ut labore et dolore magna " +
    "aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco " +
    "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure " +
    "dolor in reprehenderit in voluptate velit esse cillum dolore eu " +
    "fugiat nulla pariatur. Excepteur sint occaecat cupidatat non " +
    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
  div.appendChild(h1);
  div.appendChild(p1);
  div.appendChild(p2);
  div2.appendChild(p3);
  doc.body.appendChild(div);
  doc.body.appendChild(div2);
  setupHighlighterTests();
}

function setupHighlighterTests()
{
  h1 = doc.querySelectorAll("h1")[0];
  ok(h1, "we have the header node");
  Services.obs.addObserver(runSelectionTests, "inspector-opened", false);
  InspectorUI.toggleInspectorUI();
}

function runSelectionTests()
{
  Services.obs.removeObserver(runSelectionTests, "inspector-opened", false);
  document.addEventListener("popupshown", performTestComparisons, false);
  EventUtils.synthesizeMouse(h1, 2, 2, {type: "mousemove"}, content);
}

function performTestComparisons(evt)
{
  if (evt.target.id != "highlighter-panel")
    return true;
  document.removeEventListener("popupshown", performTestComparisons, false);
  is(h1, InspectorUI.selection, "selection matches node");
  ok(InspectorUI.highlighter.isHighlighting, "panel is highlighting");
  is(InspectorUI.highlighter.highlitNode, h1, "highlighter matches selection");

  Services.obs.addObserver(finishUp, "inspector-closed", false);
  InspectorUI.closeInspectorUI();
}

function finishUp() {
  Services.obs.removeObserver(finishUp, "inspector-closed", false);

  ok(!InspectorUI.highlighter, "panel is not highlighting");
  doc = h1 = null;
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

