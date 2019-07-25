














































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

  var singleTestPath;
  if (aTestPath) {
    var extraPath = aTestPath;
    var pathToCheck = base + aTestPath;
    if (zReader.hasEntry(pathToCheck)) {
      var pathEntry = zReader.getEntry(pathToCheck);
      if (pathEntry.isDirectory) {
        base = pathToCheck;
      } else {
        singleTestPath = basePath + '/' + base + aTestPath;
        var singleObject = {};
        singleObject[singleTestPath] = true;
        return [singleObject, singleTestPath];
      }
    }
    else if (zReader.hasEntry(pathToCheck + "/")) {
      base = pathToCheck + "/";
    }
  }
  var [links, count] = zList(base, zReader, basePath, true);
  return [links, null];
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
  basePath += '/' + dir;

  var ioSvc = Components.classes["@mozilla.org/network/io-service;1"].
              getService(Components.interfaces.nsIIOService);
  var testsDirURI = ioSvc.newFileURI(chromeDir);
  var testsDir = ioSvc.newURI(testPath, null, testsDirURI)
                  .QueryInterface(Components.interfaces.nsIFileURL).file;

  var singleTestPath;
  if (testPath != undefined) {
    var extraPath = testPath;
    
    var fileNameRegexp = /(browser|test)_.+\.(xul|html|js)$/;

    
    if (!testsDir.exists())
      return [];

    
    if (testsDir.isFile()) {
      if (fileNameRegexp.test(testsDir.leafName))
        var singlePath = basePath + '/' + testPath;
        var links = {};
        links[singlePath] = true;
        return [links, singlePath];

      
      return [];
    }

    
    basePath += "/" + testPath;
  }
  var [links, count] = srvScope.list(basePath, testsDir, true);
  return [links, null];
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
    
    
    targetDir.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, parseInt("0777", 8));
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

