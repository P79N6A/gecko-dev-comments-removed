



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

let EXPORTED_SYMBOLS = [ ];

Cu.import("resource:///modules/devtools/gcli.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");




gcli.addCommand({
  name: "dbg",
  description: gcli.lookup("dbgDesc"),
  manual: gcli.lookup("dbgManual")
});





gcli.addCommand({
  name: "dbg interrupt",
  description: gcli.lookup("dbgInterrupt"),
  params: [],
  exec: function(args, context) {
    let win = context.environment.chromeDocument.defaultView;
    let dbg = win.DebuggerUI.getDebugger();

    if (dbg) {
      let controller = dbg.contentWindow.DebuggerController;
      let thread = controller.activeThread;
      if (!thread.paused) {
        thread.interrupt();
      }
    }
  }
});




gcli.addCommand({
  name: "dbg continue",
  description: gcli.lookup("dbgContinue"),
  params: [],
  exec: function(args, context) {
    let win = context.environment.chromeDocument.defaultView;
    let dbg = win.DebuggerUI.getDebugger();

    if (dbg) {
      let controller = dbg.contentWindow.DebuggerController;
      let thread = controller.activeThread;
      if (thread.paused) {
        thread.resume();
      }
    }
  }
});





gcli.addCommand({
  name: "dbg step",
  description: gcli.lookup("dbgStepDesc"),
  manual: gcli.lookup("dbgStepManual")
});





gcli.addCommand({
  name: "dbg step over",
  description: gcli.lookup("dbgStepOverDesc"),
  params: [],
  exec: function(args, context) {
    let win = context.environment.chromeDocument.defaultView;
    let dbg = win.DebuggerUI.getDebugger();

    if (dbg) {
      let controller = dbg.contentWindow.DebuggerController;
      let thread = controller.activeThread;
      if (thread.paused) {
        thread.stepOver();
      }
    }
  }
});




gcli.addCommand({
  name: 'dbg step in',
  description: gcli.lookup("dbgStepInDesc"),
  params: [],
  exec: function(args, context) {
    let win = context.environment.chromeDocument.defaultView;
    let dbg = win.DebuggerUI.getDebugger();

    if (dbg) {
      let controller = dbg.contentWindow.DebuggerController;
      let thread = controller.activeThread;
      if (thread.paused) {
        thread.stepIn();
      }
    }
  }
});




gcli.addCommand({
  name: 'dbg step out',
  description: gcli.lookup("dbgStepOutDesc"),
  params: [],
  exec: function(args, context) {
    let win = context.environment.chromeDocument.defaultView;
    let dbg = win.DebuggerUI.getDebugger();

    if (dbg) {
      let controller = dbg.contentWindow.DebuggerController;
      let thread = controller.activeThread;
      if (thread.paused) {
        thread.stepOut();
      }
    }
  }
});
