







































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
  Services.obs.addObserver(runSelectionTests,
    INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.toggleInspectorUI();
}

function runSelectionTests()
{
  Services.obs.removeObserver(runSelectionTests,
    INSPECTOR_NOTIFICATIONS.OPENED, false);

  executeSoon(function() {
    Services.obs.addObserver(performTestComparisons,
      INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, false);
    EventUtils.synthesizeMouse(h1, 2, 2, {type: "mousemove"}, content);
  });
}

function performTestComparisons(evt)
{
  Services.obs.removeObserver(performTestComparisons,
    INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, false);

  InspectorUI.stopInspecting();
  ok(InspectorUI.highlighter.isHighlighting, "highlighter is highlighting");
  is(InspectorUI.highlighter.highlitNode, h1, "highlighter matches selection")
  is(InspectorUI.selection, h1, "selection matches node");
  is(InspectorUI.selection, InspectorUI.highlighter.highlitNode, "selection matches highlighter");

  doc = h1 = null;
  executeSoon(finishUp);
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

