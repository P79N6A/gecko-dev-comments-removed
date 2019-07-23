





































 
const MODE_PARSER = 1;
const MODE_TOKENIZER = 2;
const MODE_JSCOMPARE = 3;


























function ParserTestRunner() {
  this.mode = MODE_PARSER;
}





ParserTestRunner.prototype.writeErrorSummary = function (input, 
  expected, got, isTodo, description, expectedTokenizerOutput) {
  if (!isTodo) {
    appendChildNodes($("display"), H2("Unexpected Failure:"));
  }
  appendChildNodes(
    $("display"), BR(),
    SPAN("Matched: "), "" + (expected == got)
  );
  if (typeof(description) != "undefined") {
    appendChildNodes(
      $("display"), P("Description: " + description)
    );
  }
  appendChildNodes(
    $("display"), 
    PRE("Input: " + JSON.stringify(input))
  );
  if (typeof(expectedTokenizerOutput) != "undefined") {
    appendChildNodes(
      $("display"), P("Expected raw tokenizer output: " +
        expectedTokenizerOutput)
    );
  }
  let expectedTitle = this.mode == MODE_JSCOMPARE ? 
    "JavaScript parser output:" : "Expected:";
  let outputTitle = this.mode == MODE_JSCOMPARE ? 
    "Gecko parser output:" : "Output:";
  appendChildNodes(
    $("display"),
    PRE(expectedTitle + "\n|" + expected +"|", "\n-\n",
        outputTitle + "\n|" + got + "|\n\n"),
    HR()
  );
}





ParserTestRunner.prototype.finishTest = function() {
  if (typeof(this.originalHtml5Pref) == "boolean") {
    netscape.security.PrivilegeManager
                .enablePrivilege("UniversalXPConnect");
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                .getService(Components.interfaces.nsIPrefBranch);
    prefs.setBoolPref("html5.enable", this.originalHtml5Pref);
  }
  SimpleTest.finish();
}





ParserTestRunner.prototype.enableHTML5Parser = function() {
  netscape.security.PrivilegeManager.
           enablePrivilege("UniversalXPConnect");
  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
           .getService(Components.interfaces.nsIPrefBranch);
  this.originalHtml5Pref = prefs.getBoolPref("html5.enable");
  prefs.setBoolPref("html5.enable", true);
}







ParserTestRunner.prototype.inTodoList = function(input) {
  return html5TreeConstructionExceptions[input];
}























ParserTestRunner.prototype.checkTests = function (input, expected, 
  errors, description, expectedTokenizerOutput, testDocument) {
  
  
  if (expected.length > 1) {
    var domAsString = docToTestOutput(testDocument, this.mode);
    if (expected == domAsString) {
      is(domAsString, expected, "HTML5 expected success. " + new Date());
    } else {
      if (this.inTodoList(input)) {
        todo(domAsString == expected, 
          "HTML5 expected failure. " + new Date());
        this.writeErrorSummary(input, expected, domAsString, 
          true, description, expectedTokenizerOutput);
      } else {
        if (domAsString != expected) {
          is(domAsString, expected, 
            "HTML5 unexpected failure. " + input + " " + new Date());
          this.writeErrorSummary(input, expected, domAsString, false,
            description, expectedTokenizerOutput);
        } else {
          is(domAsString, expected, "HTML5 expected success. " + new Date());
        }
      }
    }
  }
}








ParserTestRunner.prototype.makeTestChecker = function (input, 
  expected, errors, description, expectedTokenizerOutput,
  testframe) {
  this.checkTests(input, expected, errors, description, 
    expectedTokenizerOutput, testframe.contentDocument);
  this.nextTest(testframe);
}








ParserTestRunner.prototype.parseTests = function(testlist) {
  for each (var testgroup in testlist) {
    var tests = testgroup.split("#data\n");
    tests = ["#data\n" + test for each(test in tests) if (test)];
    for each (var test in tests) {
      yield parseTestcase(test);
    }
  }
}







