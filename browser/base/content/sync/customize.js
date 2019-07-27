



"use strict";

addEventListener("dialogaccept", function () {
  let pane = document.getElementById("sync-customize-pane");
  pane.writePreferences(true);
  window.arguments[0].accepted = true;
});
