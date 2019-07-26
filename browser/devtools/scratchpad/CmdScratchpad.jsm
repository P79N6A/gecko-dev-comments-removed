



this.EXPORTED_SYMBOLS = [ ];

const { devtools } = Components.utils.import("resource://gre/modules/devtools/Loader.jsm", {});
const gcli = devtools.require("gcli/index");




gcli.addCommand({
  name: "scratchpad",
  buttonId: "command-button-scratchpad",
  buttonClass: "command-button command-button-invertable",
  tooltipText: gcli.lookup("scratchpadOpenTooltip"),
  hidden: true,
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeDocument.defaultView;
    chromeWindow.Scratchpad.ScratchpadManager.openScratchpad();
  }
});
