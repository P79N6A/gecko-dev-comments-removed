



const l10n = require("gcli/l10n");
const EventEmitter = require("devtools/toolkit/event-emitter");
const eventEmitter = new EventEmitter();

let { Eyedropper, EyedropperManager } = require("devtools/eyedropper/eyedropper");




exports.items = [{
  item: "command",
  runAt: "client",
  name: "eyedropper",
  description: l10n.lookup("eyedropperDesc"),
  manual: l10n.lookup("eyedropperManual"),
  buttonId: "command-button-eyedropper",
  buttonClass: "command-button command-button-invertable",
  tooltipText: l10n.lookup("eyedropperTooltip"),
  state: {
    isChecked: function(target) {
      if (!target.tab) {
        return false;
      }
      let chromeWindow = target.tab.ownerDocument.defaultView;
      let dropper = EyedropperManager.getInstance(chromeWindow);
      if (dropper) {
        return true;
      }
      return false;
    },
    onChange: function(target, changeHandler) {
      eventEmitter.on("changed", changeHandler);
    },
    offChange: function(target, changeHandler) {
      eventEmitter.off("changed", changeHandler);
    },
  },
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeWindow;
    let target = context.environment.target;

    let dropper = EyedropperManager.createInstance(chromeWindow,
                                                   { context: "command",
                                                     copyOnSelect: true });
    dropper.open();

    eventEmitter.emit("changed", { target: target });

    dropper.once("destroy", () => {
      eventEmitter.emit("changed", { target: target });
    });
  }
}];
