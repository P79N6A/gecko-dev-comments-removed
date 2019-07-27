


















const storeItems = [
  [["cookies", "test1.example.org"],
   ["c1", "cs2", "c3", "uc1"]],
  [["cookies", "sectest1.example.org"],
   ["uc1", "cs2", "sc1"]],
  [["localStorage", "http://test1.example.org"],
   ["ls1", "ls2"]],
  [["localStorage", "http://sectest1.example.org"],
   ["iframe-u-ls1"]],
  [["localStorage", "https://sectest1.example.org"],
   ["iframe-s-ls1"]],
  [["sessionStorage", "http://test1.example.org"],
   ["ss1"]],
  [["sessionStorage", "http://sectest1.example.org"],
   ["iframe-u-ss1", "iframe-u-ss2"]],
  [["sessionStorage", "https://sectest1.example.org"],
   ["iframe-s-ss1"]],
  [["indexedDB", "http://test1.example.org"],
   ["idb1", "idb2"]],
  [["indexedDB", "http://test1.example.org", "idb1"],
   ["obj1", "obj2"]],
  [["indexedDB", "http://test1.example.org", "idb2"],
   ["obj3"]],
  [["indexedDB", "http://test1.example.org", "idb1", "obj1"],
   [1, 2, 3]],
  [["indexedDB", "http://test1.example.org", "idb1", "obj2"],
   [1]],
  [["indexedDB", "http://test1.example.org", "idb2", "obj3"],
   []],
  [["indexedDB", "http://sectest1.example.org"],
   []],
  [["indexedDB", "https://sectest1.example.org"],
   ["idb-s1", "idb-s2"]],
  [["indexedDB", "https://sectest1.example.org", "idb-s1"],
   ["obj-s1"]],
  [["indexedDB", "https://sectest1.example.org", "idb-s2"],
   ["obj-s2"]],
  [["indexedDB", "https://sectest1.example.org", "idb-s1", "obj-s1"],
   [6, 7]],
  [["indexedDB", "https://sectest1.example.org", "idb-s2", "obj-s2"],
   [16]],
];




function testTree() {
  let doc = gPanelWindow.document;
  for (let item of storeItems) {
    ok(doc.querySelector("[data-id='" + JSON.stringify(item[0]) + "']"),
       "Tree item " + item[0] + " should be present in the storage tree");
  }
}




let testTables = Task.async(function*() {
  let doc = gPanelWindow.document;
  
  gUI.tree.expandAll();

  
  for (let id of storeItems[0][1]) {
    ok(doc.querySelector(".table-widget-cell[data-id='" + id + "']"),
       "Table item " + id + " should be present");
  }

  
  for (let item of storeItems.slice(1)) {
    selectTreeItem(item[0]);
    yield gUI.once("store-objects-updated");

    
    is(doc.querySelectorAll(
         ".table-widget-wrapper:first-of-type .table-widget-cell"
       ).length, item[1].length, "Number of items in table is correct");

    
    for (let id of item[1]) {
      ok(doc.querySelector(".table-widget-cell[data-id='" + id + "']"),
         "Table item " + id + " should be present");
    }
  }
});

let startTest = Task.async(function*() {
  testTree();
  yield testTables();
  finishTests();
});

function test() {
  openTabAndSetupStorage(MAIN_DOMAIN + "storage-listings.html").then(startTest);
}
