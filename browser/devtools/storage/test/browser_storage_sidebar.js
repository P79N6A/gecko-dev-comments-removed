












const testCases = [
  [["cookies", "sectest1.example.org"], 0, 0],
  ["cs2", 1, 1],
  [null, 0, 0],
  ["cs2", 1, 1],
  ["uc1", 1, 1],
  ["uc1", 0, 1],
  [["localStorage", "http://sectest1.example.org"], 0, 0],
  ["iframe-u-ls1", 1, 1],
  ["iframe-u-ls1", 0, 1],
  [null, 0, 0],
  [["sessionStorage", "http://test1.example.org"], 0, 0],
  ["ss1", 1, 1],
  [null, 0, 0],
  [["indexedDB", "http://test1.example.org"], 0, 0],
  ["idb2", 1, 1],
  [["indexedDB", "http://test1.example.org", "idb2", "obj3"], 0, 0],
  [["indexedDB", "https://sectest1.example.org", "idb-s2"], 0, 0],
  ["obj-s2", 1, 1],
  [null, 0, 0],
  [null, 0, 0],
  ["obj-s2", 1, 1],
  [null, 0, 0],
];

let testSidebar = Task.async(function*() {
  let doc = gPanelWindow.document;
  for (let item of testCases) {
    info("clicking for item " + item);
    if (Array.isArray(item[0])) {
      selectTreeItem(item[0]);
      yield gUI.once("store-objects-updated");
    }
    else if (item[0]) {
      selectTableItem(item[0]);
    }
    else {
      EventUtils.sendKey("ESCAPE", gPanelWindow);
    }
    if (item[1]) {
      yield gUI.once("sidebar-updated");
    }
    is(!item[2], gUI.sidebar.hidden, "Correct visibility state of sidebar");
  }
});

let startTest = Task.async(function*() {
  yield testSidebar();
  finishTests();
});

function test() {
  openTabAndSetupStorage(MAIN_DOMAIN + "storage-listings.html").then(startTest);
}
