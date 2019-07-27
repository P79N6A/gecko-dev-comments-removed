






const kWhitelist = new Set([
  /defaults\/profile\/prefs.js$/,
]);


let moduleLocation = gTestPath.replace(/\/[^\/]*$/i, "/parsingTestHelpers.jsm");
let {generateURIsFromDirTree} = Cu.import(moduleLocation, {});
let {Reflect} = Cu.import("resource://gre/modules/reflect.jsm", {});








function uriIsWhiteListed(uri) {
  for (let whitelistItem of kWhitelist) {
    if (whitelistItem.test(uri.spec)) {
      return true;
    }
  }
  return false;
}

function parsePromise(uri) {
  let promise = new Promise((resolve, reject) => {
    let xhr = new XMLHttpRequest();
    xhr.open("GET", uri, true);
    xhr.onreadystatechange = function() {
      if (this.readyState == this.DONE) {
        let scriptText = this.responseText;
        let ast;
        try {
          info("Checking " + uri);
          ast = Reflect.parse(scriptText);
          resolve(true);
        } catch (ex) {
          let errorMsg = "Script error reading " + uri + ": " + ex;
          ok(false, errorMsg);
          resolve(false);
        }
      }
    };
    xhr.onerror = (error) => {
      ok(false, "XHR error reading " + uri + ": " + error);
      resolve(false);
    };
    xhr.overrideMimeType("application/javascript");
    xhr.send(null);
  });
  return promise;
}

add_task(function* checkAllTheJS() {
  let appDir = Services.dirsvc.get("XCurProcD", Ci.nsIFile);
  
  
  
  let uris = yield generateURIsFromDirTree(appDir, [".js", ".jsm"]);

  
  
  let allPromises = [];
  for (let uri of uris) {
    if (uriIsWhiteListed(uri)) {
      info("Not checking " + uri.spec);
      continue;
    }
    allPromises.push(parsePromise(uri.spec));
  }

  let promiseResults = yield Promise.all(allPromises);
  is(promiseResults.filter((x) => !x).length, 0, "There should be 0 parsing errors");
});

