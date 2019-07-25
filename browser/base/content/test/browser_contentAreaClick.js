


































 












let gTests = [

  {
    desc: "Simple left click",
    setup: function() {},
    clean: function() {},
    event: {},
    target: "commonlink",
    expectedInvokedMethods: [],
    preventDefault: false,
  },

  {
    desc: "Ctrl/Cmd left click",
    setup: function() {},
    clean: function() {},
    event: { ctrlKey: true,
             metaKey: true },
    target: "commonlink",
    expectedInvokedMethods: [ "urlSecurityCheck", "openLinkIn" ],
    preventDefault: true,
  },

  
  
  {
    desc: "Shift+Alt left click",
    setup: function() {},
    clean: function() {},
    event: { shiftKey: true,
             altKey: true },
    target: "commonlink",
    expectedInvokedMethods: [ "gatherTextUnder", "saveURL" ],
    preventDefault: true,
  },

  {
    desc: "Shift click",
    setup: function() {},
    clean: function() {},
    event: { shiftKey: true },
    target: "commonlink",
    expectedInvokedMethods: [ "urlSecurityCheck", "openLinkIn" ],
    preventDefault: true,
  },

  {
    desc: "Alt click",
    setup: function() {},
    clean: function() {},
    event: { altKey: true },
    target: "commonlink",
    expectedInvokedMethods: [ "gatherTextUnder", "saveURL" ],
    preventDefault: true,
  },

  {
    desc: "Panel click",
    setup: function() {},
    clean: function() {},
    event: {},
    target: "panellink",
    expectedInvokedMethods: [ "urlSecurityCheck", "getShortcutOrURI", "loadURI" ],
    preventDefault: true,
  },

  {
    desc: "Simple middle click opentab",
    setup: function() {},
    clean: function() {},
    event: { button: 1 },
    target: "commonlink",
    expectedInvokedMethods: [ "urlSecurityCheck", "openLinkIn" ],
    preventDefault: true,
  },

  {
    desc: "Simple middle click openwin",
    setup: function() {
      gPrefService.setBoolPref("browser.tabs.opentabfor.middleclick", false);
    },
    clean: function() {
      try {
        gPrefService.clearUserPref("browser.tabs.opentabfor.middleclick");
      } catch(ex) {}
    },
    event: { button: 1 },
    target: "commonlink",
    expectedInvokedMethods: [ "urlSecurityCheck", "openLinkIn" ],
    preventDefault: true,
  },

  {
    desc: "Middle mouse paste",
    setup: function() {
      gPrefService.setBoolPref("middlemouse.contentLoadURL", true);
      gPrefService.setBoolPref("general.autoScroll", false);
    },
    clean: function() {
      try {
        gPrefService.clearUserPref("middlemouse.contentLoadURL");
      } catch(ex) {}
      try {
        gPrefService.clearUserPref("general.autoScroll");
      } catch(ex) {}
    },
    event: { button: 1 },
    target: "emptylink",
    expectedInvokedMethods: [ "middleMousePaste" ],
    preventDefault: true,
  },

];


let gReplacedMethods = [
  "middleMousePaste",
  "urlSecurityCheck",
  "loadURI",
  "gatherTextUnder",
  "saveURL",
  "openLinkIn",
  "getShortcutOrURI",
];


let gTestWin = null;


let gInvokedMethods = [];


let gCurrentTest = null;

function test() {
  waitForExplicitFinish();

  gTestWin = openDialog(location, "", "chrome,all,dialog=no", "about:blank");
  gTestWin.addEventListener("load", function (event) {
    info("Window loaded.");
    gTestWin.removeEventListener("load", arguments.callee, false);
    waitForFocus(function() {
      info("Setting up browser...");
      setupTestBrowserWindow();
      info("Running tests...");
      executeSoon(runNextTest);
    }, gTestWin.content, true);
  }, false);
}


let gClickHandler = {
  handleEvent: function (event) {
    let linkId = event.target.id;
    is(event.type, "click",
       gCurrentTest.desc + ":Handler received a click event on " + linkId);

    let isPanelClick = linkId == "panellink";
    let returnValue = gTestWin.contentAreaClick(event, isPanelClick);
    let prevent = event.getPreventDefault();
    is(prevent, gCurrentTest.preventDefault,
       gCurrentTest.desc + ": event.getPreventDefault() is correct (" + prevent + ")")

    
    gCurrentTest.expectedInvokedMethods.forEach(function(aExpectedMethodName) {
      isnot(gInvokedMethods.indexOf(aExpectedMethodName), -1,
            gCurrentTest.desc + ":" + aExpectedMethodName + " was invoked");
    });
    
    if (gInvokedMethods.length != gCurrentTest.expectedInvokedMethods.length) {
      is(false, "More than the expected methods have been called");
      gInvokedMethods.forEach(function (method) info(method + " was invoked"));
    }

    event.preventDefault();
    event.stopPropagation();

    executeSoon(runNextTest);
  }
}


function wrapperMethod(aInvokedMethods, aMethodName) {
  return function () {
    aInvokedMethods.push(aMethodName);
    
    return arguments[0];
  }
}

function setupTestBrowserWindow() {
  
  gTestWin.addEventListener("click", gClickHandler, true);

  
  gReplacedMethods.forEach(function (aMethodName) {
    gTestWin["old_" + aMethodName] = gTestWin[aMethodName];
    gTestWin[aMethodName] = wrapperMethod(gInvokedMethods, aMethodName);
  });

  
  let doc = gTestWin.content.document;
  let mainDiv = doc.createElement("div");
  mainDiv.innerHTML =
    '<a id="commonlink" href="http://mochi.test/moz/">Common link</a>' +
    '<a id="panellink" href="http://mochi.test/moz/">Panel link</a>' +
    '<a id="emptylink">Empty link</a>';
  doc.body.appendChild(mainDiv);
}

function runNextTest() {
  if (gCurrentTest) {
    info(gCurrentTest.desc + ": cleaning up...")
    gCurrentTest.clean();
    gInvokedMethods.length = 0;
  }

  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();

    info(gCurrentTest.desc + ": starting...");
    
    gCurrentTest.setup();

    
    let target = gTestWin.content.document.getElementById(gCurrentTest.target);
    ok(target, gCurrentTest.desc + ": target is valid (" + target.id + ")");
    EventUtils.synthesizeMouse(target, 2, 2, gCurrentTest.event, gTestWin.content);
  }
  else {
    
    finishTest()
  }
}

function finishTest() {
  info("Restoring browser...");
  gTestWin.removeEventListener("click", gClickHandler, true);

  
  gReplacedMethods.forEach(function (aMethodName) {
    gTestWin[aMethodName] = gTestWin["old_" + aMethodName];
    delete gTestWin["old_" + aMethodName];
  });

  gTestWin.close();
  finish();
}
