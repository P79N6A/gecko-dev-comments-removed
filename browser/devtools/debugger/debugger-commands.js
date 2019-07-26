



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const gcli = require("gcli/index");

loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");




exports.items = [];















function getAllBreakpoints(dbg) {
  let breakpoints = [];
  let sources = dbg._view.Sources;
  let { trimUrlLength: trim } = dbg.panelWin.SourceUtils;

  for (let source of sources) {
    for (let { attachment: breakpoint } of source) {
      breakpoints.push({
        url: source.value,
        label: source.attachment.label + ":" + breakpoint.line,
        lineNumber: breakpoint.line,
        lineText: breakpoint.text,
        truncatedLineText: trim(breakpoint.text, MAX_LINE_TEXT_LENGTH, "end")
      });
    }
  }

  return breakpoints;
}




exports.items.push({
  name: "break",
  description: gcli.lookup("breakDesc"),
  manual: gcli.lookup("breakManual")
});




exports.items.push({
  name: "break list",
  description: gcli.lookup("breaklistDesc"),
  returnType: "breakpoints",
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger", { ensureOpened: true });
    return dbg.then(getAllBreakpoints);
  }
});

exports.items.push({
  item: "converter",
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




exports.items.push({
  name: "break add",
  description: gcli.lookup("breakaddDesc"),
  manual: gcli.lookup("breakaddManual")
});




exports.items.push({
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
            return dbg._view.Sources.values;
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
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return gcli.lookup("debuggerStopped");
    }

    let deferred = context.defer();
    let position = { url: args.file, line: args.line };

    dbg.addBreakpoint(position).then(() => {
      deferred.resolve(gcli.lookup("breakaddAdded"));
    }, aError => {
      deferred.resolve(gcli.lookupFormat("breakaddFailed", [aError]));
    });

    return deferred.promise;
  }
});




