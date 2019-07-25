








































let doc;
let div;
let iframe;

function createDocument()
{
  doc.title = "Inspector scrolling Tests";

  iframe = doc.createElement("iframe");

  iframe.addEventListener("load", function () {
    iframe.removeEventListener("load", arguments.callee, false);

    div = iframe.contentDocument.createElement("div");
    div.textContent = "big div";
    div.setAttribute("style", "height:500px; width:500px; border:1px solid gray;");
    iframe.contentDocument.body.appendChild(div);
    toggleInspector();
  }, false);

  iframe.src = "data:text/html,foo bar";
  doc.body.appendChild(iframe);
}

function toggleInspector()
{
  Services.obs.addObserver(inspectNode, "inspector-opened", false);
  InspectorUI.toggleInspectorUI();
}

function inspectNode()
{
  Services.obs.removeObserver(inspectNode, "inspector-opened", false);
  document.addEventListener("popupshown", performScrollingTest, false);

  InspectorUI.inspectNode(div)
}

function performScrollingTest(aEvent)
{
  if (aEvent.target.id != "highlighter-panel") {
    return true;
  }

  document.removeEventListener("popupshown", performScrollingTest, false);

  EventUtils.synthesizeMouseScroll(aEvent.target, 10, 10,
    {axis:"vertical", delta:50, type:"MozMousePixelScroll"}, window);

  is(iframe.contentDocument.body.scrollTop, 50, "inspected iframe scrolled");

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

  content.location = "data:text/html,mouse scrolling test for inspector";
}
