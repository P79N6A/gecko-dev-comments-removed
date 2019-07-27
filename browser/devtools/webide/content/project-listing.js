



const Cu = Components.utils;
const {require} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
const {AppManager} = require("devtools/webide/app-manager");
const ProjectList = require("devtools/webide/project-list");

let projectList = new ProjectList(window, window.parent);

window.addEventListener("load", function onLoad() {
  window.removeEventListener("load", onLoad);
  AppManager.on("app-manager-update", onAppManagerUpdate);
  document.getElementById("new-app").onclick = CreateNewApp;
  document.getElementById("hosted-app").onclick = ImportHostedApp;
  document.getElementById("packaged-app").onclick = ImportPackagedApp;
  projectList.update();
}, true);

window.addEventListener("unload", function onUnload() {
  window.removeEventListener("unload", onUnload);
  projectList = null;
  AppManager.off("app-manager-update", onAppManagerUpdate);
});

function onAppManagerUpdate(event, what) {
  switch (what) {
    case "list-tabs-response":
    case "runtime-apps-found":
    case "project-validated":
    case "project-removed":
    case "project":
      projectList.update();
      break;
  }
}

function CreateNewApp() {
  projectList.newApp();
}

function ImportHostedApp() {
  projectList.importHostedApp();
}

function ImportPackagedApp() {
  projectList.importPackagedApp();
}
