



"use strict";

const { Ci, Cc } = require("chrome");
const { Services } = require("resource://gre/modules/Services.jsm");
const { DOMHelpers } = require("resource:///modules/devtools/DOMHelpers.jsm");
const { Task } = require("resource://gre/modules/Task.jsm");
const { Promise } = require("resource://gre/modules/Promise.jsm");
const { setTimeout } = require("sdk/timers");
const { getMostRecentBrowserWindow } = require("sdk/window/utils");

const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const DEV_EDITION_PROMO_URL = "chrome://browser/content/devtools/framework/dev-edition-promo.xul";
const DEV_EDITION_PROMO_ENABLED_PREF = "devtools.devedition.promo.enabled";
const DEV_EDITION_PROMO_SHOWN_PREF = "devtools.devedition.promo.shown";
const DEV_EDITION_PROMO_URL_PREF = "devtools.devedition.promo.url";
const LOCALE = Cc["@mozilla.org/chrome/chrome-registry;1"]
               .getService(Ci.nsIXULChromeRegistry)
               .getSelectedLocale("global");






function shouldDevEditionPromoShow () {
  return Services.prefs.getBoolPref(DEV_EDITION_PROMO_ENABLED_PREF) &&
         !Services.prefs.getBoolPref(DEV_EDITION_PROMO_SHOWN_PREF) &&
         LOCALE === "en-US";
}

let TYPES = {
  
  
  
  deveditionpromo: {
    predicate: shouldDevEditionPromoShow,
    success: () => Services.prefs.setBoolPref(DEV_EDITION_PROMO_SHOWN_PREF, true),
    action: () => {
      let url = Services.prefs.getCharPref(DEV_EDITION_PROMO_URL_PREF);
      getGBrowser().selectedTab = getGBrowser().addTab(url);
    },
    url: DEV_EDITION_PROMO_URL
  }
};

let panelAttrs = {
  orient: "vertical",
  hidden: "false",
  consumeoutsideclicks: "true",
  noautofocus: "true",
  align: "start",
  role: "alert"
};














exports.showDoorhanger = Task.async(function *({ window, type, anchor }) {
  let { predicate, success, url, action } = TYPES[type];
  
  if (!predicate()) {
    return;
  }

  let document = window.document;

  let panel = document.createElementNS(XULNS, "panel");
  let frame = document.createElementNS(XULNS, "iframe");
  let parentEl = document.querySelector("window");

  frame.setAttribute("src", url);
  let close = () => parentEl.removeChild(panel);

  setDoorhangerStyle(panel, frame);

  panel.appendChild(frame);
  parentEl.appendChild(panel);

  yield onFrameLoad(frame);

  panel.openPopup(anchor);

  let closeBtn = frame.contentDocument.querySelector("#close");
  if (closeBtn) {
    closeBtn.addEventListener("click", close);
  }

  let goBtn = frame.contentDocument.querySelector("#go");
  if (goBtn) {
    goBtn.addEventListener("click", () => {
      if (action) {
        action();
      }
      close();
    });
  }

  
  success();
});

function setDoorhangerStyle (panel, frame) {
  Object.keys(panelAttrs).forEach(prop => panel.setAttribute(prop, panelAttrs[prop]));
  panel.style.margin = "20px";
  panel.style.borderRadius = "5px";
  panel.style.border = "none";
  panel.style.MozAppearance = "none";
  panel.style.backgroundColor = "transparent";

  frame.style.borderRadius = "5px";
  frame.setAttribute("flex", "1");
  frame.setAttribute("width", "450");
  frame.setAttribute("height", "179");
}

function onFrameLoad (frame) {
  let { resolve, promise } = Promise.defer();

  if (frame.contentWindow) {
    let domHelper = new DOMHelpers(frame.contentWindow);
    domHelper.onceDOMReady(resolve);
  } else {
    let callback = () => {
      frame.removeEventListener("DOMContentLoaded", callback);
      resolve();
    }
    frame.addEventListener("DOMContentLoaded", callback);
  }

  return promise;
}

function getGBrowser () {
  return getMostRecentBrowserWindow().gBrowser;
}
