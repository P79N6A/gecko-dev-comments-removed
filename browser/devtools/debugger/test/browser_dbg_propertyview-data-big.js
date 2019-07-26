








const TAB_URL = EXAMPLE_URL + "browser_dbg_big-data.html";

var gPane = null;
var gTab = null;
var gDebugger = null;

requestLongerTimeout(10);

function test()
{
  debug_tab_pane(TAB_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gPane = aPane;
    gDebugger = gPane.panelWin;

    gDebugger.DebuggerController.StackFrames.autoScopeExpand = true;
    gDebugger.DebuggerView.Variables.nonEnumVisible = false;
    gDebugger.DebuggerView.Variables.lazyAppend = true;
    testWithFrame();
  });
}

function testWithFrame()
{
  let count = 0;
  gDebugger.addEventListener("Debugger:FetchedVariables", function test1() {
    
    
    if (++count < 2) {
      info("Number of received Debugger:FetchedVariables events: " + count);
      return;
    }
    gDebugger.removeEventListener("Debugger:FetchedVariables", test1, false);
    Services.tm.currentThread.dispatch({ run: function() {

      var scopes = gDebugger.DebuggerView.Variables._list,
          innerScope = scopes.querySelectorAll(".scope")[0],
          loadScope = scopes.querySelectorAll(".scope")[1],
          globalScope = scopes.querySelectorAll(".scope")[2],
          innerNodes = innerScope.querySelector(".details").childNodes,
          arrayNodes = innerNodes[4].querySelector(".details").childNodes;

      is(innerNodes[3].querySelector(".name").getAttribute("value"), "buffer",
        "Should have the right property name for |buffer|.");

      is(innerNodes[3].querySelector(".value").getAttribute("value"), "[object ArrayBuffer]",
        "Should have the right property value for |buffer|.");

      is(innerNodes[4].querySelector(".name").getAttribute("value"), "z",
        "Should have the right property name for |z|.");

      is(innerNodes[4].querySelector(".value").getAttribute("value"), "[object Int8Array]",
        "Should have the right property value for |z|.");


      EventUtils.sendMouseEvent({ type: "mousedown" }, innerNodes[3].querySelector(".arrow"), gDebugger);
      EventUtils.sendMouseEvent({ type: "mousedown" }, innerNodes[4].querySelector(".arrow"), gDebugger);

      gDebugger.addEventListener("Debugger:FetchedProperties", function test2() {
        gDebugger.removeEventListener("Debugger:FetchedProperties", test2, false);
        Services.tm.currentThread.dispatch({ run: function() {

          let total = 10000;
          let loaded = 0;
          let paints = 0;

          waitForProperties(total, {
            onLoading: function(count) {
              ok(count >= loaded, "Should have loaded more properties.");
              info("Displayed " + count + " properties, not finished yet.");
              info("Remaining " + (total - count) + " properties to display.");
              loaded = count;
              paints++;

              loadScope.hidden = true;
              globalScope.hidden = true;
              scopes.parentNode.scrollTop = scopes.parentNode.scrollHeight;
            },
            onFinished: function(count) {
              ok(count == total, "Displayed all the properties.");
              isnot(paints, 0, "Debugger was unresponsive, sad panda.");

              for (let i = 0; i < arrayNodes.length; i++) {
                let node = arrayNodes[i];
                let name = node.querySelector(".name").getAttribute("value");
                
                
                if (name !== i + "") {
                  ok(false, "The array items aren't in the correct order.");
                }
              }

              closeDebuggerAndFinish();
            }
          });
        }}, 0);
      }, false);
    }}, 0);
  }, false);

  EventUtils.sendMouseEvent({ type: "click" },
    content.document.querySelector("button"),
    content.window);
}

function waitForProperties(total, callbacks)
{
  var scopes = gDebugger.DebuggerView.Variables._list,
      innerScope = scopes.querySelectorAll(".scope")[0],
      innerNodes = innerScope.querySelector(".details").childNodes,
      arrayNodes = innerNodes[4].querySelector(".details").childNodes;

  
  let count = 0;
  let intervalID = window.setInterval(function() {
    info("count: " + count + " ");
    if (++count > total) {
      ok(false, "Timed out while polling for the properties.");
      window.clearInterval(intervalID);
      return closeDebuggerAndFinish();
    }
    
    if (arrayNodes.length < total) {
      callbacks.onLoading(arrayNodes.length);
      return;
    }
    
    window.clearInterval(intervalID);
    callbacks.onFinished(arrayNodes.length);
  }, 100);
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebugger = null;
});
