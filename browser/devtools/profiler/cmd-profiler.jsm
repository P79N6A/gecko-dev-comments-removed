



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
this.EXPORTED_SYMBOLS = [];

Cu.import("resource:///modules/devtools/gcli.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/Require.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
  "resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");

var Promise = require('util/promise');




gcli.addCommand({
  name: "profiler",
  description: gcli.lookup("profilerDesc"),
  manual: gcli.lookup("profilerManual")
});




gcli.addCommand({
  name: "profiler open",
  description: gcli.lookup("profilerOpenDesc"),
  params: [],

  exec: function (args, context) {
    return gDevTools.showToolbox(context.environment.target, "jsprofiler")
      .then(function () null);
  }
});




gcli.addCommand({
  name: "profiler close",
  description: gcli.lookup("profilerCloseDesc"),
  params: [],

  exec: function (args, context) {
    return gDevTools.closeToolbox(context.environment.target)
      .then(function () null);
  }
});




gcli.addCommand({
  name: "profiler start",
  description: gcli.lookup("profilerStartDesc"),
  returnType: "string",

  params: [
    {
      name: "name",
      type: "string"
    }
  ],

  exec: function (args, context) {
    function start() {
      let name = args.name;
      let panel = getPanel(context, "jsprofiler");
      let profile = panel.getProfileByName(name) || panel.createProfile(name);

      if (profile.isStarted) {
        throw gcli.lookup("profilerAlreadyStarted");
      }

      if (profile.isFinished) {
        throw gcli.lookup("profilerAlradyFinished");
      }

      panel.switchToProfile(profile, function () profile.start());
      return gcli.lookup("profilerStarting");
    }

    return gDevTools.showToolbox(context.environment.target, "jsprofiler")
      .then(start);
  }
});




gcli.addCommand({
  name: "profiler stop",
  description: gcli.lookup("profilerStopDesc"),
  returnType: "string",

  params: [
    {
      name: "name",
      type: "string"
    }
  ],

  exec: function (args, context) {
    function stop() {
      let panel = getPanel(context, "jsprofiler");
      let profile = panel.getProfileByName(args.name);

      if (!profile) {
        throw gcli.lookup("profilerNotFound");
      }

      if (profile.isFinished) {
        throw gcli.lookup("profilerAlreadyFinished");
      }

      if (!profile.isStarted) {
        throw gcli.lookup("profilerNotStarted");
      }

      panel.switchToProfile(profile, function () profile.stop());
      return gcli.lookup("profilerStopping");
    }

    return gDevTools.showToolbox(context.environment.target, "jsprofiler")
      .then(stop);
  }
});




gcli.addCommand({
  name: "profiler list",
  description: gcli.lookup("profilerListDesc"),
  returnType: "dom",
  params: [],

  exec: function (args, context) {
    let panel = getPanel(context, "jsprofiler");

    if (!panel) {
      throw gcli.lookup("profilerNotReady");
    }

    let doc = panel.document;
    let div = createXHTMLElement(doc, "div");
    let ol = createXHTMLElement(doc, "ol");

    for ([ uid, profile] of panel.profiles) {
      let li = createXHTMLElement(doc, "li");
      li.textContent = profile.name;
      if (profile.isStarted) {
        li.textContent += " *";
      }
      ol.appendChild(li);
    }

    div.appendChild(ol);
    return div;
  }
});




gcli.addCommand({
  name: "profiler show",
  description: gcli.lookup("profilerShowDesc"),

  params: [
    {
      name: "name",
      type: "string"
    }
  ],

  exec: function (args, context) {
    let panel = getPanel(context, "jsprofiler");

    if (!panel) {
      throw gcli.lookup("profilerNotReady");
    }

    let profile = panel.getProfileByName(args.name);
    if (!profile) {
      throw gcli.lookup("profilerNotFound");
    }

    panel.switchToProfile(profile);
  }
});

function getPanel(context, id) {
  if (context == null) {
    return undefined;
  }

  let toolbox = gDevTools.getToolbox(context.environment.target);
  return toolbox == null ? undefined : toolbox.getPanel(id);
}

function createXHTMLElement(document, tagname) {
  return document.createElementNS("http://www.w3.org/1999/xhtml", tagname);
}