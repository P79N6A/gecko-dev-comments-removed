



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const l10n = require("gcli/l10n");

loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");




exports.items = [];















function getAllBreakpoints(dbg) {
  let breakpoints = [];
  let sources = dbg._view.Sources;
  let { trimUrlLength: trim } = dbg.panelWin.SourceUtils;

  for (let source of sources) {
    for (let { attachment: breakpoint } of source) {
      breakpoints.push({
        url: source.attachment.source.url,
        label: source.attachment.label + ":" + breakpoint.line,
        lineNumber: breakpoint.line,
        lineText: breakpoint.text,
        truncatedLineText: trim(breakpoint.text, MAX_LINE_TEXT_LENGTH, "end")
      });
    }
  }

  return breakpoints;
}

function getAllSources(dbg) {
  if (!dbg) {
    return [];
  }

  let items = dbg._view.Sources.items;
  return items
    .filter(item => !!item.attachment.source.url)
    .map(item => ({
      name: item.attachment.source.url,
      value: item.attachment.source.actor
    }));
}




exports.items.push({
  name: "break",
  description: l10n.lookup("breakDesc"),
  manual: l10n.lookup("breakManual")
});




exports.items.push({
  name: "break list",
  item: "command",
  runAt: "client",
  description: l10n.lookup("breaklistDesc"),
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
        data: { message: l10n.lookup("breaklistNone") }
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
      "        " + l10n.lookup("breaklistOutRemove") + "</span>" +
      "    </td>" +
      "  </tr>" +
      " </tbody>" +
      "</table>" +
      "";

var MAX_LINE_TEXT_LENGTH = 30;
var MAX_LABEL_LENGTH = 20;




exports.items.push({
  name: "break add",
  description: l10n.lookup("breakaddDesc"),
  manual: l10n.lookup("breakaddManual")
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: "break add line",
  description: l10n.lookup("breakaddlineDesc"),
  params: [
    {
      name: "file",
      type: {
        name: "selection",
        lookup: function(context) {
          return getAllSources(getPanel(context, "jsdebugger"));
        }
      },
      description: l10n.lookup("breakaddlineFileDesc")
    },
    {
      name: "line",
      type: { name: "number", min: 1, step: 10 },
      description: l10n.lookup("breakaddlineLineDesc")
    }
  ],
  returnType: "string",
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return l10n.lookup("debuggerStopped");
    }

    let deferred = context.defer();
    let item = dbg._view.Sources.getItemForAttachment(a => {
      return a.source && a.source.actor === args.file;
    })
    let position = { actor: item.value, line: args.line };

    dbg.addBreakpoint(position).then(() => {
      deferred.resolve(l10n.lookup("breakaddAdded"));
    }, aError => {
      deferred.resolve(l10n.lookupFormat("breakaddFailed", [aError]));
    });

    return deferred.promise;
  }
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: "break del",
  description: l10n.lookup("breakdelDesc"),
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
      description: l10n.lookup("breakdelBreakidDesc")
    }
  ],
  returnType: "string",
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return l10n.lookup("debuggerStopped");
    }

    let source = dbg._view.Sources.getItemForAttachment(a => {
      return a.source && a.source.url === args.breakpoint.url
    });

    let deferred = context.defer();
    let position = { actor: source.attachment.source.actor,
                     line: args.breakpoint.lineNumber };

    dbg.removeBreakpoint(position).then(() => {
      deferred.resolve(l10n.lookup("breakdelRemoved"));
    }, () => {
      deferred.resolve(l10n.lookup("breakNotFound"));
    });

    return deferred.promise;
  }
});




exports.items.push({
  name: "dbg",
  description: l10n.lookup("dbgDesc"),
  manual: l10n.lookup("dbgManual")
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: "dbg open",
  description: l10n.lookup("dbgOpen"),
  params: [],
  exec: function(args, context) {
    let target = context.environment.target;
    return gDevTools.showToolbox(target, "jsdebugger").then(() => null);
  }
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: "dbg close",
  description: l10n.lookup("dbgClose"),
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
  item: "command",
  runAt: "client",
  name: "dbg interrupt",
  description: l10n.lookup("dbgInterrupt"),
  params: [],
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return l10n.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (!thread.paused) {
      thread.interrupt();
    }
  }
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: "dbg continue",
  description: l10n.lookup("dbgContinue"),
  params: [],
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return l10n.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (thread.paused) {
      thread.resume();
    }
  }
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: "dbg step",
  description: l10n.lookup("dbgStepDesc"),
  manual: l10n.lookup("dbgStepManual")
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: "dbg step over",
  description: l10n.lookup("dbgStepOverDesc"),
  params: [],
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return l10n.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (thread.paused) {
      thread.stepOver();
    }
  }
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: 'dbg step in',
  description: l10n.lookup("dbgStepInDesc"),
  params: [],
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return l10n.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (thread.paused) {
      thread.stepIn();
    }
  }
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: 'dbg step out',
  description: l10n.lookup("dbgStepOutDesc"),
  params: [],
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return l10n.lookup("debuggerStopped");
    }

    let controller = dbg._controller;
    let thread = controller.activeThread;
    if (thread.paused) {
      thread.stepOut();
    }
  }
});




exports.items.push({
  item: "command",
  runAt: "client",
  name: "dbg list",
  description: l10n.lookup("dbgListSourcesDesc"),
  params: [],
  returnType: "dom",
  exec: function(args, context) {
    let dbg = getPanel(context, "jsdebugger");
    if (!dbg) {
      return l10n.lookup("debuggerClosed");
    }

    let sources = getAllSources(dbg);
    let doc = context.environment.chromeDocument;
    let div = createXHTMLElement(doc, "div");
    let ol = createXHTMLElement(doc, "ol");

    sources.forEach(source => {
      let li = createXHTMLElement(doc, "li");
      li.textContent = source.name;
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
    return l10n.lookup(cmd.l10nPrefix + id);
  };

  exports.items.push({
    item: "command",
    runAt: "client",
    name: "dbg " + cmd.name,
    description: lookup("Desc"),
    params: [
      {
        name: "source",
        type: {
          name: "selection",
          lookup: function(context) {
            return getAllSources(getPanel(context, "jsdebugger"));
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
        throw new Error(l10n.lookup("debuggerClosed"));
      }

      const { promise, resolve, reject } = context.defer();
      const { activeThread } = dbg._controller;
      const globRegExp = args.glob ? globToRegExp(args.glob) : null;

      

      function shouldBlackBox(source) {
        var value = globRegExp && globRegExp.test(source.url)
          || args.source && source.actor == args.source;
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
