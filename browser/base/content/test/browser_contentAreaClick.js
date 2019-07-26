


 












let gTests = [

  {
    desc: "Simple left click",
    setup: function() {},
    clean: function() {},
    event: {},
    targets: [ "commonlink", "mathxlink", "svgxlink", "maplink" ],
    expectedInvokedMethods: [],
    preventDefault: false,
  },

  {
    desc: "Ctrl/Cmd left click",
    setup: function() {},
    clean: function() {},
    event: { ctrlKey: true,
             metaKey: true },
    targets: [ "commonlink", "mathxlink", "svgxlink", "maplink" ],
    expectedInvokedMethods: [ "urlSecurityCheck", "openLinkIn" ],
    preventDefault: true,
  },

  
  
  {
    desc: "Shift+Alt left click",
    setup: function() {
      gPrefService.setBoolPref("browser.altClickSave", true);
    },
    clean: function() {
      gPrefService.clearUserPref("browser.altClickSave"); 
    },
    event: { shiftKey: true,
             altKey: true },
    targets: [ "commonlink", "maplink" ],
    expectedInvokedMethods: [ "gatherTextUnder", "saveURL" ],
    preventDefault: true,
  },

  {
    desc: "Shift+Alt left click on XLinks",
    setup: function() {
      gPrefService.setBoolPref("browser.altClickSave", true);
    },
    clean: function() {
      gPrefService.clearUserPref("browser.altClickSave"); 
    },
    event: { shiftKey: true,
             altKey: true },
    targets: [ "mathxlink", "svgxlink"],
    expectedInvokedMethods: [ "saveURL" ],
    preventDefault: true,
  },

  {
    desc: "Shift click",
    setup: function() {},
    clean: function() {},
    event: { shiftKey: true },
    targets: [ "commonlink", "mathxlink", "svgxlink", "maplink" ],
    expectedInvokedMethods: [ "urlSecurityCheck", "openLinkIn" ],
    preventDefault: true,
  },

  {
    desc: "Alt click",
    setup: function() {
      gPrefService.setBoolPref("browser.altClickSave", true);
    },
    clean: function() {
      gPrefService.clearUserPref("browser.altClickSave"); 
    },
    event: { altKey: true },
    targets: [ "commonlink", "maplink" ],
    expectedInvokedMethods: [ "gatherTextUnder", "saveURL" ],
    preventDefault: true,
  },

  {
    desc: "Alt click on XLinks",
    setup: function() {
      gPrefService.setBoolPref("browser.altClickSave", true);
    },
    clean: function() {
      gPrefService.clearUserPref("browser.altClickSave"); 
    },
    event: { altKey: true },
    targets: [ "mathxlink", "svgxlink" ],
    expectedInvokedMethods: [ "saveURL" ],
    preventDefault: true,
  },

  {
    desc: "Panel click",
    setup: function() {},
    clean: function() {},
    event: {},
    targets: [ "panellink" ],
    expectedInvokedMethods: [ "urlSecurityCheck", "loadURI" ],
    preventDefault: true,
  },

  {
    desc: "Simple middle click opentab",
    setup: function() {},
    clean: function() {},
    event: { button: 1 },
    targets: [ "commonlink", "mathxlink", "svgxlink", "maplink" ],
    expectedInvokedMethods: [ "urlSecurityCheck", "openLinkIn" ],
    preventDefault: true,
  },

  {
    desc: "Simple middle click openwin",
    setup: function() {
      gPrefService.setBoolPref("browser.tabs.opentabfor.middleclick", false);
    },
    clean: function() {
      gPrefService.clearUserPref("browser.tabs.opentabfor.middleclick");
    },
    event: { button: 1 },
    targets: [ "commonlink", "mathxlink", "svgxlink", "maplink" ],
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
      gPrefService.clearUserPref("middlemouse.contentLoadURL");
      gPrefService.clearUserPref("general.autoScroll");
    },
    event: { button: 1 },
    targets: [ "emptylink" ],
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
  "getShortcutOrURIAndPostData",
];


