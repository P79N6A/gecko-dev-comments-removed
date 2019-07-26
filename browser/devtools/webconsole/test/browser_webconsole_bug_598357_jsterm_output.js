









const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

let testEnded = false;
let pos = -1;

let dateNow = Date.now();

let tempScope = {};
Cu.import("resource://gre/modules/devtools/dbg-server.jsm", tempScope);

let longString = (new Array(tempScope.DebuggerServer.LONG_STRING_LENGTH + 4)).join("a");
let initialString = longString.substring(0,
  tempScope.DebuggerServer.LONG_STRING_INITIAL_LENGTH);

let inputValues = [
  
  

  
  [false, "'hello \\nfrom \\rthe \\\"string world!'",
    '"hello \nfrom \rthe "string world!"',
    "hello \nfrom \rthe \"string world!"],

  
  [false, "'\xFA\u1E47\u0129\xE7\xF6d\xEA \u021B\u0115\u0219\u0165'",
    "\"\xFA\u1E47\u0129\xE7\xF6d\xEA \u021B\u0115\u0219\u0165\"",
    "\xFA\u1E47\u0129\xE7\xF6d\xEA \u021B\u0115\u0219\u0165"],

  
  [false, "window.location.href", '"' + TEST_URI + '"', TEST_URI],

  
  [false, "0", "0"],

  
  [false, "'0'", '"0"', "0"],

  
  [false, "42", "42"],

  
  [false, "'42'", '"42"', "42"],

  
  [true, "/foobar/", "[object RegExp]", '"/foobar/"', "[object RegExp]"],

  
  [false, "null", "null"],

  
  [false, "undefined", "undefined"],

  
  [false, "true", "true"],

  
  [true, "document.getElementById", "[object Function]",
    "function getElementById() {\n    [native code]\n}",
    "[object Function]"],

  
  [true, "(function() { return 42; })", "[object Function]",
    "function () { return 42; }", "[object Function]"],

  
  [true, "new Date(" + dateNow + ")", "[object Date]", (new Date(dateNow)).toString(), "[object Date]"],

  
  [true, "document.body", "[object HTMLBodyElement]"],

  
  [true, "window.location", "[object Location]", TEST_URI, "[object Location]"],

  
  [true, "[1,2,3,'a','b','c','4','5']", '[object Array]',
    '1,2,3,a,b,c,4,5',
    "[object Array]"],

  
  [true, "({a:'b', c:'d', e:1, f:'2'})", "[object Object]"],

  
  [false, "'" + longString + "'",
    '"' + initialString + "\"[\u2026]", initialString],
];

longString = null;
initialString = null;
tempScope = null;

let eventHandlers = [];
let popupShown = [];
let HUD;
let testDriver;

function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, tabLoad, true);

  waitForFocus(function () {
    openConsole(null, function(aHud) {
      HUD = aHud;
      testNext();
    });
  }, content);
}

function subtestNext() {
  testDriver.next();
}

function testNext() {
  pos++;
  if (pos == inputValues.length) {
    testEnd();
    return;
  }

  testDriver = testGen();
  testDriver.next();
}

function testGen() {
  let cpos = pos;

  let showsPropertyPanel = inputValues[cpos][0];
  let inputValue = inputValues[cpos][1];
  let expectedOutput = inputValues[cpos][2];

  let printOutput = inputValues[cpos].length >= 4 ?
    inputValues[cpos][3] : expectedOutput;

  let consoleOutput = inputValues[cpos].length >= 5 ?
    inputValues[cpos][4] : printOutput;

  let consoleTest = inputValues[cpos][5] || inputValue;

  HUD.jsterm.clearOutput();

  

  HUD.jsterm.execute("console.log(" + consoleTest + ")");

  waitForSuccess({
    name: "console.log message for test #" + cpos,
    validatorFn: function()
    {
      return HUD.outputNode.querySelector(".hud-log");
    },
    successFn: subtestNext,
    failureFn: testNext,
  });

  yield;

  let outputItem = HUD.outputNode.querySelector(".hud-log:last-child");
  ok(outputItem,
    "found the window.console output line for inputValues[" + cpos + "]");
  ok(outputItem.textContent.indexOf(consoleOutput) > -1,
    "console API output is correct for inputValues[" + cpos + "]");

  HUD.jsterm.clearOutput();

  

  HUD.jsterm.setInputValue("print(" + inputValue + ")");
  HUD.jsterm.execute();

  waitForSuccess({
    name: "jsterm print() output for test #" + cpos,
    validatorFn: function()
    {
      return HUD.outputNode.querySelector(".webconsole-msg-output:last-child");
    },
    successFn: subtestNext,
    failureFn: testNext,
  });

  yield;

  outputItem = HUD.outputNode.querySelector(".webconsole-msg-output:" +
                                            "last-child");
  ok(outputItem,
    "found the jsterm print() output line for inputValues[" + cpos + "]");
  ok(outputItem.textContent.indexOf(printOutput) > -1,
    "jsterm print() output is correct for inputValues[" + cpos + "]");

  

  HUD.jsterm.clearOutput();
  HUD.jsterm.setInputValue(inputValue);
  HUD.jsterm.execute();

  waitForSuccess({
    name: "jsterm output for test #" + cpos,
    validatorFn: function()
    {
      return HUD.outputNode.querySelector(".webconsole-msg-output:last-child");
    },
    successFn: subtestNext,
    failureFn: testNext,
  });

  yield;

  outputItem = HUD.outputNode.querySelector(".webconsole-msg-output:" +
                                            "last-child");
  ok(outputItem, "found the jsterm output line for inputValues[" + cpos + "]");
  ok(outputItem.textContent.indexOf(expectedOutput) > -1,
    "jsterm output is correct for inputValues[" + cpos + "]");

  let messageBody = outputItem.querySelector(".webconsole-msg-body");
  ok(messageBody, "we have the message body for inputValues[" + cpos + "]");

  
  let eventHandlerID = eventHandlers.length + 1;

  let propertyPanelShown = function(aEvent, aView, aOptions) {
    if (aOptions.label.indexOf(expectedOutput) == -1) {
      return;
    }

    HUD.jsterm.off("variablesview-open", propertyPanelShown);

    eventHandlers[eventHandlerID] = null;

    ok(showsPropertyPanel,
      "the property panel shown for inputValues[" + cpos + "]");

    HUD.jsterm._splitter.state = "collapsed";

    popupShown[cpos] = true;

    if (showsPropertyPanel) {
      executeSoon(subtestNext);
    }
  };

  HUD.jsterm.on("variablesview-open", propertyPanelShown);

  eventHandlers.push(propertyPanelShown);

  
  
  EventUtils.sendMouseEvent({ type: "mousedown" }, messageBody, window);
  EventUtils.sendMouseEvent({ type: "click" }, messageBody, window);

  if (showsPropertyPanel) {
    yield; 
  }

  testNext();

  yield;
}

function testEnd() {
  if (testEnded) {
    return;
  }

  testEnded = true;

  for (let i = 0; i < eventHandlers.length; i++) {
    if (eventHandlers[i]) {
      HUD.jsterm.off("variablesview-open", eventHandlers[i]);
    }
  }

  for (let i = 0; i < inputValues.length; i++) {
    if (inputValues[i][0] && !popupShown[i]) {
      ok(false, "the property panel failed to show for inputValues[" + i + "]");
    }
  }

  HUD = inputValues = testDriver = null;
  executeSoon(finishTest);
}

function test() {
  requestLongerTimeout(2);
  addTab(TEST_URI);
  browser.addEventListener("load", tabLoad, true);
}

