



let testURL = "http://example.org/browser/browser/base/content/test/general/dummy_page.html";
let testActionURL = "moz-action:switchtab," + JSON.stringify({url: testURL});
testURL = gURLBar.trimValue(testURL);
let testTab;

function runNextTest() {
  if (tests.length) {
    let t = tests.shift();
    waitForClipboard(t.expected, t.setup, function() {
      t.success();
      runNextTest();
    }, cleanup);
  }
  else {
    cleanup();
  }
}

function cleanup() {
  gBrowser.removeTab(testTab);
  finish();
}

let tests = [
  {
    expected: testURL,
    setup: function() {
      gURLBar.value = testActionURL;
      gURLBar.valueIsTyped = true;
      is(gURLBar.value, testActionURL, "gURLBar.value starts with correct value");

      
      gURLBar.focus();
      gURLBar.select();
      goDoCommand("cmd_copy");
    },
    success: function() {
      is(gURLBar.value, testActionURL, "gURLBar.value didn't change when copying");
    }
  },
  {
    expected: testURL.substring(0, 10),
    setup: function() {
      
      gURLBar.selectionStart = 0;
      gURLBar.selectionEnd = 10;
      goDoCommand("cmd_copy");
    },
    success: function() {
      is(gURLBar.value, testActionURL, "gURLBar.value didn't change when copying");
    }
  },
  {
    expected: testURL,
    setup: function() {
      
      
      gURLBar.select();
      goDoCommand("cmd_cut");
    },
    success: function() {
      is(gURLBar.value, "", "gURLBar.value is now empty");
    }
  },
  {
    expected: testURL.substring(testURL.length - 10, testURL.length),
    setup: function() {
      
      gURLBar.value = testActionURL;
      gURLBar.valueIsTyped = true;
      
      is(gURLBar.value, testActionURL, "gURLBar.value starts with correct value");

      
      gURLBar.selectionStart = testURL.length - 10;
      gURLBar.selectionEnd = testURL.length;
      goDoCommand("cmd_cut");
    },
    success: function() {
      is(gURLBar.value, testURL.substring(0, testURL.length - 10), "gURLBar.value has the correct value");
    }
  }
];

function test() {
  waitForExplicitFinish();
  testTab = gBrowser.addTab();
  gBrowser.selectedTab = testTab;

  
  runNextTest();
}