let gTestWin = null;


let gInvokedMethods = [];


let gCurrentTest = null;

function test() {
  waitForExplicitFinish();

  gTestWin = openDialog(location, "", "chrome,all,dialog=no", "about:blank");
  whenDelayedStartupFinished(gTestWin, function () {
    info("Browser window opened");
    waitForFocus(function() {
      info("Browser window focused");
      waitForFocus(function() {
        info("Setting up browser...");
        setupTestBrowserWindow();
        info("Running tests...");
        executeSoon(runNextTest);
      }, gTestWin.content, true);
    }, gTestWin);
  });
}


let gClickHandler = {
  handleEvent: function (event) {
    let linkId = event.target.id || event.target.localName;
    is(event.type, "click",
       gCurrentTest.desc + ":Handler received a click event on " + linkId);

    let isPanelClick = linkId == "panellink";
    gTestWin.contentAreaClick(event, isPanelClick);
    let prevent = event.defaultPrevented;
    is(prevent, gCurrentTest.preventDefault,
       gCurrentTest.desc + ": event.defaultPrevented is correct (" + prevent + ")")

    
    gCurrentTest.expectedInvokedMethods.forEach(function(aExpectedMethodName) {
      isnot(gInvokedMethods.indexOf(aExpectedMethodName), -1,
            gCurrentTest.desc + ":" + aExpectedMethodName + " was invoked");
    });
    
    if (gInvokedMethods.length != gCurrentTest.expectedInvokedMethods.length) {
      ok(false, "Wrong number of invoked methods");
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
    
    return (aMethodName == "getShortcutOrURIAndPostData") ? arguments.url : arguments[0];
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
    '<p><a id="commonlink" href="http://mochi.test/moz/">Common link</a></p>' +
    '<p><a id="panellink" href="http://mochi.test/moz/">Panel link</a></p>' +
    '<p><a id="emptylink">Empty link</a></p>' +
    '<p><math id="mathxlink" xmlns="http://www.w3.org/1998/Math/MathML" xlink:type="simple" xlink:href="http://mochi.test/moz/"><mtext>MathML XLink</mtext></math></p>' +
    '<p><svg id="svgxlink" xmlns="http://www.w3.org/2000/svg" width="100px" height="50px" version="1.1"><a xlink:type="simple" xlink:href="http://mochi.test/moz/"><text transform="translate(10, 25)">SVG XLink</text></a></svg></p>' +
    '<p><map name="map" id="map"><area href="http://mochi.test/moz/" shape="rect" coords="0,0,128,128" /></map><img id="maplink" usemap="#map" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAIAAAACACAIAAABMXPacAAAABGdBTUEAALGPC%2FxhBQAAAOtJREFUeF7t0IEAAAAAgKD9qRcphAoDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGDAgAEDBgwYMGBgwIAAAT0N51AAAAAASUVORK5CYII%3D"/></p>'
  doc.body.appendChild(mainDiv);
}

function runNextTest() {
  if (!gCurrentTest) {
    gCurrentTest = gTests.shift();
    gCurrentTest.setup();
  }

  if (gCurrentTest.targets.length == 0) {
    info(gCurrentTest.desc + ": cleaning up...")
    gCurrentTest.clean();

    if (gTests.length > 0) {
      gCurrentTest = gTests.shift();
      gCurrentTest.setup();
    }
    else {
      finishTest();
      return;
    }
  }

  
  gInvokedMethods.length = 0;
  let target = gCurrentTest.targets.shift();

  info(gCurrentTest.desc + ": testing " + target);

  
  let targetElt = gTestWin.content.document.getElementById(target);
  ok(targetElt, gCurrentTest.desc + ": target is valid (" + targetElt.id + ")");
  EventUtils.synthesizeMouseAtCenter(targetElt, gCurrentTest.event, gTestWin.content);
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
