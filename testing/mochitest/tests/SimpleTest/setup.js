





































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
}


if (params.timeout) {
  TestRunner.timeout = parseInt(params.timeout) * 1000;
}


var fileLevel =  params.fileLevel || null;
var consoleLevel = params.consoleLevel || null;


if (params.repeat) {
  TestRunner.repeat = params.repeat;
} 


if (params.closeWhenDone) {
  TestRunner.onComplete = goQuitApplication;
}


if (params.logFile) {
  var spl = new SpecialPowersLogger(params.logFile);
  TestRunner.logger.addListener("mozLogger", fileLevel + "", spl.getLogCallback());
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
  
  
  gTestList = filterTests(params.runOnlyTests, params.excludeTests);

  
  var my_tests = gTestList;

  if (params.totalChunks && params.thisChunk) {
    var total_chunks = parseInt(params.totalChunks);
    
    var this_chunk = parseInt(params.thisChunk);

    
    
    if (params.chunkByDir) {
      var chunkByDir = parseInt(params.chunkByDir);
      var tests_by_dir = {};
      var test_dirs = []
      for (var i = 0; i < gTestList.length; ++i) {
        var test_path = gTestList[i];
        if (test_path[0] == '/') {
          test_path = test_path.substr(1);
        }
        var dir = test_path.split("/");
        
        
        
        
        dir = dir.slice(0, Math.min(chunkByDir+1, dir.length-1));
        
        dir = dir.join("/");
        if (!(dir in tests_by_dir)) {
          tests_by_dir[dir] = [gTestList[i]];
          test_dirs.push(dir);
        } else {
          tests_by_dir[dir].push(gTestList[i]);
        }
      }
      var tests_per_chunk = test_dirs.length / total_chunks;
      var start = Math.round((this_chunk-1) * tests_per_chunk);
      var end = Math.round(this_chunk * tests_per_chunk);
      my_tests = [];
      var dirs = []
      for (var i = start; i < end; ++i) {
        var dir = test_dirs[i];
        dirs.push(dir);
        my_tests = my_tests.concat(tests_by_dir[dir]);
      }
      TestRunner.logger.log("Running tests in " + dirs.join(", "));
    } else {
      var tests_per_chunk = gTestList.length / total_chunks;
      var start = Math.round((this_chunk-1) * tests_per_chunk);
      var end = Math.round(this_chunk * tests_per_chunk);
      my_tests = gTestList.slice(start, end);
      TestRunner.logger.log("Running tests " + (start+1) + "-" + end + "/" + gTestList.length);
    }
  }
  if (params.shuffle) {
    for (var i = my_tests.length-1; i > 0; --i) {
      var j = Math.floor(Math.random() * i);
      var tmp = my_tests[j];
      my_tests[j] = my_tests[i];
      my_tests[i] = tmp;
    }
  }
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





function filterTests(runOnly, exclude) {
  var filteredTests = [];
  var filterFile = null;

  if (runOnly) {
    filterFile = runOnly;
  } else if (exclude) {
    filterFile = exclude;
  }

  if (filterFile == null)
    return gTestList;

  var datafile = "http://mochi.test:8888/" + filterFile;
  var objXml = new XMLHttpRequest();
  objXml.open("GET",datafile,false);
  objXml.send(null);
  try {
    var filter = JSON.parse(objXml.responseText);
  } catch (ex) {
    dump("INFO | setup.js | error loading or parsing '" + datafile + "'\n");
    return gTestList;
  }
  
  for (var i = 0; i < gTestList.length; ++i) {
    var test_path = gTestList[i];
    
    
    var tmp_path = test_path.replace(/^\//, '');

    var found = false;

    for (var f in filter) {
      
      file = f.replace(/^\//, '')
      file = file.replace(/^tests\//, '')
      
      
      if (tmp_path.match("^tests/" + file) != null) {
        if (runOnly)
          filteredTests.push(test_path);
        found = true;
        break;
      }
    }

    if (exclude && !found)
      filteredTests.push(test_path);
  }

  return filteredTests;
}


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
  var elems = document.getElementsClassName("non-test");
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
  document.getElementById('runtests').onclick = RunSet.reloadAndRunAll;
  document.getElementById('toggleNonTests').onclick = toggleNonTests; 
  
  if (params.autorun) {
    RunSet.runall();
  }
}
