





































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
  if (params.totalChunks && params.thisChunk) {
    var total_chunks = parseInt(params.totalChunks);
    
    var this_chunk = parseInt(params.thisChunk);

    
    var my_tests = gTestList;

    
    
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
