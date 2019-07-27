





const TEST_URI = "data:text/xml;charset=UTF-8,<?xml version='1.0'?>" +
  "<?xml-stylesheet href='chrome://global/skin/global.css'?>" +
  "<?xml-stylesheet href='chrome://browser/skin/devtools/common.css'?>" +
  "<?xml-stylesheet href='chrome://browser/skin/devtools/widgets.css'?>" +
  "<window xmlns='http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul'" +
  " title='Table Widget' width='600' height='500'><box flex='1'/></window>";
const TEST_OPT = "chrome,titlebar,toolbar,centerscreen,resizable,dialog=no";

const {TableWidget} = devtools.require("devtools/shared/widgets/TableWidget");

let doc, table;

function test() {
  waitForExplicitFinish();
  let win = Services.ww.openWindow(null, TEST_URI, "_blank", TEST_OPT, null);

  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);

    waitForFocus(function () {
      doc = win.document;
      table = new TableWidget(doc.querySelector("box"), {
        initialColumns: {
          col1: "Column 1",
          col2: "Column 2",
          col3: "Column 3",
          col4: "Column 4"
        },
        uniqueId: "col1",
        emptyText: "This is dummy empty text",
        highlightUpdated: true,
        removableColumns: true,
        firstColumn: "col4"
      });
      startTests();
    });
  });
}

function endTests() {
  table.destroy();
  doc.defaultView.close();
  doc = table = null;
  finish();
}

function startTests() {
  populateTable();
  testTreeItemInsertedCorrectly();
  testAPI();
  endTests();
}

function populateTable() {
  table.push({
    col1: "id1",
    col2: "value10",
    col3: "value20",
    col4: "value30"
  });
  table.push({
    col1: "id2",
    col2: "value14",
    col3: "value29",
    col4: "value32"
  });
  table.push({
    col1: "id3",
    col2: "value17",
    col3: "value21",
    col4: "value31",
    extraData: "foobar",
    extraData2: 42
  });
  table.push({
    col1: "id4",
    col2: "value12",
    col3: "value26",
    col4: "value33"
  });
  table.push({
    col1: "id5",
    col2: "value19",
    col3: "value26",
    col4: "value37"
  });
  table.push({
    col1: "id6",
    col2: "value15",
    col3: "value25",
    col4: "value37"
  });
  table.push({
    col1: "id7",
    col2: "value18",
    col3: "value21",
    col4: "value36",
    somethingExtra: "Hello World!"
  });
  table.push({
    col1: "id8",
    col2: "value11",
    col3: "value27",
    col4: "value34"
  });

  let span = doc.createElement("span");
  span.textContent = "domnode";

  table.push({
    col1: "id9",
    col2: "value11",
    col3: "value23",
    col4: span
  });
}




function testTreeItemInsertedCorrectly() {
  is(table.tbody.children.length, 4*2 ,
     "4 columns exist");

  
  is(table.tbody.children[0].firstChild.children.length, 9 + 1 ,
     "Correct rows in column 4");
  is(table.tbody.children[0].firstChild.firstChild.value, "Column 4",
     "Correct column header value");

  for (let i = 1; i < 4; i++) {
    is(table.tbody.children[i * 2].firstChild.children.length, 9 + 1 ,
       "Correct rows in column " + i);
    is(table.tbody.children[i * 2].firstChild.firstChild.value, "Column " + i,
       "Correct column header value");
  }
  for (let i = 1; i < 10; i++) {
    is(table.tbody.children[2].firstChild.children[i].value, "id" + i,
     "Correct value in row " + i);
  }

  
  table.clear();
  table.firstColumn = "";
  table.setColumns({
    col1: "Column 1",
    col2: "Column 2",
    col3: "Column 3",
    col4: "Column 4"
  });
  populateTable();

  
  for (let i = 0; i < 4; i++) {
  is(table.tbody.children[i * 2].firstChild.children.length, 9 + 1 ,
     "Correct rows in column " + i);
  is(table.tbody.children[i * 2].firstChild.firstChild.value, "Column " + (i + 1),
     "Correct column header value");
  }
  for (let i = 1; i < 10; i++) {
    is(table.tbody.firstChild.firstChild.children[i].value, "id" + i,
     "Correct value in row " + i);
  }
}




