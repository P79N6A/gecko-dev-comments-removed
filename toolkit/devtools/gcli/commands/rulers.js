



"use strict";

const EventEmitter = require("devtools/toolkit/event-emitter");
const eventEmitter = new EventEmitter();
const events = require("sdk/event/core");

const gcli = require("gcli/index");
const l10n = require("gcli/l10n");
require("devtools/server/actors/inspector");
const { RulersHighlighter } = require("devtools/server/actors/highlighter");

const highlighters = new WeakMap();

exports.items = [
  {
    name: "rulers",
    description: l10n.lookup("rulersDesc"),
    manual: l10n.lookup("rulersManual"),
    buttonId: "command-button-rulers",
    buttonClass: "command-button command-button-invertable",
    tooltipText: l10n.lookup("rulersTooltip"),
    state: {
      isChecked: function(aTarget) {
        if (aTarget.isLocalTab) {
          let window = aTarget.tab.linkedBrowser.contentWindow;

          if (window) {
            return highlighters.has(window.document);
          }

          return false;
        } else {
          throw new Error("Unsupported target");
        }
      },
      onChange: function(aTarget, aChangeHandler) {
        eventEmitter.on("changed", aChangeHandler);
      },
      offChange: function(aTarget, aChangeHandler) {
        eventEmitter.off("changed", aChangeHandler);
      },
    },
    exec: function(args, context) {
      let env = context.environment;
      let { target } = env;

      if (highlighters.has(env.document)) {
        highlighters.get(env.document).highlighter.destroy();
        return;
      }

      
      
      let tabContext = {
        browser: env.chromeWindow.gBrowser.getBrowserForDocument(env.document),
        window: env.window
      };

      let emitToContext = (type, data) =>
        events.emit(tabContext, type, Object.assign({isTopLevel: true}, data))

      target.once("navigate", emitToContext);
      target.once("will-navigate", emitToContext);

      let highlighter = new RulersHighlighter(tabContext);

      highlighters.set(env.document, { highlighter, listener: emitToContext });

      events.once(highlighter, "destroy", () => {
        if (highlighters.has(env.document)) {
          let { highlighter, listener } = highlighters.get(env.document);

          target.off("navigate", listener);
          target.off("will-navigate", listener);

          highlighters.delete(env.document);
        }

        eventEmitter.emit("changed", { target });
      });

      highlighter.show();

      eventEmitter.emit("changed", { target });
    }
  }
];
