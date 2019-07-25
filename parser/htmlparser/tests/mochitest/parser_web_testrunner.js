





































 












var functionsToRunAsync = [];

window.addEventListener("message", function(event) {
  if (event.source == window && event.data == "async-run") {
    event.stopPropagation();
    var fn = functionsToRunAsync.shift();
    fn();
  }
}, true);

function asyncRun(fn) {
  functionsToRunAsync.push(fn);
  window.postMessage("async-run", "*");
}

function writeErrorSummary(input, expected, got, isTodo) {
  if (isTodo) {
    appendChildNodes($("display"), H2("Unexpected Success:"));
  } else {
    appendChildNodes($("display"), H2("Unexpected Failure:"));
  }
  appendChildNodes(
    $("display"), BR(),
    SPAN("Matched: "), "" + (expected == got),
    PRE("Input: \n" + input, "\n-\n",
        "Expected:\n" + expected, "\n-\n",
        "Output:\n" + got + "\n-\n"),
    HR()
  );
}






function makeTestChecker(input, expected, errors) {
  return function (e) {
    var domAsString = docToTestOutput(e.target.contentDocument);
    if (html5Exceptions[input]) {
      todo_is(domAsString, expected, "HTML5 expected success.");
      if (domAsString == expected) {
        writeErrorSummary(input, expected, domAsString, true);
      }
    } else {
      is(domAsString, expected, "HTML5 expected success.");
      if (domAsString != expected) {
        writeErrorSummary(input, expected, domAsString, false);
      }
    }
    nextTest(e.target);
  } 
}

function makeFragmentTestChecker(input, 
                                 expected, 
                                 errors, 
                                 fragment, 
                                 testframe) {
  return function () {
    var context = document.createElementNS("http://www.w3.org/1999/xhtml",
                                           fragment);
    context.innerHTML = input;
    var domAsString = fragmentToTestOutput(context);
    is(domAsString, expected, "HTML5 expected success. " + new Date());
    if (domAsString != expected) {
      writeErrorSummary(input, expected, domAsString, false);
    }
    nextTest(testframe);
  } 
}

var testcases;
function nextTest(testframe) {
  var test = 0;
  try {
    var [input, output, errors, fragment] = testcases.next();
    if (fragment) {
      asyncRun(makeFragmentTestChecker(input, 
                                       output, 
                                       errors, 
                                       fragment, 
                                       testframe));
    } else {
      dataURL = "data:text/html;charset=utf-8," + encodeURIComponent(input);
      testframe.onload = makeTestChecker(input, output, errors);
      testframe.src = dataURL;
    }
  } catch (err if err instanceof StopIteration) {
    SimpleTest.finish();
  }
}

var testFileContents = [];
function loadNextTestFile() {
  var datFile = parserDatFiles.shift();
  if (datFile) {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
      if (this.readyState == 4) {
        testFileContents.push(this.responseText);
        loadNextTestFile();
      }
    };
    xhr.open("GET", "html5lib_tree_construction/" + datFile);
    xhr.send();
  } else {
    testcases = test_parser(testFileContents);
    nextTest($("testframe"));
  }
}

addLoadEvent(loadNextTestFile);
SimpleTest.waitForExplicitFinish();
