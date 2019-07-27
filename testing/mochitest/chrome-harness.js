






Components.utils.import("resource://gre/modules/NetUtil.jsm");








function getChromeURI(url) {
  var ios = Components.classes["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);
  return ios.newURI(url, null, null);
}








function getResolvedURI(url) {
  var chromeURI = getChromeURI(url);
  var resolvedURI = Components.classes["@mozilla.org/chrome/chrome-registry;1"].
                    getService(Components.interfaces.nsIChromeRegistry).
                    convertChromeURL(chromeURI);

  try {
    resolvedURI = resolvedURI.QueryInterface(Components.interfaces.nsIJARURI);
  } catch (ex) {} 

  return resolvedURI;
}










function getChromeDir(resolvedURI) {

  var fileHandler = Components.classes["@mozilla.org/network/protocol;1?name=file"].
                    getService(Components.interfaces.nsIFileProtocolHandler);
  var chromeDir = fileHandler.getFileFromURLSpec(resolvedURI.spec);
  return chromeDir.parent.QueryInterface(Components.interfaces.nsILocalFile);
}











function getMochitestJarListing(aBasePath, aTestPath, aDir)
{
  var zReader = Components.classes["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Components.interfaces.nsIZipReader);
  var fileHandler = Components.classes["@mozilla.org/network/protocol;1?name=file"].
                    getService(Components.interfaces.nsIFileProtocolHandler);

  var fileName = fileHandler.getFileFromURLSpec(getResolvedURI(aBasePath).JARFile.spec);
  zReader.open(fileName);
  
  var idx = aBasePath.indexOf('/content');
  var basePath = aBasePath.slice(0, idx);

  var base = "content/" + aDir + "/";

  if (aTestPath) {
    var extraPath = aTestPath;
    var pathToCheck = base + aTestPath;
    if (zReader.hasEntry(pathToCheck)) {
      var pathEntry = zReader.getEntry(pathToCheck);
      if (pathEntry.isDirectory) {
        base = pathToCheck;
      } else {
        var singleTestPath = basePath + '/' + base + aTestPath;
        var singleObject = {};
        singleObject[singleTestPath] = true;
        return singleObject;
      }
    }
    else if (zReader.hasEntry(pathToCheck + "/")) {
      base = pathToCheck + "/";
    }
    else {
      return null;
    }
  }
  var [links, count] = zList(base, zReader, basePath, true);
  return links;
}











function zList(base, zReader, baseJarName, recurse) {
  var dirs = zReader.findEntries(base + "*");
  var links = {};
  var count = 0;
  var fileArray = [];
  
  while(dirs.hasMore()) {
    var entryName = dirs.getNext();
    if (entryName.substr(-1) == '/' && entryName.split('/').length == (base.split('/').length + 1) ||
        (entryName.substr(-1) != '/' && entryName.split('/').length == (base.split('/').length))) { 
      fileArray.push(entryName);
    }
  }
  fileArray.sort();
  count = fileArray.length;
  for (var i=0; i < fileArray.length; i++) {
    var myFile = fileArray[i];
    if (myFile.substr(-1) === '/' && recurse) {
      var childCount = 0;
      [links[myFile], childCount] = zList(myFile, zReader, baseJarName, recurse);
      count += childCount;
    } else {
      if (myFile.indexOf("SimpleTest") == -1) {
        
        links[baseJarName + '/' + myFile] = true;
      }
    }
  }
  return [links, count];
}











function getFileListing(basePath, testPath, dir, srvScope)
{
  var uri = getResolvedURI(basePath);
  var chromeDir = getChromeDir(uri);
  chromeDir.appendRelativePath(dir);
  basePath += '/' + dir.replace(/\\/g, '/');

  if (testPath == "false" || testPath == false) {
    testPath = "";
  }
  testPath = testPath.replace(/\\\\/g, '\\').replace(/\\/g, '/');

  var ioSvc = Components.classes["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);
  var testsDirURI = ioSvc.newFileURI(chromeDir);
  var testsDir = ioSvc.newURI(testPath, null, testsDirURI)
                  .QueryInterface(Components.interfaces.nsIFileURL).file;

  if (testPath != undefined) {
    var extraPath = testPath;
    
    var fileNameRegexp = /(browser|test)_.+\.(xul|html|js)$/;

    
    if (!testsDir.exists())
      return null;

    if (testsDir.isFile()) {
      if (fileNameRegexp.test(testsDir.leafName)) {
        var singlePath = basePath + '/' + testPath;
        var links = {};
        links[singlePath] = true;
        return links;
      }
      
      return null;
    }

    
    basePath += "/" + testPath;
  }
  var [links, count] = srvScope.list(basePath, testsDir, true);
  return links;
}



function getRootDirectory(path, chromeURI) {
  if (chromeURI === undefined)
  {
    chromeURI = getChromeURI(path);
  }
  var myURL = chromeURI.QueryInterface(Components.interfaces.nsIURL);
  var mydir = myURL.directory;

  if (mydir.match('/$') != '/')
  {
    mydir += '/';
  }

  return chromeURI.prePath + mydir;
}


function getChromePrePath(path, chromeURI) {

  if (chromeURI === undefined) {
    chromeURI = getChromeURI(path);
  }

  return chromeURI.prePath;
}




function getJar(uri) {
  var resolvedURI = getResolvedURI(uri);
  var jar = null;
  try {
    if (resolvedURI.JARFile) {
      jar = resolvedURI;
    }
  } catch (ex) {}
  return jar;
}









function extractJarToTmp(jar) {
  var tmpdir = Components.classes["@mozilla.org/file/directory_service;1"]
                      .getService(Components.interfaces.nsIProperties)
                      .get("ProfD", Components.interfaces.nsILocalFile);
  tmpdir.append("mochikit.tmp");
  
  
  tmpdir.createUnique(Components.interfaces.nsIFile.DIRECTORY_TYPE, parseInt("0777", 8));

  var zReader = Components.classes["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Components.interfaces.nsIZipReader);

  var fileHandler = Components.classes["@mozilla.org/network/protocol;1?name=file"].
                    getService(Components.interfaces.nsIFileProtocolHandler);

  var fileName = fileHandler.getFileFromURLSpec(jar.JARFile.spec);
  zReader.open(fileName);

  
  var filepath = "";
  var parts = jar.JAREntry.split('/');
  for (var i =0; i < parts.length - 1; i++) {
    if (parts[i] != '') {
      filepath += parts[i] + '/';
    }
  }

  


  var dirs = zReader.findEntries(filepath + '*/');
  while (dirs.hasMore()) {
    var targetDir = buildRelativePath(dirs.getNext(), tmpdir, filepath);
    
    
    if (!targetDir.exists()) {
      targetDir.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, parseInt("0777", 8));
    }
  }

  
  var files = zReader.findEntries(filepath + "*");
  while (files.hasMore()) {
    var fname = files.getNext();
    if (fname.substr(-1) != '/') {
      var targetFile = buildRelativePath(fname, tmpdir, filepath);
      zReader.extract(fname, targetFile);
    }
  }
  return tmpdir;
}





function getTestFilePath(path) {
  if (path[0] == "/") {
    throw new Error("getTestFilePath only accepts relative path");
  }
  
  
  
  var baseURI = typeof(gTestPath) == "string" ? gTestPath : window.location.href;
  var parentURI = getResolvedURI(getRootDirectory(baseURI));
  var file;
  if (parentURI.JARFile) {
    
    file = extractJarToTmp(parentURI);
  } else {
    
    var fileHandler = Components.classes["@mozilla.org/network/protocol;1?name=file"].
                      getService(Components.interfaces.nsIFileProtocolHandler);
    file = fileHandler.getFileFromURLSpec(parentURI.spec);
  }
  
  path.split("/")
      .forEach(function (p) {
        if (p == "..") {
          file = file.parent;
        } else if (p != ".") {
          file.append(p);
        }
      });
  return file.path;
}





function buildRelativePath(jarentryname, destdir, basepath)
{
  var baseParts = basepath.split('/');
  if (baseParts[baseParts.length-1] == '') {
    baseParts.pop();
  }

  var parts = jarentryname.split('/');

  var targetFile = Components.classes["@mozilla.org/file/local;1"]
                   .createInstance(Components.interfaces.nsILocalFile);
  targetFile.initWithFile(destdir);

  for (var i = baseParts.length; i < parts.length; i++) {
    targetFile.append(parts[i]);
  }

  return targetFile;
}

function readConfig(filename) {
  filename = filename || "testConfig.js";

  var fileLocator = Components.classes["@mozilla.org/file/directory_service;1"].
                    getService(Components.interfaces.nsIProperties);
  var configFile = fileLocator.get("ProfD", Components.interfaces.nsIFile);
  configFile.append(filename);

  if (!configFile.exists())
    return {};

  var fileInStream = Components.classes["@mozilla.org/network/file-input-stream;1"].
                     createInstance(Components.interfaces.nsIFileInputStream);
  fileInStream.init(configFile, -1, 0, 0);

  var str = NetUtil.readInputStreamToString(fileInStream, fileInStream.available());
  fileInStream.close();
  return JSON.parse(str);
}

function registerTests() {
  var testsURI = Components.classes["@mozilla.org/file/directory_service;1"].
                 getService(Components.interfaces.nsIProperties).
                 get("ProfD", Components.interfaces.nsILocalFile);
  testsURI.append("tests.manifest");
  var ioSvc = Components.classes["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);
  var manifestFile = ioSvc.newFileURI(testsURI).
                     QueryInterface(Components.interfaces.nsIFileURL).file;

  Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar).
                     autoRegister(manifestFile);
}

function getTestList(params, callback) {
  registerTests();

  var baseurl = 'chrome://mochitests/content';
  if (window.parseQueryString) {
    params = parseQueryString(location.search.substring(1), true);
  }
  if (!params.baseurl) {
    params.baseurl = baseurl;
  }

  var config = readConfig();
  for (var p in params) {
    if (params[p] == 1) {
      config[p] = true;
    } else if (params[p] == 0) {
      config[p] = false;
    } else {
      config[p] = params[p];
    }
  }
  params = config;
  if (params.manifestFile) {
    getTestManifest("http://mochi.test:8888/" + params.manifestFile, params, callback);
    return;
  }

  var links = {};
  
  var scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                       getService(Ci.mozIJSSubScriptLoader);
  var srvScope = {};
  scriptLoader.loadSubScript('chrome://mochikit/content/server.js',
                             srvScope);

  if (getResolvedURI(baseurl).JARFile) {
    links = getMochitestJarListing(baseurl, params.testPath, params.testRoot);
  } else {
    links = getFileListing(baseurl, params.testPath, params.testRoot, srvScope);
  }
  callback(links);
}
