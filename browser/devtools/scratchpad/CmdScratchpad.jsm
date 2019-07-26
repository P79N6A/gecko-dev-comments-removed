



this.EXPORTED_SYMBOLS = [ ];

Components.utils.import("resource:///modules/devtools/gcli.jsm");




gcli.addCommand({
  name: "scratchpad",
  buttonId: "command-button-scratchpad",
  buttonClass: "command-button",
  hidden: true,
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeDocument.defaultView;
    chromeWindow.Scratchpad.ScratchpadManager.openScratchpad();
  }
});
