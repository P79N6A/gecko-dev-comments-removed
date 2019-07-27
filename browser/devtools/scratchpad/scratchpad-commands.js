



"use strict";

const l10n = require("gcli/l10n");

exports.items = [{
  item: "command",
  runAt: "client",
  name: "scratchpad",
  buttonId: "command-button-scratchpad",
  buttonClass: "command-button command-button-invertable",
  tooltipText: l10n.lookup("scratchpadOpenTooltip"),
  hidden: true,
  exec: function(args, context) {
    let Scratchpad = context.environment.chromeWindow.Scratchpad;
    Scratchpad.ScratchpadManager.openScratchpad();
  }
}];
