








const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-eval-in-stackframe.html";

let gWebConsole, gJSTerm, gDebuggerWin, gThread, gDebuggerController,
    gStackframes, gVariablesView;

function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(hud)
{
  gWebConsole = hud;
  gJSTerm = hud.jsterm;

  executeSoon(() => {
    info("openDebugger");
    openDebugger().then(debuggerOpened);
  });
}

function debuggerOpened(aResult)
{
  gDebuggerWin = aResult.panelWin;
  gDebuggerController = gDebuggerWin.DebuggerController;
  gThread = gDebuggerController.activeThread;
  gStackframes = gDebuggerController.StackFrames;

  executeSoon(() => {
    gThread.addOneTimeListener("framesadded", onFramesAdded);

    info("firstCall()");
    content.wrappedJSObject.firstCall();
  });
}

function onFramesAdded()
{
  info("onFramesAdded");

  executeSoon(() =>
    openConsole(null, () =>
      gJSTerm.execute("fooObj", onExecuteFooObj)
    )
  );
}


function onExecuteFooObj()
{
  let msg = gWebConsole.outputNode.querySelector(".webconsole-msg-output");
  ok(msg, "output message found");
  isnot(msg.textContent.indexOf("[object Object]"), -1, "message text check");

  gJSTerm.once("variablesview-fetched", onFooObjFetch);

  executeSoon(() => EventUtils.synthesizeMouse(msg, 2, 2, {},
                                               gWebConsole.iframeWindow));
}

function onFooObjFetch(aEvent, aVar)
{
  gVariablesView = aVar._variablesView;
  ok(gVariablesView, "variables view object");

  findVariableViewProperties(aVar, [
    { name: "testProp2", value: "testValue2" },
    { name: "testProp", value: "testValue", dontMatch: true },
  ], { webconsole: gWebConsole }).then(onTestPropFound);
}

function onTestPropFound(aResults)
{
  let prop = aResults[0].matchedProp;
  ok(prop, "matched the |testProp2| property in the variables view");

  
  
  updateVariablesViewProperty({
    property: prop,
    field: "value",
    string: "document.title + foo2 + $('p')",
    webconsole: gWebConsole,
    callback: onFooObjFetchAfterUpdate,
  });
}

function onFooObjFetchAfterUpdate(aEvent, aVar)
{
  info("onFooObjFetchAfterUpdate");
  let para = content.document.querySelector("p");
  let expectedValue = content.document.title + "foo2SecondCall" + para;

  findVariableViewProperties(aVar, [
    { name: "testProp2", value: expectedValue },
  ], { webconsole: gWebConsole }).then(onUpdatedTestPropFound);
}

function onUpdatedTestPropFound(aResults)
{
  let prop = aResults[0].matchedProp;
  ok(prop, "matched the updated |testProp2| property value");

  
  executeSoon(() => gJSTerm.execute("fooObj.testProp2", onExecuteFooObjTestProp2));
}

function onExecuteFooObjTestProp2()
{
  let para = content.document.querySelector("p");
  let expected = content.document.title + "foo2SecondCall" + para;

  isnot(gWebConsole.outputNode.textContent.indexOf(expected), -1,
        "fooObj.testProp2 is correct");

  gWebConsole = gJSTerm = gDebuggerWin = gThread = gDebuggerController =
    gStackframes = gVariablesView = null;
  executeSoon(finishTest);
}
