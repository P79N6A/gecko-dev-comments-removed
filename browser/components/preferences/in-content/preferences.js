



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let gLastHash = "";

addEventListener("DOMContentLoaded", function onLoad() {
  removeEventListener("DOMContentLoaded", onLoad);
  init_all();
});

function init_all() {
  document.documentElement.instantApply = true;

  gSubDialog.init();
  gMainPane.init();
  gPrivacyPane.init();
  gAdvancedPane.init();
  gApplicationsPane.init();
  gContentPane.init();
  gSyncPane.init();
  gSecurityPane.init();

  var initFinished = new CustomEvent("Initialized", {
  'bubbles': true,
  'cancelable': true
  });
  document.dispatchEvent(initFinished);

  let categories = document.getElementById("categories");
  categories.addEventListener("select", event => gotoPref(event.target.value));

  document.documentElement.addEventListener("keydown", function(event) {
    if (event.keyCode == KeyEvent.DOM_VK_TAB) {
      categories.setAttribute("keyboard-navigation", "true");
    }
  });
  categories.addEventListener("mousedown", function() {
    this.removeAttribute("keyboard-navigation");
  });

  window.addEventListener("hashchange", onHashChange);
  gotoPref();

  let helpCmd = document.getElementById("help-button");
  helpCmd.addEventListener("command", helpButtonCommand);

  
  
  Services.obs.notifyObservers(window, "advanced-pane-loaded", null);
}

window.addEventListener("unload", function onUnload() {
  gSubDialog.uninit();
});

function onHashChange() {
  gotoPref();
}

function gotoPref(aCategory) {
  let categories = document.getElementById("categories");
  const kDefaultCategoryInternalName = categories.firstElementChild.value;
  let hash = document.location.hash;
  let category = aCategory || hash.substr(1) || kDefaultCategoryInternalName;
  category = friendlyPrefCategoryNameToInternalName(category);

  
  
  if (gLastHash == category)
    return;
  let item = categories.querySelector(".category[value=" + category + "]");
  if (!item) {
    category = kDefaultCategoryInternalName;
    item = categories.querySelector(".category[value=" + category + "]");
  }

  let newHash = internalPrefCategoryNameToFriendlyName(category);
  if (gLastHash || category != kDefaultCategoryInternalName) {
    document.location.hash = newHash;
  }
  
  
  gLastHash = category;
  categories.selectedItem = item;
  window.history.replaceState(category, document.title);
  search(category, "data-category");
  let mainContent = document.querySelector(".main-content");
  mainContent.scrollTop = 0;
}

function search(aQuery, aAttribute) {
  let elements = document.getElementById("mainPrefPane").children;
  for (let element of elements) {
    let attributeValue = element.getAttribute(aAttribute);
    element.hidden = (attributeValue != aQuery);
  }
}

function helpButtonCommand() {
  let pane = history.state;
  let categories = document.getElementById("categories");
  let helpTopic = categories.querySelector(".category[value=" + pane + "]")
                            .getAttribute("helpTopic");
  openHelpLink(helpTopic);
}

function friendlyPrefCategoryNameToInternalName(aName) {
  if (aName.startsWith("pane"))
    return aName;
  return "pane" + aName.substring(0,1).toUpperCase() + aName.substr(1);
}


function internalPrefCategoryNameToFriendlyName(aName) {
  return (aName || "").replace(/^pane./, function(toReplace) { return toReplace[4].toLowerCase(); });
}
