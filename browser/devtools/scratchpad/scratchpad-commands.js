



"use strict";

const gcli = require("gcli/index");

exports.items = [{
  name: "scratchpad",
  buttonId: "command-button-scratchpad",
  buttonClass: "command-button command-button-invertable",
  tooltipText: gcli.lookup("scratchpadOpenTooltip"),
  hidden: true,
  exec: function(args, context) {
    let Scratchpad = context.environment.chromeWindow.Scratchpad;
    Scratchpad.ScratchpadManager.openScratchpad();
  }
}];
