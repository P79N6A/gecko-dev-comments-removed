









const TESTS_PATH = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/";
const TESTS = [
  { 
    file: "test-bug-595934-css-loader.html",
    category: "CSS Loader",
    matchString: "text/css",
  },
  { 
    file: "test-bug-595934-dom-events.html",
    category: "DOM Events",
    matchString: "preventBubble()",
  },
  { 
    file: "test-bug-595934-dom-html.html",
    category: "DOM:HTML",
    matchString: "getElementById",
  },
  { 
    file: "test-bug-595934-imagemap.html",
    category: "ImageMap",
    matchString: "shape=\"rect\"",
  },
  { 
    file: "test-bug-595934-html.html",
    category: "HTML",
    matchString: "multipart/form-data",
    onload: function() {
      let form = content.document.querySelector("form");
      form.submit();
    },
  },
  { 
    file: "test-bug-595934-malformedxml.xhtml",
    category: "malformed-xml",
    matchString: "no element found",
  },
  { 
    file: "test-bug-595934-svg.xhtml",
    category: "SVG",
    matchString: "fooBarSVG",
  },
  { 
    file: "test-bug-595934-workers.html",
    category: "DOM Worker javascript",
    matchString: "fooBarWorker",
  },
  { 
    file: "test-bug-595934-dom-html-external.html",
    category: "DOM:HTML",
    matchString: "document.all",
  },
  { 
    file: "test-bug-595934-dom-events-external.html",
    category: "DOM Events",
    matchString: "clientWidth",
  },
  { 
    file: "test-bug-595934-dom-events-external2.html",
    category: "DOM Events",
    matchString: "preventBubble()",
  },
  { 
    file: "test-bug-595934-canvas.html",
    category: "Canvas",
    matchString: "strokeStyle",
  },
  { 
    file: "test-bug-595934-css-parser.html",
    category: "CSS Parser",
    matchString: "foobarCssParser",
  },
  { 
    file: "test-bug-595934-malformedxml-external.html",
    category: "malformed-xml",
    matchString: "</html>",
  },
  { 
    file: "test-bug-595934-empty-getelementbyid.html",
    category: "DOM",
    matchString: "getElementById",
  },
  { 
    file: "test-bug-595934-canvas-css.html",
    category: "CSS Parser",
    matchString: "foobarCanvasCssParser",
  },
  { 
    file: "test-bug-595934-getselection.html",
    category: "content javascript",
    matchString: "getSelection",
  },
  { 
    file: "test-bug-595934-image.html",
    category: "Image",
    matchString: "corrupt",
  },
];

let pos = -1;

let TestObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  observe: function test_observe(aSubject)
  {
    if (!(aSubject instanceof Ci.nsIScriptError)) {
      return;
    }

    is(aSubject.category, TESTS[pos].category,
      "test #" + pos + ": error category '" + TESTS[pos].category + "'");

    if (aSubject.category == TESTS[pos].category) {
      executeSoon(performTest);
    }
    else {
      testEnd();
    }
  }
};

function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();

  let hudId = HUDService.getHudIdByWindow(content);
  hud = HUDService.hudReferences[hudId];

  Services.console.registerListener(TestObserver);

  executeSoon(testNext);
}

function testNext() {
  hud.jsterm.clearOutput();

  pos++;
  if (pos < TESTS.length) {
    if (TESTS[pos].onload) {
      browser.addEventListener("load", function(aEvent) {
        browser.removeEventListener(aEvent.type, arguments.callee, true);
        TESTS[pos].onload(aEvent);
      }, true);
    }

    content.location = TESTS_PATH + TESTS[pos].file;
  }
  else {
    testEnd();
  }
}

function testEnd() {
  Services.console.unregisterListener(TestObserver);
  finishTest();
}

function performTest() {
  let textContent = hud.outputNode.textContent;
  isnot(textContent.indexOf(TESTS[pos].matchString), -1,
    "test #" + pos + ": message found '" + TESTS[pos].matchString + "'");

  testNext();
}

function test() {
  addTab("data:text/html,Web Console test for bug 595934 - message categories coverage.");
  browser.addEventListener("load", tabLoad, true);
}

