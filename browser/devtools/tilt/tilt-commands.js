



"use strict";

const l10n = require("gcli/l10n");




Object.defineProperty(this, "TiltManager", {
  get: function() {
    return require("devtools/tilt/tilt").TiltManager;
  },
  enumerable: true
});

exports.items = [
{
  name: 'tilt',
  description: l10n.lookup("tiltDesc"),
  manual: l10n.lookup("tiltManual"),
  hidden: true
},
{
  name: 'tilt open',
  description: l10n.lookup("tiltOpenDesc"),
  manual: l10n.lookup("tiltOpenManual"),
  hidden: true,
  exec: function(args, context) {
    if (isMultiProcess(context)) {
      return l10n.lookupFormat("notAvailableInE10S", [this.name]);
    }

    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);
    if (!Tilt.currentInstance) {
      Tilt.toggle();
    }
  }
},
{
  name: "tilt toggle",
  buttonId: "command-button-tilt",
  buttonClass: "command-button command-button-invertable",
  tooltipText: l10n.lookup("tiltToggleTooltip"),
  hidden: true,
  state: {
    isChecked: function(aTarget) {
      if (!aTarget.tab) {
        return false;
      }
      let browserWindow = aTarget.tab.ownerDocument.defaultView;
      return !!TiltManager.getTiltForBrowser(browserWindow).currentInstance;
    },
    onChange: function(aTarget, aChangeHandler) {
      if (!aTarget.tab) {
        return;
      }
      let browserWindow = aTarget.tab.ownerDocument.defaultView;
      let tilt = TiltManager.getTiltForBrowser(browserWindow);
      tilt.on("change", aChangeHandler);
    },
    offChange: function(aTarget, aChangeHandler) {
      if (!aTarget.tab) {
        return;
      }
      let browserWindow = aTarget.tab.ownerDocument.defaultView;
      let tilt = TiltManager.getTiltForBrowser(browserWindow);
      tilt.off("change", aChangeHandler);
    },
  },
  exec: function(args, context) {
    if (isMultiProcess(context)) {
      return l10n.lookupFormat("notAvailableInE10S", [this.name]);
    }

    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);
    Tilt.toggle();
  }
},
{
  name: 'tilt translate',
  description: l10n.lookup("tiltTranslateDesc"),
  manual: l10n.lookup("tiltTranslateManual"),
  hidden: true,
  params: [
    {
      name: "x",
      type: "number",
      defaultValue: 0,
      description: l10n.lookup("tiltTranslateXDesc"),
      manual: l10n.lookup("tiltTranslateXManual")
    },
    {
      name: "y",
      type: "number",
      defaultValue: 0,
      description: l10n.lookup("tiltTranslateYDesc"),
      manual: l10n.lookup("tiltTranslateYManual")
    }
  ],
  exec: function(args, context) {
    if (isMultiProcess(context)) {
      return l10n.lookupFormat("notAvailableInE10S", [this.name]);
    }

    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);
    if (Tilt.currentInstance) {
      Tilt.currentInstance.controller.arcball.translate([args.x, args.y]);
    }
  }
},
{
  name: 'tilt rotate',
  description: l10n.lookup("tiltRotateDesc"),
  manual: l10n.lookup("tiltRotateManual"),
  hidden: true,
  params: [
    {
      name: "x",
      type: { name: 'number', min: -360, max: 360, step: 10 },
      defaultValue: 0,
      description: l10n.lookup("tiltRotateXDesc"),
      manual: l10n.lookup("tiltRotateXManual")
    },
    {
      name: "y",
      type: { name: 'number', min: -360, max: 360, step: 10 },
      defaultValue: 0,
      description: l10n.lookup("tiltRotateYDesc"),
      manual: l10n.lookup("tiltRotateYManual")
    },
    {
      name: "z",
      type: { name: 'number', min: -360, max: 360, step: 10 },
      defaultValue: 0,
      description: l10n.lookup("tiltRotateZDesc"),
      manual: l10n.lookup("tiltRotateZManual")
    }
  ],
  exec: function(args, context) {
    if (isMultiProcess(context)) {
      return l10n.lookupFormat("notAvailableInE10S", [this.name]);
    }

    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);
    if (Tilt.currentInstance) {
      Tilt.currentInstance.controller.arcball.rotate([args.x, args.y, args.z]);
    }
  }
},
{
  name: 'tilt zoom',
  description: l10n.lookup("tiltZoomDesc"),
  manual: l10n.lookup("tiltZoomManual"),
  hidden: true,
  params: [
    {
      name: "zoom",
      type: { name: 'number' },
      description: l10n.lookup("tiltZoomAmountDesc"),
      manual: l10n.lookup("tiltZoomAmountManual")
    }
  ],
  exec: function(args, context) {
    if (isMultiProcess(context)) {
      return l10n.lookupFormat("notAvailableInE10S", [this.name]);
    }

    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);

    if (Tilt.currentInstance) {
      Tilt.currentInstance.controller.arcball.zoom(-args.zoom);
    }
  }
},
{
  name: 'tilt reset',
  description: l10n.lookup("tiltResetDesc"),
  manual: l10n.lookup("tiltResetManual"),
  hidden: true,
  exec: function(args, context) {
    if (isMultiProcess(context)) {
      return l10n.lookupFormat("notAvailableInE10S", [this.name]);
    }

    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);

    if (Tilt.currentInstance) {
      Tilt.currentInstance.controller.arcball.reset();
    }
  }
},
{
  name: 'tilt close',
  description: l10n.lookup("tiltCloseDesc"),
  manual: l10n.lookup("tiltCloseManual"),
  hidden: true,
  exec: function(args, context) {
    if (isMultiProcess(context)) {
      return l10n.lookupFormat("notAvailableInE10S", [this.name]);
    }

    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);

    Tilt.destroy(Tilt.currentWindowId);
  }
}
];

function isMultiProcess(context) {
  return !context.environment.window;
}
