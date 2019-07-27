



"use strict";

const EventEmitter = require("devtools/toolkit/event-emitter");
const eventEmitter = new EventEmitter();
const events = require("sdk/event/core");

const l10n = require("gcli/l10n");
require("devtools/server/actors/inspector");
const { RulersHighlighter, HighlighterEnvironment } =
  require("devtools/server/actors/highlighter");

const highlighters = new WeakMap();
let isRulersVisible = false;

exports.items = [
  
  
  
  {
    name: "rulers",
    runAt: "client",
    description: l10n.lookup("rulersDesc"),
    manual: l10n.lookup("rulersManual"),
    buttonId: "command-button-rulers",
    buttonClass: "command-button command-button-invertable",
    tooltipText: l10n.lookup("rulersTooltip"),
    state: {
      isChecked: () => isRulersVisible,
      onChange: (target, handler) => eventEmitter.on("changed", handler),
      offChange: (target, handler) => eventEmitter.off("changed", handler)
    },
    exec: function*(args, context) {
      let { target } = context.environment;

      
      let response = yield context.updateExec("rulers_server");
      isRulersVisible = response.data;
      eventEmitter.emit("changed", { target });

      
      
      let onNavigate = () => {
        isRulersVisible = false;
        eventEmitter.emit("changed", { target });
      };
      target.off("will-navigate", onNavigate);
      target.once("will-navigate", onNavigate);
    }
  },
  
  
  {
    name: "rulers_server",
    runAt: "server",
    hidden: true,
    exec: function(args, context) {
      let env = context.environment;

      
      
      if (highlighters.has(env.document)) {
        let { highlighter, environment } = highlighters.get(env.document);
        highlighter.destroy();
        environment.destroy();
        return false;
      }

      
      let environment = new HighlighterEnvironment();
      environment.initFromWindow(env.window);
      let highlighter = new RulersHighlighter(environment);

      
      
      highlighters.set(env.document, { highlighter, environment });

      
      
      events.once(highlighter, "destroy", () => {
        if (highlighters.has(env.document)) {
          let { environment } = highlighters.get(env.document);
          environment.destroy();
          highlighters.delete(env.document);
        }
      });

      highlighter.show();
      return true;
    }
  }
];
