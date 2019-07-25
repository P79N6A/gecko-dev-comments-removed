






































let doc;
let spans;
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
    'style list-items in the box at right. If you are reading this, ' +
    'you should go do something else instead. Maybe read a book. Or better ' +
    'yet, write some test-cases for another bit of code. ' +
    '<span style="{font-style: italic}">Maybe more inspector test-cases!</span></p>\n' +
    '<p id="closing">end transmission</p>\n' +
    '</div>';
  doc.title = "Inspector Style Test";
  setupStyleTests();
}

function setupStyleTests()
{
  spans = doc.querySelectorAll("span");
  ok(spans, "captain, we have the spans");
  Services.obs.addObserver(runStyleTests, "inspector-opened", false);
  InspectorUI.openInspectorUI();
}

function spanGenerator()
{
  for (var i = 0; i < spans.length; ++i) {
    InspectorUI.inspectNode(spans[i]);
    yield;
  }
}

function runStyleTests()
{
  Services.obs.removeObserver(runStyleTests, "inspector-opened", false);
  document.addEventListener("popupshown", performTestComparisons, false);
  InspectorUI.stopInspecting();
  testGen = spanGenerator();
  testGen.next();
}

function performTestComparisons(evt)
{
  if (evt.target.id != "highlighter-panel")
    return true;

  ok(InspectorUI.selection, "selection");
  ok(InspectorUI.isStylePanelOpen, "style panel is open?");
  ok(InspectorUI.highlighter.isHighlighting, "panel is highlighting");
  ok(InspectorUI.styleBox.itemCount > 0, "styleBox has items");

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

