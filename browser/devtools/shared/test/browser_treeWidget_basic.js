





const TEST_URI = "data:text/html;charset=utf-8,<head><link rel='stylesheet' " +
  "type='text/css' href='chrome://browser/skin/devtools/common.css'><link " +
  "rel='stylesheet' type='text/css' href='chrome://browser/skin/devtools/widg" +
  "ets.css'></head><body><div></div><span></span></body>";
const {TreeWidget} = devtools.require("devtools/shared/widgets/TreeWidget");

let doc, tree;

function test() {
  waitForExplicitFinish();
  addTab(TEST_URI, () => {
    doc = content.document;
    tree = new TreeWidget(doc.querySelector("div"), {
      defaultType: "store"
    });
    startTests();
  });
}

function endTests() {
  tree.destroy();
  doc = tree = null;
  gBrowser.removeCurrentTab();
  finish();
}

function startTests() {
  populateTree();
  testTreeItemInsertedCorrectly();
  testAPI();
  endTests();
}

function populateTree() {
  tree.add([{
    id: "level1",
    label: "Level 1"
  }, {
    id: "level2-1",
    label: "Level 2"
  }, {
    id: "level3-1",
    label: "Level 3 - Child 1",
    type: "dir"
  }]);
  tree.add(["level1", "level2-1", { id: "level3-2", label: "Level 3 - Child 2"}]);
  tree.add(["level1", "level2-1", { id: "level3-3", label: "Level 3 - Child 3"}]);
  tree.add(["level1", {
    id: "level2-2",
    label: "Level 2.1"
  }, {
    id: "level3-1",
    label: "Level 3.1"
  }]);
  tree.add([{
    id: "level1",
    label: "Level 1"
  }, {
    id: "level2",
    label: "Level 2"
  }, {
    id: "level3",
    label: "Level 3",
    type: "js"
  }]);
  tree.add(["level1.1", "level2", {id: "level3", type: "url"}]);
}




function testTreeItemInsertedCorrectly() {
  is(tree.root.children.children.length, 2, "Number of top level elements match");
  is(tree.root.children.firstChild.lastChild.children.length, 3,
     "Number of first second level elements match");
  is(tree.root.children.lastChild.lastChild.children.length, 1,
     "Number of second second level elements match");

  ok(tree.root.items.has("level1"), "Level1 top level element exists");
  is(tree.root.children.firstChild.dataset.id, JSON.stringify(["level1"]),
     "Data id of first top level element matches");
  is(tree.root.children.firstChild.firstChild.textContent, "Level 1",
     "Text content of first top level element matches");

  ok(tree.root.items.has("level1.1"), "Level1.1 top level element exists");
  is(tree.root.children.firstChild.nextSibling.dataset.id,
     JSON.stringify(["level1.1"]),
     "Data id of second top level element matches");
  is(tree.root.children.firstChild.nextSibling.firstChild.textContent, "level1.1",
     "Text content of second top level element matches");

  
  let node = doc.createElement("div");
  node.textContent = "Foo Bar";
  node.className = "foo bar";
  tree.add([{
    id: "level1.2",
    node: node,
    attachment: {
      foo: "bar"
    }
  }]);

  is(tree.root.children.children.length, 3,
     "Number of top level elements match after update");
  ok(tree.root.items.has("level1.2"), "New level node got added");
  ok(tree.attachments.has(JSON.stringify(["level1.2"])),
     "Attachment is present for newly added node");
  
  is(tree.root.children.firstChild.dataset.id, JSON.stringify(["level1.2"]),
     "Data id of last top level element matches");
  is(tree.root.children.firstChild.firstChild.firstChild, node,
     "Newly added node is inserted at the right location");
}




function testAPI() {
  info("Testing TreeWidget API");
  
  
  ok(!doc.querySelector(".theme-selected"), "Nothing is selected");
  tree.selectItem(["level1"]);
  let node = doc.querySelector(".theme-selected");
  ok(!!node, "Something got selected");
  is(node.parentNode.dataset.id, JSON.stringify(["level1"]),
     "Correct node selected");

  tree.selectItem(["level1", "level2"]);
  let node2 = doc.querySelector(".theme-selected");
  ok(!!node2, "Something is still selected");
  isnot(node, node2, "Newly selected node is different from previous");
  is(node2.parentNode.dataset.id, JSON.stringify(["level1", "level2"]),
     "Correct node selected");

  
  is(tree.selectedItem.length, 2, "Correct length of selected item");
  is(tree.selectedItem[0], "level1", "Correct selected item");
  is(tree.selectedItem[1], "level2", "Correct selected item");

  
  ok(tree.isSelected(["level1", "level2"]), "isSelected works");

  tree.selectedItem = ["level1"];
  let node3 = doc.querySelector(".theme-selected");
  ok(!!node3, "Something is still selected");
  isnot(node2, node3, "Newly selected node is different from previous");
  is(node3, node, "First and third selected nodes should be same");
  is(node3.parentNode.dataset.id, JSON.stringify(["level1"]),
     "Correct node selected");

  
  is(tree.selectedItem.length, 1, "Correct length of selected item");
  is(tree.selectedItem[0], "level1", "Correct selected item");

  
  tree.clearSelection();
  ok(!doc.querySelector(".theme-selected"),
     "Nothing selected after clear selection call")

  
  ok(doc.querySelectorAll("[expanded]").length > 0,
     "Some nodes are expanded");
  tree.collapseAll();
  is(doc.querySelectorAll("[expanded]").length, 0,
     "Nothing is expanded after collapseAll call");
  tree.expandAll();
  is(doc.querySelectorAll("[expanded]").length, 13,
     "All tree items expanded after expandAll call");

  
  tree.selectedItem = ["level1", "level2"];
  ok(tree.isSelected(["level1", "level2"]), "Correct item selected");
  tree.selectNextItem();
  ok(tree.isSelected(["level1", "level2", "level3"]),
     "Correct item selected after selectNextItem call");

  tree.selectNextItem();
  ok(tree.isSelected(["level1", "level2-1"]),
     "Correct item selected after second selectNextItem call");

  tree.selectNextItem();
  ok(tree.isSelected(["level1", "level2-1", "level3-1"]),
     "Correct item selected after third selectNextItem call");

  tree.selectPreviousItem();
  ok(tree.isSelected(["level1", "level2-1"]),
     "Correct item selected after selectPreviousItem call");

  tree.selectPreviousItem();
  ok(tree.isSelected(["level1", "level2", "level3"]),
     "Correct item selected after second selectPreviousItem call");

  
  ok(doc.querySelector("[data-id='" +
       JSON.stringify(["level1", "level2", "level3"]) + "']"),
     "level1-level2-level3 item exists before removing");
  tree.remove(["level1", "level2", "level3"]);
  ok(!doc.querySelector("[data-id='" +
       JSON.stringify(["level1", "level2", "level3"]) + "']"),
     "level1-level2-level3 item does not exist after removing");
  tree.add([{
    id: "level1",
    label: "Level 1"
  }, {
    id: "level2",
    label: "Level 2"
  }, {
    id: "level3",
    label: "Level 3",
    type: "js"
  }]);

  
  is(doc.querySelectorAll("[level='1']").length, 3,
     "Correct number of top level items before clearing");
  tree.clear();
  is(doc.querySelectorAll("[level='1']").length, 0,
     "No top level item after clearing the tree");
}
