



"use strict";






const DEFAULT_SNIPPETS_URLS = [
  "https://www.mozilla.org/firefox/features/?utm_source=snippet&utm_medium=snippet&utm_campaign=default+feature+snippet"
, "https://addons.mozilla.org/firefox/?utm_source=snippet&utm_medium=snippet&utm_campaign=addons"
];

const SNIPPETS_UPDATE_INTERVAL_MS = 14400000; 


const DATABASE_NAME = "abouthome";
const DATABASE_VERSION = 1;
const DATABASE_STORAGE = "persistent";
const SNIPPETS_OBJECTSTORE_NAME = "snippets";


let gInitialized = false;
let gObserver = new MutationObserver(function (mutations) {
  for (let mutation of mutations) {
    if (mutation.attributeName == "snippetsVersion") {
      if (!gInitialized) {
        ensureSnippetsMapThen(loadSnippets);
        gInitialized = true;
      }
      return;
    }
  }
});

window.addEventListener("pageshow", function () {
  
  
  window.gObserver.observe(document.documentElement, { attributes: true });
  fitToWidth();
  setupSearch();
  window.addEventListener("resize", fitToWidth);

  
  var event = new CustomEvent("AboutHomeLoad", {bubbles:true});
  document.dispatchEvent(event);
});

window.addEventListener("pagehide", function() {
  window.gObserver.disconnect();
  window.removeEventListener("resize", fitToWidth);
});




let gSnippetsMap;
let gSnippetsMapCallbacks = [];









function ensureSnippetsMapThen(aCallback)
{
  if (gSnippetsMap) {
    aCallback(gSnippetsMap);
    return;
  }

  
  gSnippetsMapCallbacks.push(aCallback);
  if (gSnippetsMapCallbacks.length > 1) {
    
    return;
  }

  let invokeCallbacks = function () {
    if (!gSnippetsMap) {
      gSnippetsMap = Object.freeze(new Map());
    }

    for (let callback of gSnippetsMapCallbacks) {
      callback(gSnippetsMap);
    }
    gSnippetsMapCallbacks.length = 0;
  }

  let openRequest = indexedDB.open(DATABASE_NAME, {version: DATABASE_VERSION,
                                                   storage: DATABASE_STORAGE});

  openRequest.onerror = function (event) {
    
    
    indexedDB.deleteDatabase(DATABASE_NAME);
    invokeCallbacks();
  };

  openRequest.onupgradeneeded = function (event) {
    let db = event.target.result;
    if (!db.objectStoreNames.contains(SNIPPETS_OBJECTSTORE_NAME)) {
      db.createObjectStore(SNIPPETS_OBJECTSTORE_NAME);
    }
  }

  openRequest.onsuccess = function (event) {
    let db = event.target.result;

    db.onerror = function (event) {
      invokeCallbacks();
    }

    db.onversionchange = function (event) {
      event.target.close();
      invokeCallbacks();
    }

    let cache = new Map();
    let cursorRequest = db.transaction(SNIPPETS_OBJECTSTORE_NAME)
                          .objectStore(SNIPPETS_OBJECTSTORE_NAME).openCursor();
    cursorRequest.onerror = function (event) {
      invokeCallbacks();
    }

    cursorRequest.onsuccess = function(event) {
      let cursor = event.target.result;

      
      if (cursor) {
        cache.set(cursor.key, cursor.value);
        cursor.continue();
        return;
      }

      
      gSnippetsMap = Object.freeze({
        get: (aKey) => cache.get(aKey),
        set: function (aKey, aValue) {
          db.transaction(SNIPPETS_OBJECTSTORE_NAME, "readwrite")
            .objectStore(SNIPPETS_OBJECTSTORE_NAME).put(aValue, aKey);
          return cache.set(aKey, aValue);
        },
        has: (aKey) => cache.has(aKey),
        delete: function (aKey) {
          db.transaction(SNIPPETS_OBJECTSTORE_NAME, "readwrite")
            .objectStore(SNIPPETS_OBJECTSTORE_NAME).delete(aKey);
          return cache.delete(aKey);
        },
        clear: function () {
          db.transaction(SNIPPETS_OBJECTSTORE_NAME, "readwrite")
            .objectStore(SNIPPETS_OBJECTSTORE_NAME).clear();
          return cache.clear();
        },
        get size() { return cache.size; },
      });

      setTimeout(invokeCallbacks, 0);
    }
  }
}

