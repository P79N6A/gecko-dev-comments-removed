



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
this.EXPORTED_SYMBOLS = [ ];

Cu.import("resource://gre/modules/devtools/gcli.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
                                  "resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");





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
    let panel = getPanel(context, "jsdebugger", {ensure_opened: true});
    return panel.then(function(dbg) {
      let breakpoints = [];
      for (let source in dbg.panelWin.DebuggerView.Sources) {
        for (let { attachment: breakpoint } in source) {
          breakpoints.push({
            url: source.value,
            label: source.label,
            lineNumber: breakpoint.lineNumber,
            lineText: breakpoint.lineText
          });
        }
      }
      return breakpoints;
    });
  }
});

gcli.addConverter({
  from: "breakpoints",
  to: "view",
  exec: function(breakpoints, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (dbg && breakpoints.length) {
      let SourceUtils = dbg.panelWin.SourceUtils;
      let index = 0;
      return context.createView({
        html: breakListHtml,
        data: {
          breakpoints: breakpoints.map(function(breakpoint) {
            return {
              index: index++,
              url: breakpoint.url,
              label: SourceUtils.trimUrlLength(
                breakpoint.label + ":" + breakpoint.lineNumber,
                MAX_LABEL_LENGTH,
                "start"),
              lineText: breakpoint.lineText,
              truncatedLineText: SourceUtils.trimUrlLength(
                breakpoint.lineText,
                MAX_LINE_TEXT_LENGTH,
                "end")
            };
          }),
          onclick: createUpdateHandler(context),
          ondblclick: createExecuteHandler(context)
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
      "            data-command='break del ${breakpoint.index}'" +
      "            onclick='${onclick}'" +
      "            ondblclick='${ondblclick}'" +
      "          >" + gcli.lookup("breaklistOutRemove") + "</span>" +
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
        data: function(args, context) {
          let files = [];
          let dbg = getPanel(context, "jsdebugger");
          if (dbg) {
            let sourcesView = dbg.panelWin.DebuggerView.Sources;
            for (let item in sourcesView) {
              files.push(item.value);
            }
          }
          return files;
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
    var deferred = context.defer();
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
      name: "breakid",
      type: {
        name: "number",
        min: 0,
        max: function(args, context) {
          let dbg = getPanel(context, "jsdebugger");
          return dbg == null ?
              null :
              Object.keys(dbg.getAllBreakpoints()).length - 1;
        },
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

    let breakpoints = dbg.getAllBreakpoints();
    let id = Object.keys(breakpoints)[args.breakid];
    if (!id || !(id in breakpoints)) {
      return gcli.lookup("breakNotFound");
    }

    let deferred = context.defer();
    try {
      dbg.removeBreakpoint(breakpoints[id], function() {
        deferred.resolve(gcli.lookup("breakdelRemoved"));
      });
    } catch (ex) {
      
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
    return gDevTools.showToolbox(context.environment.target, "jsdebugger").then(function() null);
  }
});




gcli.addCommand({
  name: "dbg close",
  description: gcli.lookup("dbgClose"),
  params: [],
  exec: function(args, context) {
    return gDevTools.closeToolbox(context.environment.target).then(function() null);
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





function withCommand(element, action) {
  var command = element.getAttribute("data-command");
  if (!command) {
    command = element.querySelector("*[data-command]")
      .getAttribute("data-command");
  }

  if (command) {
    action(command);
  }
  else {
    console.warn("Missing data-command for " + util.findCssSelector(element));
  }
}







function createUpdateHandler(context) {
  return function(ev) {
    withCommand(ev.currentTarget, function(command) {
      context.update(command);
    });
  }
}







function createExecuteHandler(context) {
  return function(ev) {
    withCommand(ev.currentTarget, function(command) {
      context.exec({
        visible: true,
        typed: command
      });
    });
  }
}




function getPanel(context, id, opts) {
  if (context == null) {
    return undefined;
  }

  let target = context.environment.target;
  if (opts && opts.ensure_opened) {
    return gDevTools.showToolbox(target, id).then(function(toolbox) {
      return toolbox.getPanel(id);
    });
  } else {
    let toolbox = gDevTools.getToolbox(target);
    return toolbox && toolbox.getPanel(id);
  }
}
