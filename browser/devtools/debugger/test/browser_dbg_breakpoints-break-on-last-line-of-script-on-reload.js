







const TAB_URL = EXAMPLE_URL + "doc_breakpoints-break-on-last-line-of-script-on-reload.html";
const CODE_URL = EXAMPLE_URL + "code_breakpoints-break-on-last-line-of-script-on-reload.js";

const { promiseInvoke } = require("devtools/async-utils");

function test() {
  let gPanel, gDebugger, gThreadClient, gEvents;

  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gThreadClient = gDebugger.gThreadClient;
    gEvents = gDebugger.EVENTS;

    Task.spawn(function* () {
      yield waitForSourceShown(gPanel, CODE_URL);

      
      yield doInterrupt();
      yield promise.all([
        gPanel.addBreakpoint({
          url: CODE_URL,
          line: 2
        }),
        gPanel.addBreakpoint({
          url: CODE_URL,
          line: 3
        }),
        gPanel.addBreakpoint({
          url: CODE_URL,
          line: 4
        })
      ]);

      
      yield promise.all([
        reloadActiveTab(gPanel, gEvents.SOURCE_SHOWN),
        waitForCaretUpdated(gPanel, 2)
      ]);

      
      yield promise.all([
        doResume(),
        waitForCaretUpdated(gPanel, 3)
      ]);
      yield promise.all([
        doResume(),
        waitForCaretUpdated(gPanel, 4)
      ]);

      yield resumeDebuggerThenCloseAndFinish(gPanel);
    });
  });

  function rdpInvoke(obj, method) {
    return promiseInvoke(obj, method)
      .then(({error, message }) => {
        if (error) {
          throw new Error(error + ": " + message);
        }
      });
  }

  function doResume() {
    return rdpInvoke(gThreadClient, gThreadClient.resume);
  }

  function doInterrupt() {
    return rdpInvoke(gThreadClient, gThreadClient.interrupt);
  }
}
