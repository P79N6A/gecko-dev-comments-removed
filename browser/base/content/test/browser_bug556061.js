




































let cbSvc = Cc["@mozilla.org/widget/clipboard;1"].
            getService(Ci.nsIClipboard);

let testURL = "http://example.org/browser/browser/base/content/test/dummy_page.html";
let testActionURL = "moz-action:switchtab," + testURL;
let testTab;
let clipboardText = "";
let currentClipboardText = null;
let clipboardPolls = 0;





function waitForClipboard() {
  
  if (++clipboardPolls > 50) {
    
    ok(false, "Timed out while polling clipboard for pasted data");
    
    cleanup();
    return;
  }

  let xferable = Cc["@mozilla.org/widget/transferable;1"].
                 createInstance(Ci.nsITransferable);
  xferable.addDataFlavor("text/unicode");
  cbSvc.getData(xferable, cbSvc.kGlobalClipboard);
  try {
    let data = {};
    xferable.getTransferData("text/unicode", data, {});
    currentClipboardText = data.value.QueryInterface(Ci.nsISupportsString).data;
  } catch (e) {}

  if (currentClipboardText == clipboardText) {
    setTimeout(waitForClipboard, 100);
  } else {
    clipboardText = currentClipboardText;
    runNextTest();
  }
}

function runNextTest() {
  
  clipboardPolls = 0;
  
  tests.shift()();
}

function cleanup() {
  gBrowser.removeTab(testTab);
  finish();
}


let tests = [
  function () {
    
    gURLBar.value = testActionURL;
    is(gURLBar.value, testActionURL, "gURLBar.value starts with correct value");

    
    gURLBar.focus();
    gURLBar.select();
    goDoCommand("cmd_copy");
    waitForClipboard();
  },
  function () {
    is(clipboardText, testURL, "Clipboard has the correct value");
    
    is(gURLBar.value, testActionURL, "gURLBar.value didn't change when copying");

    
    gURLBar.selectionStart = 0;
    gURLBar.selectionEnd = 10;
    goDoCommand("cmd_copy");
    waitForClipboard();
  },
  function () {
    is(clipboardText, testURL.substring(0, 10), "Clipboard has the correct value");
    is(gURLBar.value, testActionURL, "gURLBar.value didn't change when copying");

    
    
    gURLBar.select();
    goDoCommand("cmd_cut");
    waitForClipboard();
  },
  function () {
    is(clipboardText, testURL, "Clipboard has the correct value");
    is(gURLBar.value, "", "gURLBar.value is now empty");

    
    gURLBar.value = testActionURL;
    
    is(gURLBar.value, testActionURL, "gURLBar.value starts with correct value");

    
    gURLBar.selectionStart = testURL.length - 10;
    gURLBar.selectionEnd = testURL.length;

    goDoCommand("cmd_cut");
    waitForClipboard();
  },
  function () {
    is(clipboardText, testURL.substring(testURL.length - 10, testURL.length),
       "Clipboard has the correct value");
    is(gURLBar.value, testURL.substring(0, testURL.length - 10), "gURLBar.value has the correct value");

    
    cleanup();
  }
]

function test() {
  waitForExplicitFinish();
  testTab = gBrowser.addTab();
  gBrowser.selectedTab = testTab;

  
  runNextTest();
}
