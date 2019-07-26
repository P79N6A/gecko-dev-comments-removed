



"use strict";

const { Cc, Ci } = require("chrome");

const { gDevTools } = require("resource:///modules/devtools/gDevTools.jsm");
const promise = require("resource://gre/modules/Promise.jsm").Promise;

const domtemplate = require("gcli/util/domtemplate");
const csscoverage = require("devtools/server/actors/csscoverage");
const l10n = csscoverage.l10n;

const { XPCOMUtils } = require("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Chart", "resource:///modules/devtools/Chart.jsm");




exports.items = [
  {
    name: "csscoverage",
    hidden: true,
    description: l10n.lookup("csscoverageDesc"),
  },
  {
    name: "csscoverage start",
    hidden: true,
    description: l10n.lookup("csscoverageStartDesc"),
    exec: function*(args, context) {
      let usage = yield csscoverage.getUsage(context.environment.target);
      yield usage.start(context.environment.chromeWindow,
                        context.environment.target);
    }
  },
  {
    name: "csscoverage stop",
    hidden: true,
    description: l10n.lookup("csscoverageStopDesc"),
    exec: function*(args, context) {
      let target = context.environment.target;
      let usage = yield csscoverage.getUsage(target);
      yield usage.stop();
      yield gDevTools.showToolbox(target, "styleeditor");
    }
  },
  {
    name: "csscoverage oneshot",
    hidden: true,
    description: l10n.lookup("csscoverageOneShotDesc"),
    exec: function*(args, context) {
      let target = context.environment.target;
      let usage = yield csscoverage.getUsage(target);
      yield usage.oneshot();
      yield gDevTools.showToolbox(target, "styleeditor");
    }
  },
  {
    name: "csscoverage toggle",
    hidden: true,
    description: l10n.lookup("csscoverageToggleDesc"),
    exec: function*(args, context) {
      let target = context.environment.target;
      let usage = yield csscoverage.getUsage(target);

      let running = yield usage.toggle();
      if (running) {
        return l10n.lookup("csscoverageRunningReply");
      }

      yield usage.stop();
      yield gDevTools.showToolbox(target, "styleeditor");
    }
  },
  {
    name: "csscoverage report",
    hidden: true,
    description: l10n.lookup("csscoverageReportDesc"),
    exec: function*(args, context) {
      let usage = yield csscoverage.getUsage(context.environment.target);
      return {
        isTypedData: true,
        type: "csscoveragePageReport",
        data: yield usage.createPageReport()
      };
    }
  },
  {
    item: "converter",
    from: "csscoveragePageReport",
    to: "dom",
    exec: function*(csscoveragePageReport, context) {
      let target = context.environment.target;

      let toolbox = yield gDevTools.showToolbox(target, "styleeditor");
      let panel = toolbox.getCurrentPanel();

      let host = panel._panelDoc.querySelector(".csscoverage-report");
      let templ = panel._panelDoc.querySelector(".csscoverage-template");

      templ = templ.cloneNode(true);
      templ.hidden = false;

      let data = {
        preload: csscoveragePageReport.preload,
        unused: csscoveragePageReport.unused,
        summary: csscoveragePageReport.summary,
        onback: () => {
          
          while (host.hasChildNodes()) {
            host.removeChild(host.firstChild);
          }
          host.hidden = true;
        }
      };

      let addOnClick = rule => {
        rule.onclick = () => {
          panel.selectStyleSheet(rule.url, rule.start.line);
        };
      };

      data.preload.forEach(page => {
        page.rules.forEach(addOnClick);
      });
      data.unused.forEach(page => {
        page.rules.forEach(addOnClick);
      });

      let options = { allowEval: true, stack: "styleeditor.xul" };
      domtemplate.template(templ, data, options);

      while (templ.hasChildNodes()) {
        host.appendChild(templ.firstChild);
      }

      
      let container = host.querySelector(".csscoverage-report-chart");
      let chart = Chart.PieTable(panel._panelDoc, {
        diameter: 200, 
        title: "CSS Usage",
        data: [
          { size: data.summary.preload, label: "Used Preload" },
          { size: data.summary.used, label: "Used" },
          { size: data.summary.unused, label: "Unused" }
        ]
      });
      container.appendChild(chart.node);

      host.hidden = false;
    }
  }
];
