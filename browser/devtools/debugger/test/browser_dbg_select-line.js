







const TAB_URL = "http://example.com/browser/browser/devtools/debugger/" +
                "test/browser_dbg_script-switching.html";
let tempScope = {};
Cu.import("resource:///modules/source-editor.jsm", tempScope);
let SourceEditor = tempScope.SourceEditor;

var gPane = null;
var gTab = null;
var gDebuggee = null;
var gDebugger = null;
var gScripts = null;

function test()
{
  debug_tab_pane(TAB_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPane = aPane;
    gDebugger = gPane.debuggerWindow;

    testSelectLine();
  });
}

function testSelectLine() {
  gPane.activeThread.addOneTimeListener("scriptsadded", function() {
    Services.tm.currentThread.dispatch({ run: function() {
      gScripts = gDebugger.DebuggerView.Scripts._scripts;

      is(gDebugger.StackFrames.activeThread.state, "paused",
        "Should only be getting stack frames while paused.");

      is(gScripts.itemCount, 2, "Found the expected number of scripts.");

      ok(gDebugger.editor.getText().search(/debugger/) != -1,
        "The correct script was loaded initially.");

      
      is(gDebugger.editor.getCaretPosition().line, 5,
         "The correct line is selected.");

      gDebugger.editor.addEventListener(SourceEditor.EVENTS.TEXT_CHANGED,
                                        function onChange() {
        gDebugger.editor.removeEventListener(SourceEditor.EVENTS.TEXT_CHANGED,
                                             onChange);
        ok(gDebugger.editor.getText().search(/debugger/) == -1,
          "The second script is no longer displayed.");

        ok(gDebugger.editor.getText().search(/firstCall/) != -1,
          "The first script is displayed.");

        
        
        executeSoon(function(){
          
          is(gDebugger.editor.getCaretPosition().line, 4,
             "The correct line is selected.");

          gDebugger.StackFrames.activeThread.resume(function() {
            removeTab(gTab);
            finish();
          });
        });
      });

      
      let element = gDebugger.document.getElementById("stackframe-3");
      EventUtils.synthesizeMouseAtCenter(element, {}, gDebugger);
    }}, 0);
  });

  gDebuggee.firstCall();
}
