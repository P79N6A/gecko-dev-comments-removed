



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const gcli = require("gcli/index");
require("devtools/server/actors/inspector");
const {HIGHLIGHTER_CLASSES} = require("devtools/server/actors/highlighter");
const {BoxModelHighlighter} = HIGHLIGHTER_CLASSES;


const MAX_HIGHLIGHTED_ELEMENTS = 100;




exports.highlighters = [];




function unhighlightAll() {
  for (let highlighter of exports.highlighters) {
    highlighter.destroy();
  }
  exports.highlighters.length = 0;
}

exports.items = [
  {
    name: "highlight",
    description: gcli.lookup("highlightDesc"),
    manual: gcli.lookup("highlightManual"),
    params: [
      {
        name: "selector",
        type: "nodelist",
        description: gcli.lookup("highlightSelectorDesc"),
        manual: gcli.lookup("highlightSelectorManual")
      },
      {
        group: gcli.lookup("highlightOptionsDesc"),
        params: [
          {
            name: "hideguides",
            type: "boolean",
            description: gcli.lookup("highlightHideGuidesDesc"),
            manual: gcli.lookup("highlightHideGuidesManual")
          },
          {
            name: "showinfobar",
            type: "boolean",
            description: gcli.lookup("highlightShowInfoBarDesc"),
            manual: gcli.lookup("highlightShowInfoBarManual")
          },
          {
            name: "showall",
            type: "boolean",
            description: gcli.lookup("highlightShowAllDesc"),
            manual: gcli.lookup("highlightShowAllManual")
          },
          {
            name: "region",
            type: {
              name: "selection",
              data: ["content", "padding", "border", "margin"]
            },
            description: gcli.lookup("highlightRegionDesc"),
            manual: gcli.lookup("highlightRegionManual"),
            defaultValue: "border"
          },
          {
            name: "fill",
            type: "string",
            description: gcli.lookup("highlightFillDesc"),
            manual: gcli.lookup("highlightFillManual"),
            defaultValue: null
          },
          {
            name: "keep",
            type: "boolean",
            description: gcli.lookup("highlightKeepDesc"),
            manual: gcli.lookup("highlightKeepManual"),
          }
        ]
      }
    ],
    exec: function(args, context) {
      
      if (!args.keep) {
        unhighlightAll();
      }

      let env = context.environment;

      
      env.target.once("navigate", unhighlightAll);

      
      
      let tabContext = {
        browser: env.chromeWindow.gBrowser.getBrowserForDocument(env.document),
        window: env.window
      };

      let i = 0;
      for (let node of args.selector) {
        if (!args.showall && i >= MAX_HIGHLIGHTED_ELEMENTS) {
          break;
        }

        let highlighter = new BoxModelHighlighter(tabContext);
        if (args.fill) {
          highlighter.regionFill[args.region] = args.fill;
        }
        highlighter.show(node, {
          region: args.region,
          hideInfoBar: !args.showinfobar,
          hideGuides: args.hideguides,
          showOnly: args.region
        });
        exports.highlighters.push(highlighter);
        i ++;
      }

      let output = gcli.lookupFormat("highlightOutputConfirm",
        ["" + args.selector.length]);
      if (args.selector.length > i) {
        output = gcli.lookupFormat("highlightOutputMaxReached",
          ["" + args.selector.length, "" + i]);
      }

      return output;
    }
  },
  {
    name: "unhighlight",
    description: gcli.lookup("unhighlightDesc"),
    manual: gcli.lookup("unhighlightManual"),
    exec: unhighlightAll
  }
];
