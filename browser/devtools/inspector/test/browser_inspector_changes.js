




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


  function getInspectorComputedProp(aName)
  {
    let computedview = inspector.sidebar.getWindowForTab("computedview").computedview.view;
    for each (let view in computedview.propertyViews) {
      if (view.name == aName) {
        return view;
      }
    }
    return null;
  }
  function getInspectorRuleProp(aName)
  {
    let ruleview = inspector.sidebar.getWindowForTab("ruleview").ruleview.view;
    let inlineStyles = ruleview._elementStyle.rules[0];

    for each (let prop in inlineStyles.textProps) {
      if (prop.name == aName) {
        return prop;
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

      
      inspector.once("computed-view-refreshed", computedStylePanelTests);

      inspector.selection.setNode(testDiv);
    });
  }

  function computedStylePanelTests()
  {
    let computedview = inspector.sidebar.getWindowForTab("computedview").computedview;
    ok(computedview, "Style Panel has a cssHtmlTree");

    let propView = getInspectorComputedProp("font-size");
    is(propView.value, "10px", "Style inspector should be showing the correct font size.");

    inspector.once("computed-view-refreshed", computedStylePanelAfterChange);

    testDiv.style.cssText = "font-size: 15px; color: red;";
  }

  function computedStylePanelAfterChange()
  {
    let propView = getInspectorComputedProp("font-size");
    is(propView.value, "15px", "Style inspector should be showing the new font size.");

    let propView = getInspectorComputedProp("color");
    is(propView.value, "#F00", "Style inspector should be showing the new color.");

    computedStylePanelNotActive();
  }

  function computedStylePanelNotActive()
  {
    
    inspector.sidebar.select("ruleview");
    testDiv.style.fontSize = "20px";
    testDiv.style.color = "blue";
    testDiv.style.textAlign = "center";
    inspector.once("computed-view-refreshed", computedStylePanelAfterSwitch);
    inspector.sidebar.select("computedview");
  }

  function computedStylePanelAfterSwitch()
  {
    let propView = getInspectorComputedProp("font-size");
    is(propView.value, "20px", "Style inspector should be showing the new font size.");

    let propView = getInspectorComputedProp("color");
    is(propView.value, "#00F", "Style inspector should be showing the new color.");

    let propView = getInspectorComputedProp("text-align");
    is(propView.value, "center", "Style inspector should be showing the new text align.");

    rulePanelTests();
  }

  function rulePanelTests()
  {
    inspector.sidebar.select("ruleview");
    let ruleview = inspector.sidebar.getWindowForTab("ruleview").ruleview;
    ok(ruleview, "Style Panel has a ruleview");

    let propView = getInspectorRuleProp("text-align");
    is(propView.value, "center", "Style inspector should be showing the new text align.");

    testDiv.style.textAlign = "right";
    testDiv.style.color = "lightgoldenrodyellow";
    testDiv.style.fontSize = "3em";
    testDiv.style.textTransform = "uppercase";


    inspector.once("rule-view-refreshed", rulePanelAfterChange);

  }

  function rulePanelAfterChange()
  {
    let propView = getInspectorRuleProp("text-align");
    is(propView.value, "right", "Style inspector should be showing the new text align.");

    let propView = getInspectorRuleProp("color");
    is(propView.value, "#FAFAD2", "Style inspector should be showing the new color.")

    let propView = getInspectorRuleProp("font-size");
    is(propView.value, "3em", "Style inspector should be showing the new font size.");

    let propView = getInspectorRuleProp("text-transform");
    is(propView.value, "uppercase", "Style inspector should be showing the new text transform.");

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