exports.items.push({
  name: "break del",
  description: gcli.lookup("breakdelDesc"),
  params: [
    {
      name: "breakpoint",
      type: {
        name: "selection",
        lookup: function(context) {
          let dbg = getPanel(context, "jsdebugger");
          if (!dbg) {
            return [];
          }
          return getAllBreakpoints(dbg).map(breakpoint => ({
            name: breakpoint.label,
            value: breakpoint,
            description: breakpoint.truncatedLineText
          }));
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

    let deferred = context.defer();
    let position = { url: args.breakpoint.url, line: args.breakpoint.lineNumber };

    dbg.removeBreakpoint(position).then(() => {
      deferred.resolve(gcli.lookup("breakdelRemoved"));
    }, () => {
      deferred.resolve(gcli.lookup("breakNotFound"));
    });

    return deferred.promise;
  }
});




exports.items.push({
  name: "dbg",
  description: gcli.lookup("dbgDesc"),
  manual: gcli.lookup("dbgManual")
});




exports.items.push({
  name: "dbg open",
  description: gcli.lookup("dbgOpen"),
  params: [],
  exec: function(args, context) {
    let target = context.environment.target;
    return gDevTools.showToolbox(target, "jsdebugger").then(() => null);
  }
});




exports.items.push({
  name: "dbg close",
  description: gcli.lookup("dbgClose"),
  params: [],
  exec: function(args, context) {
    if (!getPanel(context, "jsdebugger")) {
      return;
    }
    let target = context.environment.target;
    return gDevTools.closeToolbox(target).then(() => null);
  }
});




exports.items.push({
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




exports.items.push({
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




exports.items.push({
  name: "dbg step",
  description: gcli.lookup("dbgStepDesc"),
  manual: gcli.lookup("dbgStepManual")
});




exports.items.push({
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




exports.items.push({
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




exports.items.push({
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




exports.items.push({
  name: "dbg list",
  description: gcli.lookup("dbgListSourcesDesc"),
  params: [],
  returnType: "dom",
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return gcli.lookup("debuggerClosed");
    }

    let sources = dbg._view.Sources.values;
    let doc = context.environment.chromeDocument;
    let div = createXHTMLElement(doc, "div");
    let ol = createXHTMLElement(doc, "ol");

    sources.forEach(source => {
      let li = createXHTMLElement(doc, "li");
      li.textContent = source;
      ol.appendChild(li);
    });
    div.appendChild(ol);

    return div;
  }
});




[
  {
    name: "blackbox",
    clientMethod: "blackBox",
    l10nPrefix: "dbgBlackBox"
  },
  {
    name: "unblackbox",
    clientMethod: "unblackBox",
    l10nPrefix: "dbgUnBlackBox"
  }
].forEach(function(cmd) {
  const lookup = function(id) {
    return gcli.lookup(cmd.l10nPrefix + id);
  };

  exports.items.push({
    name: "dbg " + cmd.name,
    description: lookup("Desc"),
    params: [
      {
        name: "source",
        type: {
          name: "selection",
          data: function(context) {
            let dbg = getPanel(context, "jsdebugger");
            if (dbg) {
              return dbg._view.Sources.values;
            }
            return [];
          }
        },
        description: lookup("SourceDesc"),
        defaultValue: null
      },
      {
        name: "glob",
        type: "string",
        description: lookup("GlobDesc"),
        defaultValue: null
      },
      {
        name: "invert",
        type: "boolean",
        description: lookup("InvertDesc")
      }
    ],
    returnType: "dom",
    exec: function(args, context) {
      const dbg = getPanel(context, "jsdebugger");
      const doc = context.environment.chromeDocument;
      if (!dbg) {
        throw new Error(gcli.lookup("debuggerClosed"));
      }

      const { promise, resolve, reject } = context.defer();
      const { activeThread } = dbg._controller;
      const globRegExp = args.glob ? globToRegExp(args.glob) : null;

      

      function shouldBlackBox(source) {
        var value = globRegExp && globRegExp.test(source.url)
          || args.source && source.url == args.source;
        return args.invert ? !value : value;
      }

      const toBlackBox = [s.attachment.source
                          for (s of dbg._view.Sources.items)
                          if (shouldBlackBox(s.attachment.source))];

      

      if (toBlackBox.length === 0) {
        const empty = createXHTMLElement(doc, "div");
        empty.textContent = lookup("EmptyDesc");
        return void resolve(empty);
      }

      
      

      const blackBoxed = [];

      for (let source of toBlackBox) {
        activeThread.source(source)[cmd.clientMethod](function({ error }) {
          if (error) {
            blackBoxed.push(lookup("ErrorDesc") + " " + source.url);
          } else {
            blackBoxed.push(source.url);
          }

          if (toBlackBox.length === blackBoxed.length) {
            displayResults();
          }
        });
      }

      

      function displayResults() {
        const results = doc.createElement("div");
        results.textContent = lookup("NonEmptyDesc");

        const list = createXHTMLElement(doc, "ul");
        results.appendChild(list);

        for (let result of blackBoxed) {
          const item = createXHTMLElement(doc, "li");
          item.textContent = result;
          list.appendChild(item);
        }
        resolve(results);
      }

      return promise;
    }
  });
});




function createXHTMLElement(document, tagname) {
  return document.createElementNS("http://www.w3.org/1999/xhtml", tagname);
}




function getPanel(context, id, options = {}) {
  if (!context) {
    return undefined;
  }

  let target = context.environment.target;

  if (options.ensureOpened) {
    return gDevTools.showToolbox(target, id).then(toolbox => {
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




function globToRegExp(glob) {
  const reStr = glob
  
    .replace(/\\/g, "\\\\")
    .replace(/\//g, "\\/")
    .replace(/\^/g, "\\^")
    .replace(/\$/g, "\\$")
    .replace(/\+/g, "\\+")
    .replace(/\?/g, "\\?")
    .replace(/\./g, "\\.")
    .replace(/\(/g, "\\(")
    .replace(/\)/g, "\\)")
    .replace(/\=/g, "\\=")
    .replace(/\!/g, "\\!")
    .replace(/\|/g, "\\|")
    .replace(/\{/g, "\\{")
    .replace(/\}/g, "\\}")
    .replace(/\,/g, "\\,")
    .replace(/\[/g, "\\[")
    .replace(/\]/g, "\\]")
    .replace(/\-/g, "\\-")
  
    .replace(/\*/g, ".*")
  return new RegExp("^" + reStr + "$");
}
