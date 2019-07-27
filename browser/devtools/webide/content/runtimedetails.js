



const Cu = Components.utils;
const {Services} = Cu.import("resource://gre/modules/Services.jsm");
const {require} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
const {AppManager} = require("devtools/webide/app-manager");
const {Connection} = require("devtools/client/connection-manager");
const {RuntimeTypes} = require("devtools/webide/runtimes");
const Strings = Services.strings.createBundle("chrome://browser/locale/devtools/webide.properties");

window.addEventListener("load", function onLoad() {
  window.removeEventListener("load", onLoad);
  document.querySelector("#close").onclick = CloseUI;
  document.querySelector("#certified-check button").onclick = EnableCertApps;
  document.querySelector("#adb-check button").onclick = RootADB;
  AppManager.on("app-manager-update", OnAppManagerUpdate);
  BuildUI();
  CheckLockState();
}, true);

window.addEventListener("unload", function onUnload() {
  window.removeEventListener("unload", onUnload);
  AppManager.off("app-manager-update", OnAppManagerUpdate);
});

function CloseUI() {
  window.parent.UI.openProject();
}

function OnAppManagerUpdate(event, what) {
  if (what == "connection" || what == "list-tabs-response") {
    BuildUI();
    CheckLockState();
  }
}

let getDescriptionPromise; 
function BuildUI() {
  let table = document.querySelector("table");
  table.innerHTML = "";
  if (AppManager.connection &&
      AppManager.connection.status == Connection.Status.CONNECTED &&
      AppManager.deviceFront) {
    getDescriptionPromise = AppManager.deviceFront.getDescription();
    getDescriptionPromise.then(json => {
      for (let name in json) {
        let tr = document.createElement("tr");
        let td = document.createElement("td");
        td.textContent = name;
        tr.appendChild(td);
        td = document.createElement("td");
        td.textContent = json[name];
        tr.appendChild(td);
        table.appendChild(tr);
      }
    });
  } else {
    CloseUI();
  }
}

function CheckLockState() {
  let adbCheckResult = document.querySelector("#adb-check > .yesno");
  let certCheckResult = document.querySelector("#certified-check > .yesno");
  let flipCertPerfButton = document.querySelector("#certified-check button");
  let adbRootButton = document.querySelector("#adb-check button");
  let flipCertPerfAction = document.querySelector("#certified-check > .action");
  let adbRootAction = document.querySelector("#adb-check > .action");

  let sYes = Strings.GetStringFromName("runtimedetails_checkyes");
  let sNo = Strings.GetStringFromName("runtimedetails_checkno");
  let sUnknown = Strings.GetStringFromName("runtimedetails_checkunknown");
  let sNotUSB = Strings.GetStringFromName("runtimedetails_notUSBDevice");

  flipCertPerfButton.setAttribute("disabled", "true");
  flipCertPerfAction.setAttribute("hidden", "true");
  adbRootAction.setAttribute("hidden", "true");

  adbCheckResult.textContent = sUnknown;
  certCheckResult.textContent = sUnknown;

  if (AppManager.connection &&
      AppManager.connection.status == Connection.Status.CONNECTED) {

    
    if (AppManager.selectedRuntime.type === RuntimeTypes.USB) {
      let device = AppManager.selectedRuntime.device;
      if (device && device.summonRoot) {
        device.isRoot().then(isRoot => {
          if (isRoot) {
            adbCheckResult.textContent = sYes;
            flipCertPerfButton.removeAttribute("disabled");
          } else {
            adbCheckResult.textContent = sNo;
            adbRootAction.removeAttribute("hidden");
          }
        }, e => console.error(e));
      } else {
        adbCheckResult.textContent = sUnknown;
      }
    } else {
      adbCheckResult.textContent = sNotUSB;
    }

    
    try {
      let prefFront = AppManager.preferenceFront;
      prefFront.getBoolPref("devtools.debugger.forbid-certified-apps").then(isForbidden => {
        if (isForbidden) {
          certCheckResult.textContent = sYes;
          flipCertPerfAction.removeAttribute("hidden");
        } else {
          certCheckResult.textContent = sNo;
        }
      }, e => console.error(e));
    } catch(e) {
      
      certCheckResult.textContent = sYes;
      flipCertPerfAction.removeAttribute("hidden");
    }

  }

}

function EnableCertApps() {
  let device = AppManager.selectedRuntime.device;
  device.shell(
    "stop b2g && " +
    "cd /data/b2g/mozilla/*.default/ && " +
    "echo 'user_pref(\"devtools.debugger.forbid-certified-apps\", false);' >> prefs.js && " +
    "start b2g"
  );
}

function RootADB() {
  let device = AppManager.selectedRuntime.device;
  device.summonRoot().then(CheckLockState, (e) => console.error(e));
}
