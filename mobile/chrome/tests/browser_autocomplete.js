let testURL = chromeRoot + "browser_autocomplete.html";
messageManager.loadFrameScript(chromeRoot + "remote_autocomplete.js", true);

let newTab = null;


var gTests = [];
var gCurrentTest = null;

function test() {
  
  waitForExplicitFinish();

  
  messageManager.addMessageListener("pageshow", function(aMessage) {
    if (newTab && newTab.browser.currentURI.spec != "about:blank") {
      messageManager.removeMessageListener(aMessage.name, arguments.callee);
      BrowserUI.closeAutoComplete(true);
      setTimeout(runNextTest, 0);
    }
  });

  let startupInfo = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup).getStartupInfo();
  if (!("firstPaint" in startupInfo))
    waitFor(function() { newTab = Browser.addTab(testURL, true); }, function() {
      let startupInfo = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup).getStartupInfo();
      return ("firstPaint" in startupInfo);
    }, Date.now() + 3000);
  else
    newTab = Browser.addTab(testURL, true);
}



function runNextTest() {
  
  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();
    info(gCurrentTest.desc);
    gCurrentTest.run();
  }
  else {
    
    try {
      

      
      Browser.closeTab(newTab);
    }
    finally {
      
      finish();
    }
  }
}

function waitForAutocomplete(aCallback) {
  messageManager.addMessageListener("FormAssist:AutoComplete", function(aMessage) {
    messageManager.removeMessageListener(aMessage.name, arguments.callee);
    setTimeout(function() {
      aCallback(aMessage.json.current.list);
    }, 0);
  });
};

let data = [
  { label: "foo", value: "foo" },
  { label: "Somewhat bar", value: "bar" },
  { label: "foobar", value: "_" }
];



gTests.push({
  desc: "Click on a datalist element and show suggestions",

  run: function() {
    waitForAutocomplete(gCurrentTest.checkData);
    AsyncTests.waitFor("TestRemoteAutocomplete:Click",
                        { id: "input-datalist-1" }, function(json) {});
  },

  
  
  checkData: function(aOptions) {
    for (let i = 0; i < aOptions.length; i++) {
      let option = aOptions[i];
      let valid = data[i];

      is(option.label, valid.label, "Label should be equal (" + option.label + ", " + valid.label +")");
      is(option.value, valid.value, "Value should be equal (" + option.value + ", " + valid.value +")");
    }

    
    waitFor(gCurrentTest.checkUI, function() {
      let suggestionsBox = document.getElementById("form-helper-suggestions");
      return suggestionsBox.childNodes.length;
    });
  },

  
  checkUI: function() {
    let suggestionsBox = document.getElementById("form-helper-suggestions");
    let suggestions = suggestionsBox.childNodes;

    for (let i = 0; i < suggestions.length; i++) {
      let suggestion = suggestions[i];
      let valid = data[i];
      let label = suggestion.getAttribute("value");
      let value = suggestion.getAttribute("data");

      is(label, valid.label, "Label should be equal (" + label + ", " + valid.label +")");
      is(value, valid.value, "Value should be equal (" + value + ", " + valid.value +")");
    }

    gCurrentTest.checkUIClick(0);
  },

  
  
  checkUIClick: function(aIndex) {
    let suggestionsBox = document.getElementById("form-helper-suggestions");

    let suggestion = suggestionsBox.childNodes[aIndex];
    if (!suggestion) {
      gCurrentTest.finish();
      return;
    }

    
    FormHelperUI.doAutoComplete(suggestion);

    AsyncTests.waitFor("TestRemoteAutocomplete:Check", { id: "input-datalist-1" }, function(json) {
      is(json.result, suggestion.getAttribute("data"), "The target input value should be set to " + data);
      gCurrentTest.checkUIClick(aIndex + 1);
    });
  },

  finish: function() {
    
    FormHelperUI.hide();


    AsyncTests.waitFor("TestRemoteAutocomplete:Reset", { id: "input-datalist-1" }, function(json) {
      runNextTest();
    });
  }
});



gTests.push({
  desc: "Check arrows visibility",

  run: function() {
    let popup = document.getElementById("form-helper-suggestions-container");
    popup.addEventListener("contentpopupshown", function(aEvent) {
      aEvent.target.removeEventListener(aEvent.type, arguments.callee, false);
      waitFor(gCurrentTest.checkNoArrows, function() {
        return FormHelperUI._open;
      });
    }, false);

    AsyncTests.waitFor("TestRemoteAutocomplete:Click",
                        { id: "input-datalist-3" }, function(json) {});
  },

  checkNoArrows: function() {
    let scrollbox = document.getElementById("form-helper-suggestions");
    todo_is(scrollbox._scrollButtonUp.collapsed, true, "Left button should be collapsed");
    todo_is(scrollbox._scrollButtonDown.collapsed, true, "Right button should be collapsed");
    gCurrentTest.finish();
  },

  finish: function() {
    
    FormHelperUI.hide();

    runNextTest();
  }
});