function onSearchSubmit(aEvent)
{
  gContentSearchController.search(aEvent);
}


let gContentSearchController;

function setupSearch()
{
  
  
  
  let searchText = document.getElementById("searchText");
  searchText.addEventListener("blur", function searchText_onBlur() {
    searchText.removeEventListener("blur", searchText_onBlur);
    searchText.removeAttribute("autofocus");
  });

  if (!gContentSearchController) {
    gContentSearchController =
      new ContentSearchUIController(searchText, searchText.parentNode,
                                    "abouthome", "homepage");
  }
}




function loadCompleted()
{
  var event = new CustomEvent("AboutHomeLoadSnippetsCompleted", {bubbles:true});
  document.dispatchEvent(event);
}





function loadSnippets()
{
  if (!gSnippetsMap)
    throw new Error("Snippets map has not properly been initialized");

  
  var event = new CustomEvent("AboutHomeLoadSnippets", {bubbles:true});
  document.dispatchEvent(event);

  
  let cachedVersion = gSnippetsMap.get("snippets-cached-version") || 0;
  let currentVersion = document.documentElement.getAttribute("snippetsVersion");
  if (cachedVersion < currentVersion) {
    
    gSnippetsMap.clear();
  }

  
  let lastUpdate = gSnippetsMap.get("snippets-last-update");
  let updateURL = document.documentElement.getAttribute("snippetsURL");
  let shouldUpdate = !lastUpdate ||
                     Date.now() - lastUpdate > SNIPPETS_UPDATE_INTERVAL_MS;
  if (updateURL && shouldUpdate) {
    
    let xhr = new XMLHttpRequest();
    xhr.timeout = 5000;
    try {
      xhr.open("GET", updateURL, true);
    } catch (ex) {
      showSnippets();
      loadCompleted();
      return;
    }
    
    
    gSnippetsMap.set("snippets-last-update", Date.now());
    xhr.onloadend = function (event) {
      if (xhr.status == 200) {
        gSnippetsMap.set("snippets", xhr.responseText);
        gSnippetsMap.set("snippets-cached-version", currentVersion);
      }
      showSnippets();
      loadCompleted();
    };
    xhr.send(null);
  } else {
    showSnippets();
    loadCompleted();
  }
}







let _snippetsShown = false;
function showSnippets()
{
  let snippetsElt = document.getElementById("snippets");

  
  let showRights = document.documentElement.getAttribute("showKnowYourRights");
  if (showRights) {
    let rightsElt = document.getElementById("rightsSnippet");
    let anchor = rightsElt.getElementsByTagName("a")[0];
    anchor.href = "about:rights";
    snippetsElt.appendChild(rightsElt);
    rightsElt.removeAttribute("hidden");
    return;
  }

  if (!gSnippetsMap)
    throw new Error("Snippets map has not properly been initialized");
  if (_snippetsShown) {
    
    
    showDefaultSnippets();
    throw new Error("showSnippets should never be invoked multiple times");
  }
  _snippetsShown = true;

  let snippets = gSnippetsMap.get("snippets");
  
  if (snippets) {
    
    try {
      snippetsElt.innerHTML = snippets;
      
      
      Array.forEach(snippetsElt.getElementsByTagName("script"), function(elt) {
        let relocatedScript = document.createElement("script");
        relocatedScript.type = "text/javascript;version=1.8";
        relocatedScript.text = elt.text;
        elt.parentNode.replaceChild(relocatedScript, elt);
      });
      return;
    } catch (ex) {
      
    }
  }

  showDefaultSnippets();
}




function showDefaultSnippets()
{
  
  let snippetsElt = document.getElementById("snippets");
  snippetsElt.innerHTML = "";

  
  let defaultSnippetsElt = document.getElementById("defaultSnippets");
  let entries = defaultSnippetsElt.querySelectorAll("span");
  
  let randIndex = Math.floor(Math.random() * entries.length);
  let entry = entries[randIndex];
  
  if (DEFAULT_SNIPPETS_URLS[randIndex]) {
    let links = entry.getElementsByTagName("a");
    
    
    if (links.length == 1) {
      links[0].href = DEFAULT_SNIPPETS_URLS[randIndex];
    }
  }
  
  snippetsElt.appendChild(entry);
}

function fitToWidth() {
  if (window.scrollMaxX) {
    document.body.setAttribute("narrow", "true");
  } else if (document.body.hasAttribute("narrow")) {
    document.body.removeAttribute("narrow");
    fitToWidth();
  }
}
