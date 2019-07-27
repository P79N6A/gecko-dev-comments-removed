






const kWhitelist = new Set([
  /defaults\/profile\/prefs.js$/,
  /browser\/content\/browser\/places\/controller.js$/,
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
  
  
  
  
  
  
  
  let parseRequested = Services.prefs.prefHasUserValue("parse");
  let parseValue = parseRequested && Services.prefs.getCharPref("parse");
  if (SpecialPowers.isDebugBuild) {
    if (!parseRequested) {
      ok(true, "Test disabled on debug build. To run, execute: ./mach" +
               " mochitest-browser --setpref parse=<case_sensitive_filter>" +
               " browser/base/content/test/general/browser_parsable_script.js");
      return;
    }
    
    requestLongerTimeout(20);
  }

  let uris;
  
  if (parseValue && parseValue.includes(":")) {
    uris = [NetUtil.newURI(parseValue)];
  } else {
    let appDir = Services.dirsvc.get("XCurProcD", Ci.nsIFile);
    
    
    
    let startTimeMs = Date.now();
    info("Collecting URIs");
    uris = yield generateURIsFromDirTree(appDir, [".js", ".jsm"]);
    info("Collected URIs in " + (Date.now() - startTimeMs) + "ms");

    
    if (parseValue) {
      uris = uris.filter(uri => {
        if (uri.spec.includes(parseValue)) {
          return true;
        }
        info("Not checking filtered out " + uri.spec);
        return false;
      });
    }
  }

  
  
  let allPromises = [];
  for (let uri of uris) {
    if (uriIsWhiteListed(uri)) {
      info("Not checking whitelisted " + uri.spec);
      continue;
    }
    allPromises.push(parsePromise(uri.spec));
  }

  let promiseResults = yield Promise.all(allPromises);
  is(promiseResults.filter((x) => !x).length, 0, "There should be 0 parsing errors");
});

