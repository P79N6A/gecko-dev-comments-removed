









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

let foundCategory = false;
let foundText = false;
let output = null;
let jsterm = null;

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
      foundCategory = true;
      if (foundText) {
        executeSoon(testNext);
      }
    }
    else {
      ok(false, aSubject.sourceName + ':' + aSubject.lineNumber + '; ' +
                aSubject.errorMessage);
      executeSoon(finish);
    }
  }
};

function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();

  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.hudReferences[hudId];
  output = hud.outputNode;
  output.addEventListener("DOMNodeInserted", onDOMNodeInserted, false);
  jsterm = hud.jsterm;

  Services.console.registerListener(TestObserver);

  executeSoon(testNext);
}

function testNext() {
  jsterm.clearOutput();
  foundCategory = false;
  foundText = false;

  pos++;
  if (pos < TESTS.length) {
    if (TESTS[pos].onload) {
      let position = pos;
      browser.addEventListener("load", function(aEvent) {
        browser.removeEventListener(aEvent.type, arguments.callee, true);
        TESTS[position].onload(aEvent);
      }, true);
    }

    content.location = TESTS_PATH + TESTS[pos].file;
  }
  else {
    executeSoon(finish);
  }
}

function testEnd() {
  Services.console.unregisterListener(TestObserver);
  output.removeEventListener("DOMNodeInserted", onDOMNodeInserted, false);
  output = jsterm = null;
  finishTest();
}

function onDOMNodeInserted(aEvent) {
  let textContent = output.textContent;
  foundText = textContent.indexOf(TESTS[pos].matchString) > -1;
  if (foundText) {
    ok(foundText, "test #" + pos + ": message found '" + TESTS[pos].matchString + "'");
  }

  if (foundCategory) {
    executeSoon(testNext);
  }
}

function test() {
  registerCleanupFunction(testEnd);

  addTab("data:text/html,Web Console test for bug 595934 - message categories coverage.");
  browser.addEventListener("load", tabLoad, true);
}

