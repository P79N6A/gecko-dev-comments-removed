












"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Web Console test for " +
                 "bug 804845 and bug 619598";

let jsterm, inputNode;

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  doTests(hud);

  jsterm = inputNode = null;
});

function doTests(HUD) {
  jsterm = HUD.jsterm;
  inputNode = jsterm.inputNode;
  ok(!jsterm.inputNode.value, "inputNode.value is empty");
  is(jsterm.inputNode.selectionStart, 0);
  is(jsterm.inputNode.selectionEnd, 0);

  testSingleLineInputNavNoHistory();
  testMultiLineInputNavNoHistory();
  testNavWithHistory();
}

function testSingleLineInputNavNoHistory() {
  
  EventUtils.synthesizeKey("1", {});
  is(inputNode.selectionStart, 1, "caret location after single char input");

  
  EventUtils.synthesizeKey("a", { ctrlKey: true });
  is(inputNode.selectionStart, 0,
     "caret location after single char input and ctrl-a");

  EventUtils.synthesizeKey("e", { ctrlKey: true });
  is(inputNode.selectionStart, 1,
     "caret location after single char input and ctrl-e");

  
  EventUtils.synthesizeKey("2", {});
  
  EventUtils.synthesizeKey("VK_UP", {});
  is(inputNode.selectionStart, 0,
     "caret location after two char input and VK_UP");
  EventUtils.synthesizeKey("VK_DOWN", {});
  is(inputNode.selectionStart, 2,
     "caret location after two char input and VK_DOWN");

  EventUtils.synthesizeKey("a", { ctrlKey: true });
  is(inputNode.selectionStart, 0,
     "move caret to beginning of 2 char input with ctrl-a");
  EventUtils.synthesizeKey("a", { ctrlKey: true });
  is(inputNode.selectionStart, 0,
     "no change of caret location on repeat ctrl-a");
  EventUtils.synthesizeKey("p", { ctrlKey: true });
  is(inputNode.selectionStart, 0,
     "no change of caret location on ctrl-p from beginning of line");

  EventUtils.synthesizeKey("e", { ctrlKey: true });
  is(inputNode.selectionStart, 2,
     "move caret to end of 2 char input with ctrl-e");
  EventUtils.synthesizeKey("e", { ctrlKey: true });
  is(inputNode.selectionStart, 2,
     "no change of caret location on repeat ctrl-e");
  EventUtils.synthesizeKey("n", { ctrlKey: true });
  is(inputNode.selectionStart, 2,
     "no change of caret location on ctrl-n from end of line");

  EventUtils.synthesizeKey("p", { ctrlKey: true });
  is(inputNode.selectionStart, 0, "ctrl-p moves to start of line");

  EventUtils.synthesizeKey("n", { ctrlKey: true });
  is(inputNode.selectionStart, 2, "ctrl-n moves to end of line");
}

