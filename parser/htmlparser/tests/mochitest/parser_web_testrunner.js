




































 












function writeErrorSummary(input, expected, got, isTodo, description,
  expectedTokenizerOutput) {
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
    PRE("Input: " + input)
  );
  if (typeof(expectedTokenizerOutput) != "undefined") {
    appendChildNodes(
      $("display"), P("Expected raw tokenizer output: " +
        expectedTokenizerOutput)
    );
  }
  let expectedTitle = "Expected:";
  let outputTitle = "Output:";
  if (gJSCompatibilityMode) {
    outputTitle = "Gecko parser output:";
    expectedTitle = "JavaScript parser output:";
  }
  appendChildNodes(
    $("display"),
    PRE(expectedTitle + "\n|" + expected +"|", "\n-\n",
        outputTitle + "\n|" + got + "|\n\n"),
    HR()
  );
}

function checkTests(input, expected, errors, description, 
  expectedTokenizerOutput, testDocument) {
  
  
  if (expected.length > 1) {    
    var domAsString = docToTestOutput(testDocument);
    
    if (expected == domAsString) {
      is(domAsString, expected, "HTML5 expected success. " + new Date());
    } else {
      var reorderedDOM = reorderToMatchExpected(domAsString, expected);
      if (!gJSCompatibilityMode && html5Exceptions[input]) {
        todo(reorderedDOM == expected, "HTML5 expected failure. " + new Date());
        writeErrorSummary(input, expected, reorderedDOM, true, description,
          expectedTokenizerOutput);
      } else {
        if (reorderedDOM != expected) {
          is(reorderedDOM, expected, "HTML5 unexpected failure. " + input + " " + new Date());
          writeErrorSummary(input, expected, reorderedDOM, false,
            description, expectedTokenizerOutput);
        } else {
          is(reorderedDOM, expected, "HTML5 expected success. " + new Date());
        }
      }
    }
  }    
}






function makeTestChecker(input, expected, errors, description, 
  expectedTokenizerOutput) {
  return function (e) {
    if (!gJSCompatibilityMode) {
      checkTests(input, expected, errors, description, 
        expectedTokenizerOutput, e.target.contentDocument);
      nextTest(e.target);
    }
    else {
      window.parseHtmlDocument(input, $("jsframe").contentDocument,
        function() {
          expected = docToTestOutput($("jsframe").contentDocument);
          checkTests(input, expected, errors, description, 
            expectedTokenizerOutput, e.target.contentDocument);
          nextTest(e.target);        
        }, null);      
    }
  }
}

var testcases;
function nextTest(testframe) {
  var test = 0;
  try {
    if (gTokenizerMode) {
      
      
      
      var [index, input, output, errors, description, expectedTokenizerOutput]
        = testcases.next();
      dataURL = "tokenizer_file_server.sjs?" + index + 
        "&" + gTokenizerTestFile;
    } else {
      var [input, output, errors, description, expectedTokenizerOutput] = 
        testcases.next();
      dataURL = "data:text/html;base64," + btoa(input);
    }
    testframe.onload = makeTestChecker(input, output, errors, description,
      expectedTokenizerOutput);
    testframe.src = dataURL;
  } catch (err if err instanceof StopIteration) {
    
    if (typeof(gOriginalHtml5Pref) == "boolean") {
      netscape.security.PrivilegeManager
                  .enablePrivilege("UniversalXPConnect");
      var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                  .getService(Components.interfaces.nsIPrefBranch);
      prefs.setBoolPref("html5.enable", gOriginalHtml5Pref);
    }
    SimpleTest.finish();
  }
}

var framesLoaded = [];
function frameLoaded(e) {
  framesLoaded.push(e.target);
  if (framesLoaded.length == parserDatFiles.length) {
    var tests = [scrapeText(ifr.contentDocument)
                 for each (ifr in framesLoaded)];
    testcases = test_parser(tests);    
    nextTest($("testframe"));

    
  }
}




function makeIFrames() {
  
  
  gJSCompatibilityMode = typeof(window.parseHtmlDocument) != "undefined";

  if (gTokenizerMode) {
    
    appendChildNodes($("display"), BR(), "Results: ", HR());
    testcases = test_parser(tokenizerTests["tests"]);    
    nextTest($("testframe"));
  }
  else {
    for each (var filename in parserDatFiles) {
      var datFrame = document.createElement("iframe");
      datFrame.onload = frameLoaded;
      datFrame.src = filename;
      $("display").appendChild(datFrame);
    }
    appendChildNodes($("display"), BR(), "Results: ", HR());
  }
}



if (typeof(gTokenizerMode) == "undefined") {
  gTokenizerMode = false;
}



var gJSCompatibilityMode;

addLoadEvent(makeIFrames);
SimpleTest.waitForExplicitFinish();
