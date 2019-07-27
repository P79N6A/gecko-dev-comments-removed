


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { processes, remoteRequire } = require("../remote/parent");
remoteRequire("sdk/content/l10n-html");

let enabled = false;
function enable() {
  if (!enabled) {
    processes.port.emit("sdk/l10n/html/enable");
    enabled = true;
  }
}
exports.enable = enable;

function disable() {
  if (enabled) {
    processes.port.emit("sdk/l10n/html/disable");
    enabled = false;
  }
}
exports.disable = disable;

processes.forEvery(process => {
  process.port.emit(enabled ? "sdk/l10n/html/enable" : "sdk/l10n/html/disable");
});