function testMultiLineInputNavNoHistory() {
  let lineValues = ["one", "2", "something longer", "", "", "three!"];
  jsterm.setInputValue("");
  
  for (let i = 0; i < lineValues.length; i++) {
    jsterm.setInputValue(inputNode.value + lineValues[i]);
    EventUtils.synthesizeKey("VK_RETURN", { shiftKey: true });
  }
  let inputValue = inputNode.value;
  is(inputNode.selectionStart, inputNode.selectionEnd);
  is(inputNode.selectionStart, inputValue.length,
     "caret at end of multiline input");

  
  
  let newlineString = inputValue.match(/(\r\n?|\n\r?)$/)[0];

  
  EventUtils.synthesizeKey("VK_UP", {});
  let expectedStringAfterCarat = lineValues[5] + newlineString;
  is(inputNode.value.slice(inputNode.selectionStart), expectedStringAfterCarat,
     "up arrow from end of multiline");

  EventUtils.synthesizeKey("VK_DOWN", {});
  is(inputNode.value.slice(inputNode.selectionStart), "",
     "down arrow from within multiline");

  
  EventUtils.synthesizeKey("p", { ctrlKey: true });
  is(inputNode.value.slice(inputNode.selectionStart), expectedStringAfterCarat,
     "ctrl-p from end of multiline");

  for (let i = 4; i >= 0; i--) {
    EventUtils.synthesizeKey("p", { ctrlKey: true });
    expectedStringAfterCarat = lineValues[i] + newlineString +
      expectedStringAfterCarat;
    is(inputNode.value.slice(inputNode.selectionStart),
      expectedStringAfterCarat, "ctrl-p from within line " + i +
      " of multiline input");
  }
  EventUtils.synthesizeKey("p", { ctrlKey: true });
  is(inputNode.selectionStart, 0, "reached start of input");
  is(inputNode.value, inputValue,
     "no change to multiline input on ctrl-p from beginning of multiline");

  
  EventUtils.synthesizeKey("e", { ctrlKey: true });
  let caretPos = inputNode.selectionStart;
  let expectedStringBeforeCarat = lineValues[0];
  is(inputNode.value.slice(0, caretPos), expectedStringBeforeCarat,
     "ctrl-e into multiline input");
  EventUtils.synthesizeKey("e", { ctrlKey: true });
  is(inputNode.selectionStart, caretPos,
     "repeat ctrl-e doesn't change caret position in multiline input");

  
  for (let i = 1; i < lineValues.length; i++) {
    EventUtils.synthesizeKey("n", { ctrlKey: true });
    EventUtils.synthesizeKey("a", { ctrlKey: true });
    caretPos = inputNode.selectionStart;
    expectedStringBeforeCarat += newlineString;
    is(inputNode.value.slice(0, caretPos), expectedStringBeforeCarat,
       "ctrl-a to beginning of line " + (i + 1) + " in multiline input");

    EventUtils.synthesizeKey("e", { ctrlKey: true });
    caretPos = inputNode.selectionStart;
    expectedStringBeforeCarat += lineValues[i];
    is(inputNode.value.slice(0, caretPos), expectedStringBeforeCarat,
       "ctrl-e to end of line " + (i + 1) + "in multiline input");
  }
}

function testNavWithHistory() {
  
  
  let values = ['"single line input"',
                '"a longer single-line input to check caret repositioning"',
                ['"multi-line"', '"input"', '"here!"'].join("\n"),
               ];
  
  for (let i = 0; i < values.length; i++) {
    jsterm.setInputValue(values[i]);
    jsterm.execute();
  }
  is(inputNode.selectionStart, 0, "caret location at start of empty line");

  EventUtils.synthesizeKey("p", { ctrlKey: true });
  is(inputNode.selectionStart, values[values.length - 1].length,
     "caret location correct at end of last history input");

  
  for (let i = values.length - 1; i > 0; i--) {
    let match = values[i].match(/(\n)/g);
    if (match) {
      
      EventUtils.synthesizeKey("a", { ctrlKey: true });
      for (let j = 0; j < match.length; j++) {
        EventUtils.synthesizeKey("p", { ctrlKey: true });
      }
      EventUtils.synthesizeKey("p", { ctrlKey: true });
    } else {
      
      EventUtils.synthesizeKey("p", { ctrlKey: true });
    }
    is(inputNode.value, values[i - 1],
       "ctrl-p updates inputNode from backwards history values[" + i - 1 + "]");
  }
  let inputValue = inputNode.value;
  EventUtils.synthesizeKey("p", { ctrlKey: true });
  is(inputNode.selectionStart, 0,
     "ctrl-p at beginning of history moves caret location to beginning " +
     "of line");
  is(inputNode.value, inputValue,
     "no change to input value on ctrl-p from beginning of line");

  
  for (let i = 1; i < values.length; i++) {
    EventUtils.synthesizeKey("n", { ctrlKey: true });
    is(inputNode.value, values[i],
       "ctrl-n updates inputNode from forwards history values[" + i + "]");
    is(inputNode.selectionStart, values[i].length,
       "caret location correct at end of history input for values[" + i + "]");
  }
  EventUtils.synthesizeKey("n", { ctrlKey: true });
  ok(!inputNode.value, "ctrl-n at end of history updates to empty input");

  
  inputValue = "one\nlinebreak";
  jsterm.setInputValue(inputValue);

  
  EventUtils.synthesizeKey("p", { ctrlKey: true });
  is(inputNode.value, inputValue,
     "ctrl-p from end of multi-line does not trigger history");

  EventUtils.synthesizeKey("a", { ctrlKey: true });
  EventUtils.synthesizeKey("p", { ctrlKey: true });
  is(inputNode.value, values[values.length - 1],
     "ctrl-p from start of multi-line triggers history");
}
