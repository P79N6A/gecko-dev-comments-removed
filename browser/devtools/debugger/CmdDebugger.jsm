




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

this.EXPORTED_SYMBOLS = [ ];

Cu.import("resource://gre/modules/devtools/gcli.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
  "resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");















function getAllBreakpoints(dbg) {
  let breakpoints = [];
  let SourceUtils = dbg.panelWin.SourceUtils;

  for (let source in dbg.panelWin.DebuggerView.Sources) {
    for (let { attachment: breakpoint } in source) {
      breakpoints.push({
        id: source.value + ":" + breakpoint.lineNumber,
        label: source.label + ":" + breakpoint.lineNumber,
        url: source.value,
        lineNumber: breakpoint.lineNumber,
        lineText: breakpoint.lineText,
        truncatedLineText: SourceUtils.trimUrlLength(breakpoint.lineText,
                                                  MAX_LINE_TEXT_LENGTH, "end")
      });
    }
  }

  return breakpoints;
}




gcli.addCommand({
  name: "break",
  description: gcli.lookup("breakDesc"),
  manual: gcli.lookup("breakManual")
});




gcli.addCommand({
  name: "break list",
  description: gcli.lookup("breaklistDesc"),
  returnType: "breakpoints",
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger", { ensure_opened: true });
    return dbg.then(function(dbg) {
      return getAllBreakpoints(dbg);
    });
  }
});

gcli.addConverter({
  from: "breakpoints",
  to: "view",
  exec: function(breakpoints, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (dbg && breakpoints.length) {
      return context.createView({
        html: breakListHtml,
        data: {
          breakpoints: breakpoints,
          onclick: context.update,
          ondblclick: context.updateExec
        }
      });
    } else {
      return context.createView({
        html: "<p>${message}</p>",
        data: { message: gcli.lookup("breaklistNone") }
      });
    }
  }
});

var breakListHtml = "" +
      "<table>" +
      " <thead>" +
      "  <th>Source</th>" +
      "  <th>Line</th>" +
      "  <th>Actions</th>" +
      " </thead>" +
      " <tbody>" +
      "  <tr foreach='breakpoint in ${breakpoints}'>" +
      "    <td class='gcli-breakpoint-label'>${breakpoint.label}</td>" +
      "    <td class='gcli-breakpoint-lineText'>" +
      "      ${breakpoint.truncatedLineText}" +
      "    </td>" +
      "    <td>" +
      "      <span class='gcli-out-shortcut'" +
      "            data-command='break del ${breakpoint.label}'" +
      "            onclick='${onclick}'" +
      "            ondblclick='${ondblclick}'>" +
      "        " + gcli.lookup("breaklistOutRemove") + "</span>" +
      "    </td>" +
      "  </tr>" +
      " </tbody>" +
      "</table>" +
      "";

var MAX_LINE_TEXT_LENGTH = 30;
var MAX_LABEL_LENGTH = 20;




gcli.addCommand({
  name: "break add",
  description: gcli.lookup("breakaddDesc"),
  manual: gcli.lookup("breakaddManual")
});




gcli.addCommand({
  name: "break add line",
  description: gcli.lookup("breakaddlineDesc"),
  params: [
    {
      name: "file",
      type: {
        name: "selection",
        data: function(context) {
          let dbg = getPanel(context, "jsdebugger");
          if (dbg) {
            return dbg.panelWin.DebuggerView.Sources.values;
          }
          return [];
        }
      },
      description: gcli.lookup("breakaddlineFileDesc")
    },
    {
      name: "line",
      type: { name: "number", min: 1, step: 10 },
      description: gcli.lookup("breakaddlineLineDesc")
    }
  ],
  returnType: "string",
  exec: function(args, context) {
    args.type = "line";

    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return gcli.lookup("debuggerStopped");
    }

    let deferred = context.defer();
    let position = { url: args.file, line: args.line };
    dbg.addBreakpoint(position, function(aBreakpoint, aError) {
      if (aError) {
        deferred.resolve(gcli.lookupFormat("breakaddFailed", [aError]));
        return;
      }
      deferred.resolve(gcli.lookup("breakaddAdded"));
    });
    return deferred.promise;
  }
});




