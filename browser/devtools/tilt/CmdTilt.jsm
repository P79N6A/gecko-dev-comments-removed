




this.EXPORTED_SYMBOLS = [ ];

Components.utils.import('resource://gre/modules/XPCOMUtils.jsm');
Components.utils.import("resource:///modules/devtools/gcli.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TiltManager",
                                  "resource:///modules/devtools/Tilt.jsm");



gcli.addCommand({
  name: 'tilt',
  description: gcli.lookup("tiltDesc"),
  manual: gcli.lookup("tiltManual")
});





gcli.addCommand({
  name: 'tilt open',
  description: gcli.lookup("tiltOpenDesc"),
  manual: gcli.lookup("tiltOpenManual"),
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);
    if (!Tilt.currentInstance) {
      Tilt.toggle();
    }
  }
});





gcli.addCommand({
  name: "tilt toggle",
  buttonId: "command-button-tilt",
  buttonClass: "command-button",
  tooltipText: gcli.lookup("tiltToggleTooltip"),
  hidden: true,
  state: {
    isChecked: function(aTarget) {
      let browserWindow = aTarget.tab.ownerDocument.defaultView;
      return !!TiltManager.getTiltForBrowser(browserWindow).currentInstance;
    },
    onChange: function(aTarget, aChangeHandler) {
      let browserWindow = aTarget.tab.ownerDocument.defaultView;
      let tilt = TiltManager.getTiltForBrowser(browserWindow);
      tilt.on("change", aChangeHandler);
    },
    offChange: function(aTarget, aChangeHandler) {
      if (aTarget.tab) {
        let browserWindow = aTarget.tab.ownerDocument.defaultView;
        let tilt = TiltManager.getTiltForBrowser(browserWindow);
        tilt.off("change", aChangeHandler);
      }
    },
  },
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);
    Tilt.toggle();
  }
});





gcli.addCommand({
  name: 'tilt translate',
  description: gcli.lookup("tiltTranslateDesc"),
  manual: gcli.lookup("tiltTranslateManual"),
  params: [
    {
      name: "x",
      type: "number",
      defaultValue: 0,
      description: gcli.lookup("tiltTranslateXDesc"),
      manual: gcli.lookup("tiltTranslateXManual")
    },
    {
      name: "y",
      type: "number",
      defaultValue: 0,
      description: gcli.lookup("tiltTranslateYDesc"),
      manual: gcli.lookup("tiltTranslateYManual")
    }
  ],
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);
    if (Tilt.currentInstance) {
      Tilt.currentInstance.controller.arcball.translate([args.x, args.y]);
    }
  }
});





gcli.addCommand({
  name: 'tilt rotate',
  description: gcli.lookup("tiltRotateDesc"),
  manual: gcli.lookup("tiltRotateManual"),
  params: [
    {
      name: "x",
      type: { name: 'number', min: -360, max: 360, step: 10 },
      defaultValue: 0,
      description: gcli.lookup("tiltRotateXDesc"),
      manual: gcli.lookup("tiltRotateXManual")
    },
    {
      name: "y",
      type: { name: 'number', min: -360, max: 360, step: 10 },
      defaultValue: 0,
      description: gcli.lookup("tiltRotateYDesc"),
      manual: gcli.lookup("tiltRotateYManual")
    },
    {
      name: "z",
      type: { name: 'number', min: -360, max: 360, step: 10 },
      defaultValue: 0,
      description: gcli.lookup("tiltRotateZDesc"),
      manual: gcli.lookup("tiltRotateZManual")
    }
  ],
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);
    if (Tilt.currentInstance) {
      Tilt.currentInstance.controller.arcball.rotate([args.x, args.y, args.z]);
    }
  }
});





gcli.addCommand({
  name: 'tilt zoom',
  description: gcli.lookup("tiltZoomDesc"),
  manual: gcli.lookup("tiltZoomManual"),
  params: [
    {
      name: "zoom",
      type: { name: 'number' },
      description: gcli.lookup("tiltZoomAmountDesc"),
      manual: gcli.lookup("tiltZoomAmountManual")
    }
  ],
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);

    if (Tilt.currentInstance) {
      Tilt.currentInstance.controller.arcball.zoom(-args.zoom);
    }
  }
});





gcli.addCommand({
  name: 'tilt reset',
  description: gcli.lookup("tiltResetDesc"),
  manual: gcli.lookup("tiltResetManual"),
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);

    if (Tilt.currentInstance) {
      Tilt.currentInstance.controller.arcball.reset();
    }
  }
});





gcli.addCommand({
  name: 'tilt close',
  description: gcli.lookup("tiltCloseDesc"),
  manual: gcli.lookup("tiltCloseManual"),
  exec: function(args, context) {
    let chromeWindow = context.environment.chromeDocument.defaultView;
    let Tilt = TiltManager.getTiltForBrowser(chromeWindow);

    Tilt.destroy(Tilt.currentWindowId);
  }
});
