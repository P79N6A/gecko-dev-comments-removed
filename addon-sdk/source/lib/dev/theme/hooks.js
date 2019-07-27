



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { method } = require("method/core");

const onEnable = method("dev/theme/hooks#onEnable");
const onDisable = method("dev/theme/hooks#onDisable");

exports.onEnable = onEnable;
exports.onDisable = onDisable;
