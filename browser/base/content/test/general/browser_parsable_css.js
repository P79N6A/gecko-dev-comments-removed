










const kWhitelist = [
  
  {sourceName: /cleopatra.*(tree|ui)\.css/i},
  
  {sourceName: /codemirror\.css/i},
  
  {sourceName: /web\/viewer\.css/i,
   errorMessage: /Unknown pseudo-class.*(fullscreen|selection)/i},
  
  {sourceName: /aboutaccounts\/(main|normalize)\.css/i},
  
  {sourceName: /loop\/.*sdk-content\/.*\.css$/i},
  
  {sourceName: /highlighter\.css/i,
   errorMessage: /Unknown pseudo-class.*moz-native-anonymous/i}
];

let moduleLocation = gTestPath.replace(/\/[^\/]*$/i, "/parsingTestHelpers.jsm");
let {generateURIsFromDirTree} = Cu.import(moduleLocation, {});








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

add_task(function checkAllTheCSS() {
  let appDir = Services.dirsvc.get("XCurProcD", Ci.nsIFile);
  
  
  
  let uris = yield generateURIsFromDirTree(appDir, ".css");

  
  let hiddenWin = Services.appShell.hiddenDOMWindow;
  let iframe = hiddenWin.document.createElementNS("http://www.w3.org/1999/xhtml", "html:iframe");
  hiddenWin.document.documentElement.appendChild(iframe);
  let doc = iframe.contentWindow.document;


  
  let errorListener = {
    observe: function(aMessage) {
      if (!aMessage || !(aMessage instanceof Ci.nsIScriptError)) {
        return;
      }
      
      if (aMessage.category.includes("CSS") && aMessage.innerWindowID === 0 && aMessage.outerWindowID === 0) {
        
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