ParserTestRunner.prototype.nextTest = function(testframe) {
  try {
    var [input, output, errors, description, expectedTokenizerOutput] = 
      this.testcases.next();
    dataURL = "data:text/html;base64," + btoa(input);
    var me = this;
    testframe.onload = function(e) {
      me.makeTestChecker.call(me, input, output, errors, 
        description, expectedTokenizerOutput, e.target);
    };
    testframe.src = dataURL;
  } catch (err if err instanceof StopIteration) {
    this.finishTest();
  }
}







ParserTestRunner.prototype.makeIFrames = function () {
  var framesLoaded = [];
  var me = this;
  for each (var filename in parserDatFiles) {
    var datFrame = document.createElement("iframe");
    datFrame.addEventListener("load", function(e) {
      framesLoaded.push(e.target);
      if (framesLoaded.length == parserDatFiles.length) {
        var tests = [scrapeText(ifr.contentDocument)
                     for each (ifr in framesLoaded)];
        me.testcases = me.parseTests(tests);    
        me.nextTest($("testframe"));
      }
    }, false);
    datFrame.src = filename;
    $("display").appendChild(datFrame);
  }
  appendChildNodes($("display"), BR(), "Results: ", HR());
}




ParserTestRunner.prototype.startTest = function () {
  SimpleTest.waitForExplicitFinish();
  this.enableHTML5Parser();
  var me = this;
  var onLoadHandler = function (e) { me.makeIFrames.apply(me); }
  addLoadEvent(onLoadHandler);
};




function TokenizerTestRunner(filename) {
  this.mode = MODE_TOKENIZER;
  this.filename = filename;
}
TokenizerTestRunner.prototype = new ParserTestRunner();
TokenizerTestRunner.prototype.constructor = TokenizerTestRunner; 







TokenizerTestRunner.prototype.inTodoList = function(input) {
  return html5TokenizerExceptions[input];
}













TokenizerTestRunner.prototype.nextTest = function(testframe) {
  try {
    var [index, input, expected, errors, description, expectedTokenizerOutput]
      = this.testcases.next();
    let dataURL = "tokenizer_file_server.sjs?" + index + 
      "&" + this.filename;
    var me = this;
    testframe.onload = function(e) {
      me.makeTestChecker.call(me, input, expected, errors, 
        description, expectedTokenizerOutput, e.target);
    };
    testframe.src = dataURL;
  } catch (err if err instanceof StopIteration) {
    this.finishTest();
  }
}








TokenizerTestRunner.prototype.parseTests = function(jsonTestList) {
  var index = 1;
  for each (var test in jsonTestList) {
    var tmpArray = [index];
    yield tmpArray.concat(parseJsonTestcase(test));
    index++;
  }
}





TokenizerTestRunner.prototype.startTestParser = function() {
  appendChildNodes($("display"), BR(), "Results: ", HR());
  this.testcases = this.parseTests(tokenizerTests["tests"]);    
  this.nextTest($("testframe"));
}




TokenizerTestRunner.prototype.startTest = function () {
  SimpleTest.waitForExplicitFinish();
  this.enableHTML5Parser();
  var me = this;
  var onLoadHandler = function (e) { me.startTestParser.apply(me); }
  addLoadEvent(onLoadHandler);
}







function JSCompareTestRunner() {
  this.mode = MODE_JSCOMPARE;
}
JSCompareTestRunner.prototype = new ParserTestRunner();
JSCompareTestRunner.prototype.constructor = JSCompareTestRunner;








JSCompareTestRunner.prototype.makeTestChecker = function (input, 
  expected, errors, description, expectedTokenizerOutput,
  testframe) {
  var me = this;
  window.parseHtmlDocument(input, $("jsframe").contentDocument,
    function() {
      expected = docToTestOutput($("jsframe").contentDocument,
        me.mode);
      me.checkTests(input, expected, errors, description, 
        expectedTokenizerOutput, testframe.contentDocument);
      me.nextTest(testframe);
    }, null);
}







JSCompareTestRunner.prototype.inTodoList = function(input) {
  return html5JSCompareExceptions[input];
}
