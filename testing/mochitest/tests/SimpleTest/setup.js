





































TestRunner.logEnabled = true;
TestRunner.logger = new Logger();


var params = parseQueryString(location.search.substring(1), true);


var fileLevel =  params.fileLevel || null;
var consoleLevel = params.consoleLevel || null;


if (params.closeWhenDone) {
  TestRunner.onComplete = goQuitApplication;
}


if (params.logFile) {
  MozillaFileLogger.init(params.logFile);
  TestRunner.logger.addListener("mozLogger", fileLevel + "", MozillaFileLogger.getLogCallback());
}


if (!params.quiet) {
  function dumpListener(msg) {
    dump(msg.num + " " + msg.level + " " + msg.info.join(' ') + "\n");
  }
  TestRunner.logger.addListener("dumpListener", consoleLevel + "", dumpListener);
}

var gTestList = [];
var RunSet = {}
RunSet.runall = function(e) {
  TestRunner.runTests(gTestList);
}
RunSet.reloadAndRunAll = function(e) {
  e.preventDefault();
  
  var addParam = "";
  if (params.autorun) {
    window.location.search += "";
    window.location.href = window.location.href;
  } else if (window.location.search) {
    window.location.href += "&autorun=1";
  } else {
    window.location.href += "?autorun=1";
  }
  
};


function toggleVisible(elem) {
    toggleElementClass("invisible", elem);
}

function makeVisible(elem) {
    removeElementClass(elem, "invisible");
}

function makeInvisible(elem) {
    addElementClass(elem, "invisible");
}

function isVisible(elem) {
    
    
    return !hasElementClass(elem, "invisible");
};

function toggleNonTests (e) {
  e.preventDefault();
  var elems = getElementsByTagAndClassName("*", "non-test");
  for (var i="0"; i<elems.length; i++) {
    toggleVisible(elems[i]);
  }
  if (isVisible(elems[0])) {
    $("toggleNonTests").innerHTML = "Hide Non-Tests";
  } else {
    $("toggleNonTests").innerHTML = "Show Non-Tests";
  }
}


function hookup() {
  connect("runtests", "onclick", RunSet, "reloadAndRunAll");
  connect("toggleNonTests", "onclick", toggleNonTests);
  
  if (params.autorun) {
    RunSet.runall();
  }
}
