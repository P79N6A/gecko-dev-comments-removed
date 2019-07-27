





TestRunner.logEnabled = true;
TestRunner.logger = LogController;


parseQueryString = function(encodedString, useArrays) {
  
  var qstr = (encodedString[0] == "?") ? encodedString.substring(1) :
                                         encodedString;
  var pairs = qstr.replace(/\+/g, "%20").split(/(\&amp\;|\&\#38\;|\&#x26;|\&)/);
  var o = {};
  var decode;
  if (typeof(decodeURIComponent) != "undefined") {
    decode = decodeURIComponent;
  } else {
    decode = unescape;
  }
  if (useArrays) {
    for (var i = 0; i < pairs.length; i++) {
      var pair = pairs[i].split("=");
      if (pair.length !== 2) {
        continue;
      }
      var name = decode(pair[0]);
      var arr = o[name];
      if (!(arr instanceof Array)) {
        arr = [];
        o[name] = arr;
      }
      arr.push(decode(pair[1]));
    }
  } else {
    for (i = 0; i < pairs.length; i++) {
      pair = pairs[i].split("=");
      if (pair.length !== 2) {
        continue;
      }
      o[decode(pair[0])] = decode(pair[1]);
    }
  }
  return o;
};


var params = parseQueryString(location.search.substring(1), true);

var config = {};
if (window.readConfig) {
  config = readConfig();
}

if (config.testRoot == "chrome" || config.testRoot == "a11y") {
  for (p in params) {
    if (params[p] == 1) {
      config[p] = true;
    } else if (params[p] == 0) {
      config[p] = false;
    } else {
      config[p] = params[p];
    }
  }
  params = config;
  params.baseurl = "chrome://mochitests/content";
} else {
  params.baseurl = "";
}

if (params.testRoot == "browser") {
  params.testPrefix = "chrome://mochitests/content/browser/";
} else if (params.testRoot == "chrome") {
  params.testPrefix = "chrome://mochitests/content/chrome/";
} else if (params.testRoot == "a11y") {
  params.testPrefix = "chrome://mochitests/content/a11y/";
} else {
  params.testPrefix = "/tests/";
}


if (params.timeout) {
  TestRunner.timeout = parseInt(params.timeout) * 1000;
}


var fileLevel =  params.fileLevel || null;
var consoleLevel = params.consoleLevel || null;


if (params.repeat) {
  TestRunner.repeat = params.repeat;
}

if (params.runUntilFailure) {
  TestRunner.runUntilFailure = true;
}


if (params.closeWhenDone) {
  TestRunner.onComplete = SpecialPowers.quit;
}

if (params.failureFile) {
  TestRunner.setFailureFile(params.failureFile);
}


if (params.debugOnFailure) {
  TestRunner.debugOnFailure = true;
}


if (params.logFile) {
  var spl = new SpecialPowersLogger(params.logFile);
  TestRunner.logger.addListener("mozLogger", fileLevel + "", spl.getLogCallback());
}


if (params.runSlower) {
  TestRunner.runSlower = true;
}

if (params.dumpOutputDirectory) {
  TestRunner.dumpOutputDirectory = params.dumpOutputDirectory;
}

if (params.dumpAboutMemoryAfterTest) {
  TestRunner.dumpAboutMemoryAfterTest = true;
}

if (params.dumpDMDAfterTest) {
  TestRunner.dumpDMDAfterTest = true;
}

if (params.interactiveDebugger) {
  TestRunner.structuredLogger.interactiveDebugger = true;
}


TestRunner.logger.addListener("dumpListener", consoleLevel + "", function(msg) {
  dump(msg.info.join(' ') + "\n");
});

var gTestList = [];
var RunSet = {};
RunSet.runall = function(e) {
  
  
  if (params.testManifest) {
    getTestManifest("http://mochi.test:8888/" + params.testManifest, params, function(filter) { gTestList = filterTests(filter, gTestList, params.runOnly); RunSet.runtests(); });
  } else {
    RunSet.runtests();
  }
}

RunSet.runtests = function(e) {
  
  var my_tests = gTestList;

  if (params.startAt || params.endAt) {
    my_tests = skipTests(my_tests, params.startAt, params.endAt);
  }

  if (params.totalChunks && params.thisChunk) {
    my_tests = chunkifyTests(my_tests, params.totalChunks, params.thisChunk, params.chunkByDir, TestRunner.logger);
  }

  if (params.shuffle) {
    for (var i = my_tests.length-1; i > 0; --i) {
      var j = Math.floor(Math.random() * i);
      var tmp = my_tests[j];
      my_tests[j] = my_tests[i];
      my_tests[i] = tmp;
    }
  }
  TestRunner.setParameterInfo(params);
  TestRunner.runTests(my_tests);
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
  var elems = document.getElementsByClassName("non-test");
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
  if (params.manifestFile) {
    getTestManifest("http://mochi.test:8888/" + params.manifestFile, params, hookupTests);
  } else {
    hookupTests(gTestList);
  }
}

function hookupTests(testList) {
  if (testList.length > 0) {
    gTestList = testList;
  } else {
    gTestList = [];
    for (var obj in testList) {
        gTestList.push(obj);
    }
  }

  document.getElementById('runtests').onclick = RunSet.reloadAndRunAll;
  document.getElementById('toggleNonTests').onclick = toggleNonTests;
  
  if (params.autorun) {
    RunSet.runall();
  }
}
