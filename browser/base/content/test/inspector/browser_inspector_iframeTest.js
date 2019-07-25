







































let doc;
let div1;
let div2;
let iframe1;
let iframe2;
let highlighterFrame;

function createDocument()
{
  doc.title = "Inspector iframe Tests";

  iframe1 = doc.createElement('iframe');

  iframe1.addEventListener("load", function () {
    iframe1.removeEventListener("load", arguments.callee, false);

    div1 = iframe1.contentDocument.createElement('div');
    div1.textContent = 'little div';
    iframe1.contentDocument.body.appendChild(div1);

    iframe2 = iframe1.contentDocument.createElement('iframe');

    iframe2.addEventListener('load', function () {
      iframe2.removeEventListener("load", arguments.callee, false);

      div2 = iframe2.contentDocument.createElement('div');
      div2.textContent = 'nested div';
      iframe2.contentDocument.body.appendChild(div2);

      setupIframeTests();
    }, false);

    iframe2.src = 'data:text/html,nested iframe';
    iframe1.contentDocument.body.appendChild(iframe2);
  }, false);

  iframe1.src = 'data:text/html,little iframe';
  doc.body.appendChild(iframe1);
}

function moveMouseOver(aElement)
{
  EventUtils.synthesizeMouse(aElement, 2, 2, {type: "mousemove"},
    aElement.ownerDocument.defaultView);
}

function setupIframeTests()
{
  Services.obs.addObserver(runIframeTests,
    INSPECTOR_NOTIFICATIONS.OPENED, false);
  InspectorUI.openInspectorUI();
}

function runIframeTests()
{
  Services.obs.removeObserver(runIframeTests,
    INSPECTOR_NOTIFICATIONS.OPENED, false);

  Services.obs.addObserver(performTestComparisons1,
    INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, false);

  highlighterFrame = InspectorUI.highlighter.iframe;
  executeSoon(moveMouseOver.bind(this, div1));
}

function performTestComparisons1()
{
  Services.obs.removeObserver(performTestComparisons1,
    INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, false);
  Services.obs.addObserver(performTestComparisons2,
    INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, false);

  is(InspectorUI.selection, div1, "selection matches div1 node");
  is(InspectorUI.highlighter.highlitNode, div1, "highlighter matches selection");

  executeSoon(moveMouseOver.bind(this, div2));
}

function performTestComparisons2()
{
  Services.obs.removeObserver(performTestComparisons2,
    INSPECTOR_NOTIFICATIONS.HIGHLIGHTING, false);

  is(InspectorUI.selection, div2, "selection matches div2 node");
  is(InspectorUI.highlighter.highlitNode, div2, "highlighter matches selection");

  finishUp();
}

function finishUp() {
  InspectorUI.closeInspectorUI(true);

  doc = div1 = div2 = iframe1 = iframe2 = highlighterFrame = null;
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

  content.location = "data:text/html,iframe tests for inspector";
}