gcli.addCommand({
  name: "break del",
  description: gcli.lookup("breakdelDesc"),
  params: [
    {
      name: "breakpoint",
      type: {
        name: "selection",
        lookup: function(context) {
          let dbg = getPanel(context, "jsdebugger");

          if (dbg == null) {
            return [];
          }

          return getAllBreakpoints(dbg).map(breakpoint => {
            return {
              name: breakpoint.label,
              value: breakpoint,
              description: breakpoint.truncatedLineText
            };
          });
        }
      },
      description: gcli.lookup("breakdelBreakidDesc")
    }
  ],
  returnType: "string",
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return gcli.lookup("debuggerStopped");
    }

    let breakpoint = dbg.getBreakpoint(args.breakpoint.url,
                                       args.breakpoint.lineNumber);

    if (breakpoint == null) {
      return gcli.lookup("breakNotFound");
    }

    let deferred = context.defer();
    try {
      dbg.removeBreakpoint(breakpoint, function() {
        deferred.resolve(gcli.lookup("breakdelRemoved"));
      });
    } catch (ex) {
      console.error('Error removing breakpoint, already removed?', ex);
      
      deferred.resolve(gcli.lookup("breakdelRemoved"));
    }
    return deferred.promise;
  }
});




gcli.addCommand({
  name: "dbg",
  description: gcli.lookup("dbgDesc"),
  manual: gcli.lookup("dbgManual")
});




gcli.addCommand({
  name: "dbg open",
  description: gcli.lookup("dbgOpen"),
  params: [],
  exec: function(args, context) {
    return gDevTools.showToolbox(context.environment.target, "jsdebugger")
                    .then(() => null);
  }
});




gcli.addCommand({
  name: "dbg close",
  description: gcli.lookup("dbgClose"),
  params: [],
  exec: function(args, context) {
    if (!getPanel(context, "jsdebugger"))
      return;

    return gDevTools.closeToolbox(context.environment.target)
                    .then(() => null);
  }
});




gcli.addCommand({
  name: "dbg interrupt",
  description: gcli.lookup("dbgInterrupt"),
  params: [],
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return gcli.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (!thread.paused) {
      thread.interrupt();
    }
  }
});




gcli.addCommand({
  name: "dbg continue",
  description: gcli.lookup("dbgContinue"),
  params: [],
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return gcli.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (thread.paused) {
      thread.resume();
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
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return gcli.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (thread.paused) {
      thread.stepOver();
    }
  }
});




gcli.addCommand({
  name: 'dbg step in',
  description: gcli.lookup("dbgStepInDesc"),
  params: [],
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return gcli.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (thread.paused) {
      thread.stepIn();
    }
  }
});




gcli.addCommand({
  name: 'dbg step out',
  description: gcli.lookup("dbgStepOutDesc"),
  params: [],
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return gcli.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (thread.paused) {
      thread.stepOut();
    }
  }
});




gcli.addCommand({
  name: "dbg list",
  description: gcli.lookup("dbgListSourcesDesc"),
  params: [],
  returnType: "dom",
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    let doc = context.environment.chromeDocument;
    if (!dbg) {
      return gcli.lookup("debuggerClosed");
    }

    let sources = dbg._view.Sources.values;
    let div = createXHTMLElement(doc, "div");
    let ol = createXHTMLElement(doc, "ol");
    sources.forEach(function(src) {
      let li = createXHTMLElement(doc, "li");
      li.textContent = src;
      ol.appendChild(li);
    });
    div.appendChild(ol);

    return div;
  }
});




function createXHTMLElement(document, tagname) {
  return document.createElementNS("http://www.w3.org/1999/xhtml", tagname);
}




function getPanel(context, id, options = {}) {
  if (context == null) {
    return undefined;
  }

  let target = context.environment.target;
  if (options.ensure_opened) {
    return gDevTools.showToolbox(target, id).then(function(toolbox) {
      return toolbox.getPanel(id);
    });
  } else {
    let toolbox = gDevTools.getToolbox(target);
    if (toolbox) {
      return toolbox.getPanel(id);
    } else {
      return undefined;
    }
  }
}
