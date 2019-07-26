




let doc;
let testDiv;

function test() {
  let inspector;

  function createDocument()
  {
    doc.body.innerHTML = '<div id="testdiv">Test div!</div>';
    doc.title = "Inspector Change Test";
    openInspector(runInspectorTests);
  }


  function getInspectorProp(aName)
  {
    let computedview = inspector.sidebar.getWindowForTab("computedview").computedview.view;
    for each (let view in computedview.propertyViews) {
      if (view.name == aName) {
        return view;
      }
    }
    return null;
  }

  function runInspectorTests(aInspector)
  {
    inspector = aInspector;
    inspector.sidebar.once("computedview-ready", function() {
      info("Computed View ready");
      inspector.sidebar.select("computedview");

      testDiv = doc.getElementById("testdiv");

      testDiv.style.fontSize = "10px";

      
      inspector.once("computed-view-refreshed", stylePanelTests);

      inspector.selection.setNode(testDiv);
    });
  }

  function stylePanelTests()
  {
    let computedview = inspector.sidebar.getWindowForTab("computedview").computedview;
    ok(computedview, "Style Panel has a cssHtmlTree");

    let propView = getInspectorProp("font-size");
    is(propView.value, "10px", "Style inspector should be showing the correct font size.");

    inspector.once("computed-view-refreshed", stylePanelAfterChange);

    testDiv.style.fontSize = "15px";
    inspector.emit("layout-change");
  }

  function stylePanelAfterChange()
  {
    let propView = getInspectorProp("font-size");
    is(propView.value, "15px", "Style inspector should be showing the new font size.");

    stylePanelNotActive();
  }

  function stylePanelNotActive()
  {
    
    inspector.sidebar.select("ruleview");

    executeSoon(function() {
      inspector.once("computed-view-refreshed", stylePanelAfterSwitch);
      testDiv.style.fontSize = "20px";
      inspector.sidebar.select("computedview");
    });
  }

  function stylePanelAfterSwitch()
  {
    let propView = getInspectorProp("font-size");
    is(propView.value, "20px", "Style inspector should be showing the newest font size.");

    finishTest();
  }

  function finishTest()
  {
    gBrowser.removeCurrentTab();
    finish();
  }

  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    doc = content.document;
    waitForFocus(createDocument, content);
  }, true);

  content.location = "data:text/html,basic tests for inspector";
}
