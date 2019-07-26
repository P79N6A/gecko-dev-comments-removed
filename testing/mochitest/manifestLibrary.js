





function parseTestManifest(testManifest, params, callback) {
  var links = {};
  var paths = [];

  
  if ("runtests" in testManifest || "excludetests" in testManifest) {
    callback(testManifest);
    return;
  }

  
  
  
  for (var obj of testManifest['tests']) {
    var path = obj['path'];
    if (obj.disabled) {
      dump("TEST-SKIPPED | " + path + " | " + obj.disabled + "\n");
      continue;
    }
    if (params.testRoot != 'tests' && params.testRoot !== undefined) {
      links[params.baseurl + '/' + params.testRoot + '/' + path] = true
    } else {
      paths.push(params.testPrefix + path);
    }
  }
  if (paths.length > 0) {
    callback(paths);
  } else {
    callback(links);
  }
}

function getTestManifest(url, params, callback) {
  var req = new XMLHttpRequest();
  req.open("GET", url);
  req.onload = function() {
    if (req.readyState == 4) {
      if (req.status == 200) {
        parseTestManifest(JSON.parse(req.responseText), params, callback);
      } else {
        dump("TEST-ERROR: setup.js | error loading " + url + "\n");
        callback({});
      }
    }
  }
  req.send();
}













function filterTests(filter, testList, runOnly) {

  var filteredTests = [];
  var removedTests = [];
  var runtests = {};
  var excludetests = {};

  if (filter == null) {
    return testList;
  }

  if ('runtests' in filter) {
    runtests = filter.runtests;
  }
  if ('excludetests' in filter) {
    excludetests = filter.excludetests;
  }
  if (!('runtests' in filter) && !('excludetests' in filter)) {
    if (runOnly == 'true') {
      runtests = filter;
    } else {
      excludetests = filter;
    }
  }

  var testRoot = config.testRoot || "tests";
  
  
  if (Object.keys(runtests).length) {
    for (var i = 0; i < testList.length; i++) {
      var testpath = testList[i];
      var tmppath = testpath.replace(/^\//, '');
      for (var f in runtests) {
        
        file = f.replace(/^\//, '')
        file = file.replace(/^tests\//, '')

        
        if (tmppath.match(testRoot + "/" + file) != null) {
          filteredTests.push(testpath);
          break;
        }
      }
    }
  } else {
    filteredTests = testList.slice(0);
  }

  
  
  if (!Object.keys(excludetests).length) {
    return filteredTests;
  }

  var refilteredTests = [];
  for (var i = 0; i < filteredTests.length; i++) {
    var found = false;
    var testpath = filteredTests[i];
    var tmppath = testpath.replace(/^\//, '');
    for (var f in excludetests) {
      
      file = f.replace(/^\//, '')
      file = file.replace(/^tests\//, '')

      
      if (tmppath.match(testRoot + "/" + file) != null) {
        found = true;
        break;
      }
    }
    if (!found) {
      refilteredTests.push(testpath);
    }
  }
  return refilteredTests;
}

