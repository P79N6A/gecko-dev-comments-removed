





const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test" +
                 "/test-bug-782653-css-errors.html";

let nodes;

let styleEditorWin;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testViewSource);
  }, true);
}

function testViewSource(hud) {

  waitForSuccess({
    name: "find the location node",
    validatorFn: function()
    {
      return hud.outputNode.querySelector(".webconsole-location");
    },
    successFn: function()
    {
      nodes = hud.outputNode.querySelectorAll(".webconsole-location");

      Services.ww.registerNotification(observer);

      EventUtils.sendMouseEvent({ type: "click" }, nodes[0]);
    },
    failureFn: finishTest,
  });
}

function checkStyleEditorForSheetAndLine(aStyleSheetIndex, aLine, aCallback) {

  function doCheck(aEditor) {
    if (aEditor.styleSheetIndex != aStyleSheetIndex) {
      ok(false, "Correct Style Sheet was not selected.");
      if (aCallback) {
        executeSoon(aCallback);
      }
      return;
    }

    ok(true, "Correct Style Sheet is selected in the editor");

    
    if (aEditor.sourceEditor) {
      ok(true, "Editor is already loaded, check the current line of caret");
      executeSoon(function() {
        ok(true, "Execute soon occured");
        ok(aEditor.sourceEditor != null, "sourceeditor not null");
        ok(aEditor.sourceEditor.getCaretPosition() != null, "position not null");
        ok(aEditor.sourceEditor.getCaretPosition().line != null, "line not null");
        is(aEditor.sourceEditor.getCaretPosition().line, aLine,
           "Correct line is selected");
        if (aCallback) {
          aCallback();
        }
      });
      return;
    }

    ok(true, "Editor is not loaded, waiting for it.");
    ok(aEditor, "aEditor is defined.");
    
    aEditor.addActionListener({
      onAttach: function onAttach() {
        ok(true, "on attach happened");
        aEditor.removeActionListener(this);
        ok(true, "this removed");
        executeSoon(function() {
          ok(true, "execute soon");
          ok(aEditor.sourceEditor != null, "sourceeditor not null");
          ok(aEditor.sourceEditor.getCaretPosition() != null, "position not null");
          ok(aEditor.sourceEditor.getCaretPosition().line != null, "line not null");
          is(aEditor.sourceEditor.getCaretPosition().line, aLine,
             "Correct line is selected");
          if (aCallback) {
            aCallback();
          }
        });
      }
    });
  }

  let SEC = styleEditorWin.styleEditorChrome;
  ok(SEC, "Syle Editor Chrome is defined properly while calling for [" +
          aStyleSheetIndex + ", " + aLine + "]");

  
  if (!SEC.editors.length) {
    SEC.addChromeListener({
      onEditorAdded: function onEditorAdded(aChrome, aEditor) {
        aChrome.removeChromeListener(this);
        doCheck(aEditor);
      }
    });
  }
  
  else {
    executeSoon(function() {
      let aEditor = SEC.editors[SEC.selectedStyleSheetIndex];
      doCheck(aEditor);
    });
  }
}

let observer = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic != "domwindowopened") {
      return;
    }
    Services.ww.unregisterNotification(observer);
    ok(true, "Style Editor window was opened in response to clicking " +
             "the location node");

    executeSoon(function() {
      styleEditorWin = window.StyleEditor
                             .StyleEditorManager
                             .getEditorForWindow(content.window);
      ok(styleEditorWin, "Style Editor Window is defined");
      styleEditorWin.addEventListener("load", function onStyleEditorWinLoad() {
        styleEditorWin.removeEventListener("load", onStyleEditorWinLoad);

        checkStyleEditorForSheetAndLine(0, 7, function() {
          checkStyleEditorForSheetAndLine(1, 6, function() {
            window.StyleEditor.toggle();
            styleEditorWin = null;
            finishTest();
          });
          EventUtils.sendMouseEvent({ type: "click" }, nodes[1]);
        });
      });
    });
  }
};
