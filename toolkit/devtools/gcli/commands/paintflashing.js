



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const TargetFactory = require("resource://gre/modules/devtools/Loader.jsm").devtools.TargetFactory;

const Telemetry = require("devtools/shared/telemetry");
const telemetry = new Telemetry();

const EventEmitter = require("devtools/toolkit/event-emitter");
const eventEmitter = new EventEmitter();

const gcli = require("gcli/index");
const l10n = require("gcli/l10n");






let isContentPaintFlashing = false;




function onPaintFlashingChanged(target, value) {
  eventEmitter.emit("changed", { target: target });
  function fireChange() {
    eventEmitter.emit("changed", { target: target });
  }

  target.off("navigate", fireChange);
  target.once("navigate", fireChange);

  if (value) {
    telemetry.toolOpened("paintflashing");
  } else {
    telemetry.toolClosed("paintflashing");
  }
}
















function setPaintFlashing(window, state) {
  const winUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIDOMWindowUtils)

  if (["on", "off", "toggle", "query"].indexOf(state) === -1) {
    throw new Error("Unsupported state: " + state);
  }

  if (state === "on") {
    winUtils.paintFlashing = true;
  }
  else if (state === "off") {
    winUtils.paintFlashing = false;
  }
  else if (state === "toggle") {
    winUtils.paintFlashing = !winUtils.paintFlashing;
  }

  return winUtils.paintFlashing;
}

exports.items = [
  {
    name: "paintflashing",
    description: l10n.lookup("paintflashingDesc")
  },
  {
    item: "command",
    runAt: "client",
    name: "paintflashing on",
    description: l10n.lookup("paintflashingOnDesc"),
    manual: l10n.lookup("paintflashingManual"),
    params: [{
      group: "options",
      params: [
        {
          type: "boolean",
          name: "chrome",
          get hidden() gcli.hiddenByChromePref(),
          description: l10n.lookup("paintflashingChromeDesc"),
        }
      ]
    }],
    exec: function*(args, context) {
      if (!args.chrome) {
        const value = yield context.updateExec("paintflashing_server --state on");
        isContentPaintFlashing = value;
        onPaintFlashingChanged(context.environment.target, value);
      }
      else {
        setPaintFlashing(context.environment.chromeWindow, "on");
      }
    }
  },
  {
    item: "command",
    runAt: "client",
    name: "paintflashing off",
    description: l10n.lookup("paintflashingOffDesc"),
    manual: l10n.lookup("paintflashingManual"),
    params: [{
      group: "options",
      params: [
        {
          type: "boolean",
          name: "chrome",
          get hidden() gcli.hiddenByChromePref(),
          description: l10n.lookup("paintflashingChromeDesc"),
        }
      ]
    }],
    exec: function(args, context) {
      if (!args.chrome) {
        const value = yield context.updateExec("paintflashing_server --state off");
        isContentPaintFlashing = value;
        onPaintFlashingChanged(context.environment.target, value);
      }
      else {
        setPaintFlashing(context.environment.chromeWindow, "off");
      }
    }
  },
  {
    item: "command",
    runAt: "client",
    name: "paintflashing toggle",
    hidden: true,
    buttonId: "command-button-paintflashing",
    buttonClass: "command-button command-button-invertable",
    state: {
      isChecked: () => isContentPaintFlashing,
      onChange: (_, handler) => eventEmitter.on("changed", handler),
      offChange: (_, handler) => eventEmitter.off("changed", handler),
    },
    tooltipText: l10n.lookup("paintflashingTooltip"),
    description: l10n.lookup("paintflashingToggleDesc"),
    manual: l10n.lookup("paintflashingManual"),
    exec: function(args, context) {
      const value = yield context.updateExec("paintflashing_server --state toggle");
      isContentPaintFlashing = value;
      onPaintFlashingChanged(context.environment.target, value);
    }
  },
  {
    item: "command",
    runAt: "server",
    name: "paintflashing_server",
    hidden: true,
    params: [
      {
        name: "state",
        type: {
          name: "selection",
          data: [ "on", "off", "toggle", "query" ]
        }
      },
    ],
    returnType: "boolean",
    exec: function(args, context) {
      return setPaintFlashing(context.environment.window, args.state);
    }
  }
];
