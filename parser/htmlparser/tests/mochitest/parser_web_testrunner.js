




































 












function writeErrorSummary(input, expected, got, isTodo) {
  if (!isTodo) {
    appendChildNodes($("display"), H2("Unexpected Failure:"));
  }
  appendChildNodes(
    $("display"), BR(),
    SPAN("Matched: "), "" + (expected == got),
    P("Input: " + input),
    PRE("Expected:\n|" + expected +"|", "\n-\n",
        "Output:\n|" + got + "|\n\n"),
    HR()
  );
}






function makeTestChecker(input, expected, errors) {
  return function (e) {
    var domAsString = docToTestOutput(e.target.contentDocument);
    
    if (expected == domAsString) {
      is(domAsString, expected, "HTML5 expected success. " + new Date());
    } else {
      var reorderedDOM = reorderToMatchExpected(domAsString, expected);
      if (html5Exceptions[input]) {
        todo(reorderedDOM == expected, "HTML5 expected failure. " + new Date());
        writeErrorSummary(input, expected, reorderedDOM, true);
      } else {
        if (reorderedDOM != expected) {
          is(reorderedDOM, expected, "HTML5 unexpected failure. " + input + " " + new Date());
          writeErrorSummary(input, expected, reorderedDOM, false);
        } else {
          is(reorderedDOM, expected, "HTML5 expected success. " + new Date());
        }
      }
    }
    nextTest(e.target);
  } 
}

var testcases;
function nextTest(testframe) {
  var test = 0;
  try {
    var [input, output, errors] = testcases.next();
    dataURL = "data:text/html;base64," + btoa(input);
    testframe.onload = makeTestChecker(input, output, errors);
    testframe.src = dataURL;
  } catch (err if err instanceof StopIteration) {
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
  for each (var filename in parserDatFiles) {
    var datFrame = document.createElement("iframe");
    datFrame.onload = frameLoaded;
    datFrame.src = filename;
    $("display").appendChild(datFrame);
  }
  appendChildNodes($("display"), BR(), "Results: ", HR());
}

addLoadEvent(makeIFrames);
SimpleTest.waitForExplicitFinish();
