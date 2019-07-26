



"use strict";

let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ResetProfile.jsm");


function populateResetPane(aContainerID) {
  let resetProfileItems = document.getElementById(aContainerID);
  try {
    let dataTypes = ResetProfile.getMigratedData();
    for (let dataType of dataTypes) {
      let label = document.createElement("label");
      label.setAttribute("value", dataType);
      resetProfileItems.appendChild(label);
    }
  } catch (ex) {
    Cu.reportError(ex);
  }
}

function onResetProfileLoad() {
  populateResetPane("migratedItems");
}

function onResetProfileAccepted() {
  let retVals = window.arguments[0];
  retVals.reset = true;
}
