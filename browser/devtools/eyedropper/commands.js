



const gcli = require("gcli/index");
const EventEmitter = require("devtools/toolkit/event-emitter");
const eventEmitter = new EventEmitter();

let { Eyedropper, EyedropperManager } = require("devtools/eyedropper/eyedropper");




exports.items = [{
  name: "eyedropper",
  description: gcli.lookup("eyedropperDesc"),
  manual: gcli.lookup("eyedropperManual"),
  buttonId: "command-button-eyedropper",
  buttonClass: "command-button command-button-invertable",
  tooltipText: gcli.lookup("eyedropperTooltip"),
  state: {
    isChecked: function(target) {
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

    let dropper = EyedropperManager.createInstance(chromeWindow);
    dropper.open();

    eventEmitter.emit("changed", { target: target });

    dropper.once("destroy", () => {
      eventEmitter.emit("changed", { target: target });
    });
  }
}];
