


"use strict";



const TEST_URL = TEST_URL_ROOT + "doc_inspector_menu.html";

const PASTE_MENU_ITEMS = [
  "node-menu-pasteinnerhtml",
  "node-menu-pasteouterhtml",
  "node-menu-pastebefore",
  "node-menu-pasteafter",
  "node-menu-pastefirstchild",
  "node-menu-pastelastchild",
];

const ALL_MENU_ITEMS = [
  "node-menu-edithtml",
  "node-menu-copyinner",
  "node-menu-copyouter",
  "node-menu-copyuniqueselector",
  "node-menu-copyimagedatauri",
  "node-menu-showdomproperties",
  "node-menu-delete",
  "node-menu-pseudo-hover",
  "node-menu-pseudo-active",
  "node-menu-pseudo-focus",
  "node-menu-scrollnodeintoview"
].concat(PASTE_MENU_ITEMS);

const ITEMS_WITHOUT_SHOWDOMPROPS =
  ALL_MENU_ITEMS.filter(item => item != "node-menu-showdomproperties");

const TEST_CASES = [
  {
    desc: "doctype node with empty clipboard",
    selector: null,
    disabled: ITEMS_WITHOUT_SHOWDOMPROPS,
  },
  {
    desc: "doctype node with html on clipboard",
    clipboardData: "<p>some text</p>",
    clipboardDataType: "html",
    selector: null,
    disabled: ITEMS_WITHOUT_SHOWDOMPROPS,
  },
  {
    desc: "element node HTML on the clipboard",
    clipboardData: "<p>some text</p>",
    clipboardDataType: "html",
    disabled: ["node-menu-copyimagedatauri"],
    selector: "#sensitivity",
  },
  {
    desc: "<html> element",
    clipboardData: "<p>some text</p>",
    clipboardDataType: "html",
    selector: "html",
    disabled: [
      "node-menu-copyimagedatauri",
      "node-menu-pastebefore",
      "node-menu-pasteafter",
      "node-menu-pastefirstchild",
      "node-menu-pastelastchild",
    ],
  },
  {
    desc: "<body> with HTML on clipboard",
    clipboardData: "<p>some text</p>",
    clipboardDataType: "html",
    selector: "body",
    disabled: [
      "node-menu-copyimagedatauri",
      "node-menu-pastebefore",
      "node-menu-pasteafter",
    ]
  },
  {
    desc: "<img> with HTML on clipboard",
    clipboardData: "<p>some text</p>",
    clipboardDataType: "html",
    selector: "img",
    disabled: []
  },
  {
    desc: "<head> with HTML on clipboard",
    clipboardData: "<p>some text</p>",
    clipboardDataType: "html",
    selector: "head",
    disabled: [
      "node-menu-copyimagedatauri",
      "node-menu-pastebefore",
      "node-menu-pasteafter",
    ]
  },
  {
    desc: "<element> with text on clipboard",
    clipboardData: "some text",
    clipboardDataType: undefined,
    selector: "#paste-area",
    disabled: ["node-menu-copyimagedatauri"],
  },
  {
    desc: "<element> with base64 encoded image data uri on clipboard",
    clipboardData:
      "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABC" +
      "AAAAAA6fptVAAAACklEQVQYV2P4DwABAQEAWk1v8QAAAABJRU5ErkJggg==",
    clipboardDataType: undefined,
    selector: "#paste-area",
    disabled: PASTE_MENU_ITEMS.concat(["node-menu-copyimagedatauri"]),
  },
  {
    desc: "<element> with empty string on clipboard",
    clipboardData: "",
    clipboardDataType: undefined,
    selector: "#paste-area",
    disabled: PASTE_MENU_ITEMS.concat(["node-menu-copyimagedatauri"]),
  },
  {
    desc: "<element> with whitespace only on clipboard",
    clipboardData: " \n\n\t\n\n  \n",
    clipboardDataType: undefined,
    selector: "#paste-area",
    disabled: PASTE_MENU_ITEMS.concat(["node-menu-copyimagedatauri"]),
  },
];

let clipboard = require("sdk/clipboard");
registerCleanupFunction(() => {
  clipboard = null;
});

add_task(function *() {
  let { inspector } = yield openInspectorForURL(TEST_URL);
  for (let test of TEST_CASES) {
    let { desc, disabled, selector } = test;

    info(`Test ${desc}`);
    setupClipboard(test.clipboardData, test.clipboardDataType);

    let front = yield getNodeFrontForSelector(selector, inspector);

    info("Selecting the specified node.");
    yield selectNode(front, inspector);

    info("Simulating context menu click on the selected node container.");
    contextMenuClick(getContainerForNodeFront(front, inspector).tagLine);

    for (let menuitem of ALL_MENU_ITEMS) {
      let elt = inspector.panelDoc.getElementById(menuitem);
      let shouldBeDisabled = disabled.indexOf(menuitem) !== -1;
      let isDisabled = elt.hasAttribute("disabled");

      is(isDisabled, shouldBeDisabled,
        `#${menuitem} should be ${shouldBeDisabled ? "disabled" : "enabled"} `);
    }
  }
});





function* getNodeFrontForSelector(selector, inspector) {
  if (selector) {
    info("Retrieving front for selector " + selector);
    return getNodeFront(selector, inspector);
  } else {
    info("Retrieving front for doctype node");
    let {nodes} = yield inspector.walker.children(inspector.walker.rootNode);
    return nodes[0];
  }
}





function setupClipboard(data, type) {
  if (data) {
    info("Populating clipboard with " + type + " data.");
    clipboard.set(data, type);
  } else {
    info("Clearing clipboard.");
    clipboard.set("", "text");
  }
}




function contextMenuClick(element) {
  let evt = element.ownerDocument.createEvent('MouseEvents');
  let button = 2;  

  evt.initMouseEvent('contextmenu', true, true,
       element.ownerDocument.defaultView, 1, 0, 0, 0, 0, false,
       false, false, false, button, null);

  element.dispatchEvent(evt);
}
