



"use strict";












const {
  VIEW_NODE_SELECTOR_TYPE,
  VIEW_NODE_PROPERTY_TYPE,
  VIEW_NODE_VALUE_TYPE,
  VIEW_NODE_IMAGE_URL_TYPE
} = devtools.require("devtools/styleinspector/style-inspector-overlays");

const PAGE_CONTENT = [
  '<style type="text/css">',
  '  body {',
  '    background: red;',
  '    color: white;',
  '  }',
  '  div {',
  '    background: green;',
  '  }',
  '  div div {',
  '    background-color: yellow;',
  '    background-image: url(chrome://global/skin/icons/warning-64.png);',
  '    color: red;',
  '  }',
  '</style>',
  '<div><div id="testElement">Test element</div></div>'
].join("\n");







const TEST_DATA = [
  {
    desc: "Testing a null node",
    getHoveredNode: function*() {
      return null;
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo, null);
    }
  },
  {
    desc: "Testing a useless node",
    getHoveredNode: function*(view) {
      return view.element;
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo, null);
    }
  },
  {
    desc: "Testing a property name",
    getHoveredNode: function*(view) {
      return getComputedViewProperty(view, "color").nameSpan;
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo.type, VIEW_NODE_PROPERTY_TYPE);
      ok("property" in nodeInfo.value);
      ok("value" in nodeInfo.value);
      is(nodeInfo.value.property, "color");
      is(nodeInfo.value.value, "#F00");
    }
  },
  {
    desc: "Testing a property value",
    getHoveredNode: function*(view) {
      return getComputedViewProperty(view, "color").valueSpan;
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo.type, VIEW_NODE_VALUE_TYPE);
      ok("property" in nodeInfo.value);
      ok("value" in nodeInfo.value);
      is(nodeInfo.value.property, "color");
      is(nodeInfo.value.value, "#F00");
    }
  },
  {
    desc: "Testing an image url",
    getHoveredNode: function*(view) {
      let {valueSpan} = getComputedViewProperty(view, "background-image");
      return valueSpan.querySelector(".theme-link");
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo.type, VIEW_NODE_IMAGE_URL_TYPE);
      ok("property" in nodeInfo.value);
      ok("value" in nodeInfo.value);
      is(nodeInfo.value.property, "background-image");
      is(nodeInfo.value.value, "url(\"chrome://global/skin/icons/warning-64.png\")");
      is(nodeInfo.value.url, "chrome://global/skin/icons/warning-64.png");
    }
  },
  {
    desc: "Testing a matched rule selector (bestmatch)",
    getHoveredNode: function*(view) {
      let content = yield getComputedViewMatchedRules(view, "background-color");
      return content.querySelector(".bestmatch");
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo.type, VIEW_NODE_SELECTOR_TYPE);
      is(nodeInfo.value, "div div");
    }
  },
  {
    desc: "Testing a matched rule selector (matched)",
    getHoveredNode: function*(view) {
      let content = yield getComputedViewMatchedRules(view, "background-color");
      return content.querySelector(".matched");
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo.type, VIEW_NODE_SELECTOR_TYPE);
      is(nodeInfo.value, "div");
    }
  },
  {
    desc: "Testing a matched rule selector (parentmatch)",
    getHoveredNode: function*(view) {
      let content = yield getComputedViewMatchedRules(view, "color");
      return content.querySelector(".parentmatch");
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo.type, VIEW_NODE_SELECTOR_TYPE);
      is(nodeInfo.value, "body");
    }
  },
  {
    desc: "Testing a matched rule value",
    getHoveredNode: function*(view) {
      let content = yield getComputedViewMatchedRules(view, "color");
      return content.querySelector(".other-property-value");
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo.type, VIEW_NODE_VALUE_TYPE);
      is(nodeInfo.value.property, "color");
      is(nodeInfo.value.value, "#F00");
    }
  },
  {
    desc: "Testing a matched rule stylesheet link",
    getHoveredNode: function*(view) {
      let content = yield getComputedViewMatchedRules(view, "color");
      return content.querySelector(".rule-link .theme-link");
    },
    assertNodeInfo: function(nodeInfo) {
      is(nodeInfo, null);
    }
  }
];

let test = asyncTest(function*() {
  yield addTab("data:text/html;charset=utf-8," + PAGE_CONTENT);

  let {inspector, view} = yield openComputedView();
  yield selectNode("#testElement", inspector);

  for (let {desc, getHoveredNode, assertNodeInfo} of TEST_DATA) {
    info(desc);
    let nodeInfo = view.getNodeInfo(yield getHoveredNode(view));
    assertNodeInfo(nodeInfo);
  }
});
