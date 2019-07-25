







































let doc;
let testGen;
let newProperty;

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
  Services.obs.addObserver(runDOMTests, "inspector-opened", false);
  InspectorUI.openInspectorUI();
}

function nodeGenerator()
{
  let body = doc.body;
  newProperty = "rand" + Date.now();
  body[newProperty] = Math.round(Math.random() * 100);
  InspectorUI.inspectNode(body);
  yield;

  let h1 = doc.querySelector("h1");
  newProperty = "rand2" + Date.now();
  h1[newProperty] = "test" + Math.random();
  InspectorUI.inspectNode(h1);
  yield;

  let first = doc.getElementById("first");
  newProperty = "rand3" + Date.now();
  first[newProperty] = null;
  InspectorUI.inspectNode(first);
  yield;

  let closing = doc.getElementById("closing");
  newProperty = "bazbaz" + Date.now();
  closing[newProperty] = false;
  InspectorUI.inspectNode(closing);
  yield;
}

function runDOMTests()
{
  InspectorUI._log("runDOMtests");
  Services.obs.removeObserver(runDOMTests, "inspector-opened", false);
  document.addEventListener("popupshown", performTestComparisons, false);
  InspectorUI.stopInspecting();
  testGen = nodeGenerator();
  testGen.next();
}

function findInDOMPanel(aString)
{
  let treeView = InspectorUI.domTreeView;
  let row;

  for (let i = 0, n = treeView.rowCount; i < n; i++) {
    row = treeView.getCellText(i, 0);
    if (row && row.indexOf(aString) != -1) {
      return true;
    }
  }

  return false;
}

function performTestComparisons(evt)
{
  InspectorUI._log("performTestComparisons");
  if (evt.target.id != "highlighter-panel")
    return true;

  let selection = InspectorUI.selection;

  ok(selection, "selection");
  ok(InspectorUI.isDOMPanelOpen, "DOM panel is open?");
  ok(InspectorUI.highlighter.isHighlighting, "panel is highlighting");

  let value = selection[newProperty];
  if (typeof value == "string") {
    value = '"' + value + '"';
  }

  ok(findInDOMPanel(newProperty + ': ' + value),
    "domPanel shows the correct value for " + newProperty);

  ok(findInDOMPanel('tagName: "' + selection.tagName + '"'),
    "domPanel shows the correct tagName");

  if (selection.id) {
    ok(findInDOMPanel('id: "' + selection.id + '"'),
      "domPanel shows the correct id");
  }

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

