






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

function getTestList(params, callback) {
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
  getTestManifest("http://mochi.test:8888/" + params.manifestFile, params, callback);
  return;
}
