

































"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
const APPLY_TIMEOUT = 10;

let browser = Services.wm.getMostRecentWindow("navigator:browser");
if (!browser) {
  log("Warning: No content browser");
}

let shell = browser.shell;
function getContentWindow() {
  return shell.contentBrowser.contentWindow;
}

function sendContentEvent(type, detail) {
  detail = detail || {};
  detail.type = type;

  let content = getContentWindow();
  shell.sendEvent(content, "mozContentEvent",
                   ObjectWrapper.wrap(detail, content));
  return true;
}

function addChromeEventListener(type, listener) {
  let content = getContentWindow();
  content.addEventListener("mozChromeEvent", function chromeListener(evt) {
    if (!evt.detail || evt.detail.type !== type) {
      return;
    }

    let remove = listener(evt);
    if (remove) {
      content.removeEventListener(chromeListener);
    }
  });
}

function createUpdatePrompt() {
  return Cc["@mozilla.org/updates/update-prompt;1"].
         createInstance(Ci.nsIUpdatePrompt);
}

let oldPrefs = {};

function getPrefByType(pref, prefType) {
  
  try {
    switch (prefType) {
      case "string":
        return Services.prefs.getCharPref(pref);
      case "number":
        return Services.prefs.getIntPref(pref);
      case "boolean":
        return Services.prefs.getBoolPref(pref);
    }
  } catch (e) {}
  return undefined;
}

function setPref(pref, value) {
  switch (typeof(value)) {
    case "string":
      Services.prefs.setCharPref(pref, value);
      break;
    case "number":
      Services.prefs.setIntPref(pref, value);
      break;
    case "boolean":
      Services.prefs.setBoolPref(pref, value);
      break;
  }
}

function getPrefTypeDefaultValue(prefType) {
  switch (prefType) {
    case "string":
      return null;
    case "number":
      return 0;
    case "boolean":
      return false;
  }
  return undefined;
}

function setPrefs() {
  if (!updateArgs) {
    return;
  }

  let prefs = updateArgs.prefs;
  if (!prefs) {
    return;
  }

  let keys = Object.keys(prefs);
  for (let i = 0; i < keys.length; i++) {
    let key = keys[i];
    let value = prefs[key];
    let oldValue = getPrefByType(key, typeof(value));
    if (oldValue !== undefined) {
      oldPrefs[key] = oldValue;
    }

    setPref(key, value);
  }
}

function cleanPrefs() {
  if (!updateArgs) {
    return;
  }

  let prefs = updateArgs.prefs;
  if (!prefs) {
    return;
  }

  let keys = Object.keys(prefs);
  for (let i = 0; i < keys.length; i++) {
    let key = keys[i];
    let value = prefs[key];
    let oldValue = oldPrefs[key];
    if (oldValue === undefined) {
      oldValue = getPrefTypeDefaultValue(typeof(value));
      if (oldValue === undefined) {
        log("Warning: Couldn't unset pref " + key +
            ", unknown type for: " + value);
        continue;
      }
    }

    setPref(key, oldValue);
  }
}

function getStartBuild() {
  let start = updateArgs.start;
  ok(start, "Start build not found in updateArgs");
  return start;
}

function getFinishBuild() {
  let finish = updateArgs.finish;
  ok(finish, "Finish build not found in updateArgs");
  return finish;
}

function isFinishUpdate(update) {
  let finish = getFinishBuild();

  is(update.appVersion, finish.app_version,
     "update app version should be finish app version: " + finish.app_version);
  is(update.buildID, finish.app_build_id,
     "update build ID should be finish app build ID: " + finish.app_build_id);
}

function isStartToFinishUpdate(update) {
  let start = getStartBuild();

  is(update.previousAppVersion, start.app_version,
     "update previous app version should be start app version: " +
     start.app_version);
  isFinishUpdate(update);
}

function statusSettingIs(value, next) {
  Services.settings.createLock().get("gecko.updateStatus", {
    handle: function(name, v) {
      is(v, value);
      next();
    },
    handleError: function(error) {
      fail(error);
    }
  });
}

function applyUpdate() {
  sendContentEvent("update-prompt-apply-result", {
    result: "restart"
  });
}

function runUpdateTest(stage) {
  switch (stage) {
    case "pre-update":
      if (preUpdate) {
        preUpdate();
      }
      break;
    case "apply-update":
      if (applyUpdate) {
        applyUpdate();
      }
      break;
    case "post-update":
      if (postUpdate) {
        postUpdate();
      }
      break;
  }
}

let updateArgs;
if (__marionetteParams) {
  updateArgs = __marionetteParams[0];
}

function cleanUp() {
  cleanPrefs();
  finish();
  
}

setPrefs();
