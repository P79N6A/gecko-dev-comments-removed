





const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test" +
                 "/test-bug-782653-css-errors.html";

let nodes, hud, StyleEditorUI;

function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testViewSource);
  }, true);
}

function testViewSource(aHud)
{
  hud = aHud;

  registerCleanupFunction(function() {
    nodes = hud = StyleEditorUI = null;
  });

  let selector = ".webconsole-msg-cssparser .webconsole-location";

  waitForSuccess({
    name: "find the location node",
    validatorFn: function()
    {
      return hud.outputNode.querySelector(selector);
    },
    successFn: function()
    {
      nodes = hud.outputNode.querySelectorAll(selector);
      is(nodes.length, 2, "correct number of css messages");

      let target = TargetFactory.forTab(gBrowser.selectedTab);
      let toolbox = gDevTools.getToolbox(target);
      toolbox.once("styleeditor-selected", (event, panel) => {
        StyleEditorUI = panel.UI;

        let count = 0;
        StyleEditorUI.on("editor-added", function() {
          if (++count == 2) {
            onStyleEditorReady(panel);
          }
        });
      });

      EventUtils.sendMouseEvent({ type: "click" }, nodes[0]);
    },
    failureFn: finishTest,
  });
}

function onStyleEditorReady(aPanel)
{
  let win = aPanel.panelWindow;
  ok(win, "Style Editor Window is defined");
  ok(StyleEditorUI, "Style Editor UI is defined");

  waitForFocus(function() {
    info("style editor window focused");

    let href = nodes[0].getAttribute("title");
    ok(href.contains("test-bug-782653-css-errors-1.css"), "got first stylesheet href")
    let line = nodes[0].sourceLine;
    is(line, 8, "found source line");

    checkStyleEditorForSheetAndLine(href, line - 1, function() {
      info("first check done");

      let target = TargetFactory.forTab(gBrowser.selectedTab);
      let toolbox = gDevTools.getToolbox(target);

      let href = nodes[1].getAttribute("title");
      ok(href.contains("test-bug-782653-css-errors-2.css"), "got second stylesheet href")
      let line = nodes[1].sourceLine;
      is(line, 7, "found source line");

      toolbox.selectTool("webconsole").then(function() {
        info("webconsole selected");

        toolbox.once("styleeditor-selected", function(aEvent) {
          info(aEvent + " event fired");

          checkStyleEditorForSheetAndLine(href, line - 1, function() {
            info("second check done");
            finishTest();
          });
        });

        EventUtils.sendMouseEvent({ type: "click" }, nodes[1]);
      });
    });
  }, win);
}

function checkStyleEditorForSheetAndLine(aHref, aLine, aCallback)
{
  let foundEditor = null;
  waitForSuccess({
    name: "style editor for stylesheet",
    validatorFn: function()
    {
      for (let editor of StyleEditorUI.editors) {
        if (editor.styleSheet.href == aHref) {
          foundEditor = editor;
          return true;
        }
      }
      return false;
    },
    successFn: function()
    {
      performLineCheck(foundEditor, aLine, aCallback);
    },
    failureFn: finishTest,
  });
}

function performLineCheck(aEditor, aLine, aCallback)
{
  function checkForCorrectState()
  {
    is(aEditor.sourceEditor.getCaretPosition().line, aLine,
       "correct line is selected");
    is(StyleEditorUI.selectedStyleSheetIndex, aEditor.styleSheet.styleSheetIndex,
       "correct stylesheet is selected in the editor");

    aCallback && executeSoon(aCallback);
  }

  waitForSuccess({
    name: "source editor load",
    validatorFn: function()
    {
      return aEditor.sourceEditor;
    },
    successFn: checkForCorrectState,
    failureFn: function() {
      info("selectedStyleSheetIndex " + StyleEditorUI.selectedStyleSheetIndex
           + " expected " + aEditor.styleSheet.styleSheetIndex);
      finishTest();
    },
  });
}
