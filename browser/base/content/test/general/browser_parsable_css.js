










const kWhitelist = [
  {sourceName: /cleopatra.*(tree|ui)\.css/i}, 
  {sourceName: /codemirror\.css/i}, 
  {sourceName: /web\/viewer\.css/i, errorMessage: /Unknown pseudo-class.*(fullscreen|selection)/i }, 
  {sourceName: /aboutaccounts\/(main|normalize)\.css/i}, 
  {sourceName: /loop\/.*sdk-content\/.*\.css$/i }
];








function ignoredError(aErrorObject) {
  for (let whitelistItem of kWhitelist) {
    let matches = true;
    for (let prop in whitelistItem) {
      if (!whitelistItem[prop].test(aErrorObject[prop] || "")) {
        matches = false;
        break;
      }
    }
    if (matches) {
      return true;
    }
  }
  return false;
}








function generateURIsFromDirTree(appDir) {
  let rv = [];
  let dirQueue = [appDir.path];
  return Task.spawn(function*() {
    while (dirQueue.length) {
      let nextDir = dirQueue.shift();
      let {subdirs, cssfiles} = yield iterateOverPath(nextDir);
      dirQueue = dirQueue.concat(subdirs);
      rv = rv.concat(cssfiles);
    }
    return rv;
  });
}


let LocalFile = Components.Constructor("@mozilla.org/file/local;1", Ci.nsIFile, "initWithPath");










function iterateOverPath(path) {
  let iterator = new OS.File.DirectoryIterator(path);
  let parentDir = new LocalFile(path);
  let subdirs = [];
  let cssfiles = [];
  
  let promise = iterator.forEach(
    function onEntry(entry) {
      if (entry.isDir) {
        let subdir = parentDir.clone();
        subdir.append(entry.name);
        subdirs.push(subdir.path);
      } else if (entry.name.endsWith(".css")) {
        let file = parentDir.clone();
        file.append(entry.name);
        let uriSpec = getURLForFile(file);
        cssfiles.push(Services.io.newURI(uriSpec, null, null));
      } else if (entry.name.endsWith(".ja")) {
        let file = parentDir.clone();
        file.append(entry.name);
        let subentries = [uri for (uri of generateEntriesFromJarFile(file))];
        cssfiles = cssfiles.concat(subentries);
      }
    }
  );

  let outerPromise = Promise.defer();
  promise.then(function() {
    outerPromise.resolve({cssfiles: cssfiles, subdirs: subdirs});
    iterator.close();
  }, function(e) {
    outerPromise.reject(e);
    iterator.close();
  });
  return outerPromise.promise;
}



function getURLForFile(file) {
  let fileHandler = Services.io.getProtocolHandler("file");
  fileHandler = fileHandler.QueryInterface(Ci.nsIFileProtocolHandler);
  return fileHandler.getURLSpecFromFile(file);
}







function* generateEntriesFromJarFile(jarFile) {
  const ZipReader = new Components.Constructor("@mozilla.org/libjar/zip-reader;1", "nsIZipReader", "open");
  let zr = new ZipReader(jarFile);
  let entryEnumerator = zr.findEntries("*.css$");

  const kURIStart = getURLForFile(jarFile);
  while (entryEnumerator.hasMore()) {
    let entry = entryEnumerator.getNext();
    let entryURISpec = "jar:" + kURIStart + "!/" + entry;
    yield Services.io.newURI(entryURISpec, null, null);
  }
  zr.close();
}




add_task(function checkAllTheCSS() {
  let appDir = Services.dirsvc.get("XCurProcD", Ci.nsIFile);
  
  
  
  let uris = yield generateURIsFromDirTree(appDir);

  
  let hiddenWin = Services.appShell.hiddenDOMWindow;
  let iframe = hiddenWin.document.createElementNS("http://www.w3.org/1999/xhtml", "html:iframe");
  hiddenWin.document.documentElement.appendChild(iframe);
  let doc = iframe.contentWindow.document;


  
  let errorListener = {
    observe: function(aMessage) {
      if (!aMessage || !(aMessage instanceof Ci.nsIScriptError)) {
        return;
      }
      
      if (aMessage.category.contains("CSS") && aMessage.innerWindowID === 0 && aMessage.outerWindowID === 0) {
        
        if (!ignoredError(aMessage)) {
          ok(false, "Got error message for " + aMessage.sourceName + ": " + aMessage.errorMessage);
          errors++;
        } else {
          info("Ignored error for " + aMessage.sourceName + " because of filter.");
        }
      }
    }
  };

  
  
  let allPromises = [];
  let errors = 0;
  
  Services.console.registerListener(errorListener);
  for (let uri of uris) {
    let linkEl = doc.createElement("link");
    linkEl.setAttribute("rel", "stylesheet");
    let promiseForThisSpec = Promise.defer();
    let onLoad = (e) => {
      promiseForThisSpec.resolve();
      linkEl.removeEventListener("load", onLoad);
      linkEl.removeEventListener("error", onError);
    };
    let onError = (e) => {
      promiseForThisSpec.reject({error: e, href: linkEl.getAttribute("href")});
      linkEl.removeEventListener("load", onLoad);
      linkEl.removeEventListener("error", onError);
    };
    linkEl.addEventListener("load", onLoad);
    linkEl.addEventListener("error", onError);
    linkEl.setAttribute("type", "text/css");
    linkEl.setAttribute("href", uri.spec);
    allPromises.push(promiseForThisSpec.promise);
    doc.head.appendChild(linkEl);
  }

  
  yield Promise.all(allPromises);
  
  
  is(errors, 0, "All the styles (" + allPromises.length + ") loaded without errors.");

  
  Services.console.unregisterListener(errorListener);
  iframe.remove();
  doc.head.innerHTML = '';
  doc = null;
  iframe = null;
});
