







































let doc;
let div1;
let div2;
let iframe1;
let iframe2;

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

function setupIframeTests()
{
  document.addEventListener("popupshown", runIframeTests, false);
  InspectorUI.toggleInspectorUI();
}

function runIframeTests(evt)
{
  if (evt.target.id != "inspector-panel")
    return true;

  document.removeEventListener("popupshown", runIframeTests, false);
  document.addEventListener("popupshown", performTestComparisons1, false);

  EventUtils.synthesizeMouse(div1, 2, 2, {type: "mousemove"},
    iframe1.contentWindow);
}

function performTestComparisons1(evt)
{
  if (evt.target.id != "highlighter-panel")
    return true;

  document.removeEventListener("popupshown", performTestComparisons1, false);

  is(InspectorUI.treeView.selectedNode, div1, "selection matches div1 node");
  is(InspectorUI.highlighter.highlitNode, div1, "highlighter matches selection");

  document.addEventListener("popupshown", performTestComparisons2, false);

  EventUtils.synthesizeMouse(div2, 2, 2, {type: "mousemove"},
    iframe2.contentWindow);
}

function performTestComparisons2(evt)
{
  if (evt.target.id != "highlighter-panel")
    return true;

  document.removeEventListener("popupshown", performTestComparisons2, false);

  is(InspectorUI.treeView.selectedNode, div2, "selection matches div2 node");
  is(InspectorUI.highlighter.highlitNode, div2, "highlighter matches selection");

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
  
  content.location = "data:text/html,iframe tests for inspector";
}