function testAPI() {
  info("Testing TableWidget API");
  
  
  ok(!doc.querySelector(".theme-selected"), "Nothing is selected");
  table.selectRow("id4");
  let node = doc.querySelector(".theme-selected");
  ok(!!node, "Somthing got selected");
  is(node.getAttribute("data-id"), "id4", "Correct node selected");

  table.selectRow("id7");
  let node2 = doc.querySelector(".theme-selected");
  ok(!!node2, "Somthing is still selected");
  isnot(node, node2, "Newly selected node is different from previous");
  is(node2.getAttribute("data-id"), "id7", "Correct node selected");

  
  is(table.selectedRow["col1"], "id7", "Correct result of selectedRow getter");

  
  ok(table.isSelected("id7"), "isSelected with column id works");
  ok(table.isSelected({
    col1: "id7",
    col2: "value18",
    col3: "value21",
    col4: "value36",
    somethingExtra: "Hello World!"
  }), "isSelected with json works");

  table.selectedRow = "id4";
  let node3 = doc.querySelector(".theme-selected");
  ok(!!node3, "Somthing is still selected");
  isnot(node2, node3, "Newly selected node is different from previous");
  is(node3, node, "First and third selected nodes should be same");
  is(node3.getAttribute("data-id"), "id4", "Correct node selected");

  
  is(table.selectedRow["col1"], "id4", "Correct result of selectedRow getter");

  
  table.clearSelection();
  ok(!doc.querySelector(".theme-selected"),
     "Nothing selected after clear selection call");

  
  table.selectedRow = "id7";
  ok(table.isSelected("id7"), "Correct row selected");
  table.selectNextRow();
  ok(table.isSelected("id8"), "Correct row selected after selectNextRow call");

  table.selectNextRow();
  ok(table.isSelected("id9"), "Correct row selected after selectNextRow call");

  table.selectNextRow();
  ok(table.isSelected("id1"),
     "Properly cycled to first row after selectNextRow call on last row");

  table.selectNextRow();
  ok(table.isSelected("id2"), "Correct row selected after selectNextRow call");

  table.selectPreviousRow();
  ok(table.isSelected("id1"), "Correct row selected after selectPreviousRow call");

  table.selectPreviousRow();
  ok(table.isSelected("id9"),
     "Properly cycled to last row after selectPreviousRow call on first row");

  
  ok(doc.querySelector("[data-id='id4']"), "id4 row exists before removal");
  table.remove("id4");
  ok(!doc.querySelector("[data-id='id4']"),
     "id4 row does not exist after removal through id");

  ok(doc.querySelector("[data-id='id6']"), "id6 row exists before removal");
  table.remove({
    col1: "id6",
    col2: "value15",
    col3: "value25",
    col4: "value37"
  });
  ok(!doc.querySelector("[data-id='id6']"),
     "id6 row does not exist after removal through json");

  table.push({
    col1: "id4",
    col2: "value12",
    col3: "value26",
    col4: "value33"
  });
  table.push({
    col1: "id6",
    col2: "value15",
    col3: "value25",
    col4: "value37"
  });

  
  table.selectedIndex = 2;
  ok(table.isSelected("id3"), "Correct row selected by selectedIndex setter");

  table.selectedIndex = 4;
  ok(table.isSelected("id5"), "Correct row selected by selectedIndex setter");

  table.selectRow("id8");
  is(table.selectedIndex, 7, "Correct value of selectedIndex getter");

  
  table.clear();
  is(table.tbody.children.length, 4*2 ,
     "4 columns exist even after clear");
  for (let i = 0; i < 4; i++) {
    is(table.tbody.children[i*2].firstChild.children.length, 1 ,
       "Only header in the column " + i + " after clear call");
    is(table.tbody.children[i*2].firstChild.firstChild.value, "Column " + (i + 1),
       "Correct column header value");
  }

  
  table.setColumns({
    col1: "Foobar",
    col2: "Testing"
  });

  is(table.tbody.children.length, 2*2 ,
     "2 columns exist after setColumn call");
  is(table.tbody.children[0].firstChild.firstChild.value, "Foobar",
     "Correct column header value for first column");
  is(table.tbody.children[2].firstChild.firstChild.value, "Testing",
     "Correct column header value for second column");

  table.setColumns({
    col1: "Column 1",
    col2: "Column 2",
    col3: "Column 3",
    col4: "Column 4"
  });
  is(table.tbody.children.length, 4*2 ,
     "4 columns exist after second setColumn call");

  populateTable();

  
  is(doc.querySelectorAll("[data-id='id4']")[1].value, "value12",
     "Correct value before update");
  table.update({
    col1: "id4",
    col2: "UPDATED",
    col3: "value26",
    col4: "value33"
  });
  is(doc.querySelectorAll("[data-id='id4']")[1].value, "UPDATED",
     "Correct value after update");

  
  
  table.sortBy("col1");
  for (let i = 1; i < 10; i++) {
    is(table.tbody.firstChild.firstChild.children[i].value, "id" + (10 - i),
     "Correct value in row " + i + " after descending sort by on col1");
  }
  
  table.sortBy("col2");
  let cell = table.tbody.children[2].firstChild.children[2];
  checkAscendingOrder(cell);

  
  table.sortBy("col2");
  let cell = table.tbody.children[2].firstChild.lastChild.previousSibling;
  checkDescendingOrder(cell);

  
  table.sortBy("col2");
  let cell = table.tbody.children[2].firstChild.children[2];
  checkAscendingOrder(cell);

  table.clear();
  populateTable();

  
  table.sortBy("col4");
  let cell = table.tbody.children[6].firstChild.children[1];
  is(cell.textContent, "domnode", "DOMNode sorted correctly");
  checkAscendingOrder(cell.nextSibling);

  
  table.sortBy("col4");
  let cell = table.tbody.children[6].firstChild.children[9];
  is(cell.textContent, "domnode", "DOMNode sorted correctly");
  checkDescendingOrder(cell.previousSibling);
}

function checkAscendingOrder(cell) {
  while(cell) {
    let currentCell = cell.value || cell.textContent;
    let prevCell = cell.previousSibling.value || cell.previousSibling.textContent;
    ok(currentCell >= prevCell, "Sorting is in ascending order");
    cell = cell.nextSibling;
  }
}

function checkDescendingOrder(cell) {
  while(cell != cell.parentNode.firstChild) {
    let currentCell = cell.value || cell.textContent;
    let nextCell = cell.nextSibling.value || cell.nextSibling.textContent;
    ok(currentCell >= nextCell, "Sorting is in descending order");
    cell = cell.previousSibling;
  }
}
