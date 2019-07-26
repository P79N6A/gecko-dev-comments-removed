







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
      try {

        yield ensureSourceIs(gPanel, CODE_URL, true);

        
        yield doInterrupt();
        const [bp1, bp2, bp3] = yield promise.all([
          setBreakpoint({
            url: CODE_URL,
            line: 2
          }),
          setBreakpoint({
            url: CODE_URL,
            line: 3
          }),
          setBreakpoint({
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

        
        yield promise.all([
          rdpInvoke(bp1, bp1.remove),
          rdpInvoke(bp2, bp1.remove),
          rdpInvoke(bp3, bp1.remove),
        ]);

        yield resumeDebuggerThenCloseAndFinish(gPanel);

      } catch (e) {
        DevToolsUtils.reportException(
          "browser_dbg_breakpoints-break-on-last-line-of-script-on-reload.js",
          e
        );
        ok(false);
      }
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

  function setBreakpoint(location) {
    let deferred = promise.defer();
    gThreadClient.setBreakpoint(location, ({ error, message }, bpClient) => {
      if (error) {
        deferred.reject(error + ": " + message);
      }
      deferred.resolve(bpClient);
    });
    return deferred.promise;
  }
}
