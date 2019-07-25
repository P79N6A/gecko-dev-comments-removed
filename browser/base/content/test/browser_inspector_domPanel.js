






































let doc;
let testGen;

function createDocument()
{
  doc.body.innerHTML = '<div id="first" style="{ margin: 10em; ' +
    'font-size: 14pt; font-family: helvetica, sans-serif; color: #AAA}">\n' +
    '<h1>Some header text</h1>\n' +
    '<p id="salutation" style="{font-size: 12pt}">hi.</p>\n' +
    '<p id="body" style="{font-size: 12pt}">I am a test-case. This text exists ' +
    'solely to provide some things to <span style="{color: yellow}">' +
    'highlight</span> and <span style="{font-weight: bold}">count</span> ' +
    'DOM list-items in the box at right. If you are reading this, ' +
    'you should go do something else instead. Maybe read a book. Or better ' +
    'yet, write some test-cases for another bit of code. ' +
    '<span style="{font-style: italic}">Maybe more inspector test-cases!</span></p>\n' +
    '<p id="closing">end transmission</p>\n' +
    '</div>';
  doc.title = "Inspector DOM Test";
  document.addEventListener("popupshown", runDOMTests, false);
  InspectorUI.openInspectorUI();
}

function nodeGenerator()
{
  let body = doc.body;
  InspectorUI.inspectNode(body);
  yield;
  let h1 = doc.querySelector("h1");
  InspectorUI.inspectNode(h1);
  yield;
  let first = doc.getElementById("first");
  InspectorUI.inspectNode(first);
  yield;
  let closing = doc.getElementById("#closing");
  InspectorUI.inspectNode(closing);
  yield;
}

function runDOMTests(evt)
{
  if (evt.target.id != "inspector-dom-panel")
    return true;
  InspectorUI._log("runDOMtests");
  document.removeEventListener("popupshown", runDOMTests, false);
  InspectorUI.stopInspecting();
  document.addEventListener("popupshown", performTestComparisons, false);
  testGen = nodeGenerator();
  testGen.next();
}

function performTestComparisons(evt)
{
  InspectorUI._log("performTestComparisons");
  if (evt.target.id != "highlighter-panel")
    return true;

  ok(InspectorUI.treeView.selectedNode, "selection");
  ok(InspectorUI.isDOMPanelOpen, "DOM panel is open?");
  ok(InspectorUI.highlighter.isHighlighting, "panel is highlighting");
  ok(InspectorUI.domTreeView.rowCount > 0, "domBox has items");

  try {
    testGen.next();
  } catch(StopIteration) {
    document.removeEventListener("popupshown", performTestComparisons, false);
    finishUp();
  }
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

