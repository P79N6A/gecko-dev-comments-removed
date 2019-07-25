









const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

let inputNode, testKey, values, pos;

function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  waitForFocus(function() {
    openConsole();

    let hudId = HUDService.getHudIdByWindow(content);
    HUD = HUDService.hudWeakReferences[hudId].get();

    let display = HUDService.getOutputNodeById(hudId);
    inputNode = display.querySelector(".jsterm-input-node");

    inputNode.focus();

    ok(!inputNode.value, "inputNode.value is empty");

    values = ["document", "window", "document.body"];
    values.push(values.join(";\n"), "document.location");

    
    for (let i = 0; i < values.length; i++) {
      HUD.jsterm.setInputValue(values[i]);
      HUD.jsterm.execute();
    }

    inputNode.addEventListener("keyup", onKeyUp, false);

    
    testKey = "VK_UP";
    pos = values.length - 1;
    testNext();
  }, content);
}

function testNext() {
  EventUtils.synthesizeKey(testKey, {});
}

function onKeyUp() {
  is(inputNode.value, values[pos], "inputNode.value = '" + values[pos] + "'");

  if (testKey == "VK_UP") {
    pos--;
    if (pos >= 0) {
      testNext();
    }
    else {
      testMore();
    }
  }
  else {
    pos++;
    if (pos < values.length) {
      testNext();
    }
    else {
      testMore();
    }
  }
}

function testMore() {
  if (testKey == "VK_UP") {
    
    testKey = "VK_DOWN";
    pos = 0;
    testNext();
    return;
  }

  inputNode.removeEventListener("keyup", onKeyUp, false);

  
  HUD.jsterm.setInputValue(values[3]);

  inputNode.setSelectionRange(2, 2);

  inputNode.addEventListener("keyup", function(aEvent) {
    this.removeEventListener(aEvent.type, arguments.callee, false);

    is(this.value, values[3], "inputNode.value = '" + values[3] +
      "' after VK_UP from caret position 2");

    is(this.selectionStart, 0, "inputNode.selectionStart = 0");
    is(this.selectionStart, this.selectionEnd, "inputNode.selectionEnd = 0");
  }, false);

  EventUtils.synthesizeKey("VK_UP", {});

  
  inputNode.setSelectionRange(values[3].length - 2, values[3].length - 2);

  inputNode.addEventListener("keyup", function(aEvent) {
    this.removeEventListener(aEvent.type, arguments.callee, false);

    is(this.value, values[3], "inputNode.value = '" + values[3] +
      "' after VK_UP from caret position 2");

    is(this.selectionStart, this.value.length, "inputNode.selectionStart = " +
      this.value.length);
    is(this.selectionStart, this.selectionEnd, "inputNode.selectionEnd = " +
      this.value.length);

    testEnd();
  }, false);

  EventUtils.synthesizeKey("VK_DOWN", {});
}

function testEnd() {
  inputNode = testKey = values = pos = null;
  executeSoon(finishTest);
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", tabLoad, true);
}

