var columns_simpletree =
[
  { name: "name", label: "Name", key: true, properties: "one two" },
  { name: "address", label: "Address" }
];

var columns_hiertree =
[
  { name: "name", label: "Name", primary: true, key: true, properties: "one two" },
  { name: "address", label: "Address" },
  { name: "planet", label: "Planet" },
  { name: "gender", label: "Gender", cycler: true }
];








function testtag_tree(treeid, treerowinfoid, seltype, columnstype, testid)
{
  
  
  function preventDefault(event) {
    event.preventDefault();
  }
  document.addEventListener("keypress", preventDefault, false);

  var multiple = (seltype == "multiple");

  var tree = document.getElementById(treeid);
  var treerowinfo = document.getElementById(treerowinfoid);
  var rowInfo;
  if (testid =="tree view")
    rowInfo = getCustomTreeViewCellInfo();
  else
    rowInfo = convertDOMtoTreeRowInfo(treerowinfo, 0, { value: -1 });
  var columnInfo = (columnstype == "simple") ? columns_simpletree : columns_hiertree;

  is(tree.view.selection.currentColumn, null, testid + " initial currentColumn");
  is(tree.selType, seltype == "multiple" ? "" : seltype, testid + " seltype");

  
  
  
  
  if (seltype == "cell")
    tree.view.selection.currentColumn = tree.columns[0];

  testtag_tree_columns(tree, columnInfo, testid);
  testtag_tree_TreeSelection(tree, testid, multiple);
  testtag_tree_TreeSelection_UI(tree, testid, multiple);
  if (seltype == "cell")
    testtag_tree_TreeSelection_UI_cell(tree, testid, rowInfo);

  testtag_tree_TreeView(tree, testid, rowInfo);

  is(tree.editable, false, "tree should not be editable");
  
  
  is(tree.editingRow, -1, testid + " initial editingRow");
  is(tree.editingColumn, null, testid + " initial editingColumn");

  testtag_tree_UI_editing(tree, testid, rowInfo);

  is(tree.editable, false, "tree should not be editable after testtag_tree_UI_editing");
  
  
  is(tree.editingRow, -1, testid + " initial editingRow (continued)");
  is(tree.editingColumn, null, testid + " initial editingColumn (continued)");

  var ecolumn = tree.columns[0];
  ok(!tree.startEditing(1, ecolumn), "non-editable trees shouldn't start editing");
  is(tree.editingRow, -1, testid + " failed startEditing shouldn't set editingRow");
  is(tree.editingColumn, null, testid + " failed startEditing shouldn't set editingColumn");  
  
  tree.editable = true;

  ok(tree.startEditing(1, ecolumn), "startEditing should have returned true");
  is(tree.editingRow, 1, testid + " startEditing editingRow");
  is(tree.editingColumn, ecolumn, testid + " startEditing editingColumn");
  is(tree.getAttribute("editing"), "true", testid + " startEditing editing attribute");

  tree.stopEditing(true);
  is(tree.editingRow, -1, testid + " stopEditing editingRow");
  is(tree.editingColumn, null, testid + " stopEditing editingColumn");
  is(tree.hasAttribute("editing"), false, testid + " stopEditing editing attribute");

  tree.startEditing(-1, ecolumn);
  is(tree.editingRow == -1 && tree.editingColumn == null, true, testid + " startEditing -1 editingRow");
  tree.startEditing(15, ecolumn);
  is(tree.editingRow == -1 && tree.editingColumn == null, true, testid + " startEditing 15 editingRow");
  tree.startEditing(1, null);
  is(tree.editingRow == -1 && tree.editingColumn == null, true, testid + " startEditing null column editingRow");
  tree.startEditing(2, tree.columns[1]);
  is(tree.editingRow == -1 && tree.editingColumn == null, true, testid + " startEditing non editable cell editingRow");

  tree.startEditing(1, ecolumn);
  var inputField = tree.inputField;
  is(inputField instanceof Components.interfaces.nsIDOMXULTextBoxElement, true, testid + "inputField");
  inputField.value = "Changed Value";
  tree.stopEditing(true);
  is(tree.view.getCellText(1, ecolumn), "Changed Value", testid + "edit cell accept");

  
  tree.startEditing(1, ecolumn);
  inputField.value = "Second Value";
  tree.stopEditing(false);
  is(tree.view.getCellText(1, ecolumn), "Changed Value", testid + "edit cell no accept");

  tree.editable = false;

  
  
  if (testid !="tree view")
    testtag_tree_TreeView_rows_sort(tree, testid, rowInfo);

  testtag_tree_wheel(tree);

  document.removeEventListener("keypress", preventDefault, false);

  SimpleTest.finish();
}

function testtag_tree_columns(tree, expectedColumns, testid)
{
  testid += " ";

  var columns = tree.columns;

  is(columns instanceof TreeColumns, true, testid + "columns is a TreeColumns");
  is(columns.count, expectedColumns.length, testid + "TreeColumns count");
  is(columns.length, expectedColumns.length, testid + "TreeColumns length");

  var treecols = tree.getElementsByTagName("treecols")[0];
  var treecol = treecols.getElementsByTagName("treecol");

  var x = 0;
  var primary = null, sorted = null, key = null;
  for (var c = 0; c < expectedColumns.length; c++) {
    var adjtestid = testid + " column " + c + " ";
    var column = columns[c];
    var expectedColumn = expectedColumns[c];
    is(columns.getColumnAt(c), column, adjtestid + "getColumnAt");
    is(columns.getNamedColumn(expectedColumn.name), column, adjtestid + "getNamedColumn");
    is(columns.getColumnFor(treecol[c]), column, adjtestid + "getColumnFor");
    if (expectedColumn.primary)
      primary = column;
    if (expectedColumn.sorted)
      sorted = column;
    if (expectedColumn.key)
      key = column;

    
    
    var adj = 1;
    is(column.x + adj >= x, true, adjtestid + "position is after last column " +
       column.x + "," + column.width + "," + x);
    is(column.width > 0, true, adjtestid + "width is greater than 0");
    x = column.x + column.width;

    
    is(column instanceof TreeColumn, true, adjtestid + "is a TreeColumn");
    is(column.element, treecol[c], adjtestid + "element is treecol");
    is(column.columns, columns, adjtestid + "columns is TreeColumns");
    is(column.id, expectedColumn.name, adjtestid + "name");
    is(column.index, c, adjtestid + "index");
    is(column.primary, primary == column, adjtestid + "column is primary");

    is(column.cycler, "cycler" in expectedColumn && expectedColumn.cycler,
                  adjtestid + "column is cycler");
    is(column.selectable, true, adjtestid + "column is selectable");
    is(column.editable, "editable" in expectedColumn && expectedColumn.editable,
                  adjtestid + "column is editable");

    is(column.type, "type" in expectedColumn ? expectedColumn.type : 1, adjtestid + "type");

    is(column.getPrevious(), c > 0 ? columns[c - 1] : null, adjtestid + "getPrevious");
    is(column.getNext(), c < columns.length - 1 ? columns[c + 1] : null, adjtestid + "getNext");

    
    var properties = tree.view.getColumnProperties(column);
    var expectedProperties = expectedColumn.properties;
    is(properties,  expectedProperties ? expectedProperties : "", adjtestid + "getColumnProperties");
  }

  is(columns.getFirstColumn(), columns[0], testid + "getFirstColumn");
  is(columns.getLastColumn(), columns[columns.length - 1], testid + "getLastColumn");
  is(columns.getPrimaryColumn(), primary, testid + "getPrimaryColumn");
  is(columns.getSortedColumn(), sorted, testid + "getSortedColumn");
  is(columns.getKeyColumn(), key, testid + "getKeyColumn");

  is(columns.getColumnAt(-1), null, testid + "getColumnAt under");
  is(columns.getColumnAt(columns.length), null, testid + "getColumnAt over");
  is(columns.getNamedColumn(""), null, testid + "getNamedColumn null");
  is(columns.getNamedColumn("unknown"), null, testid + "getNamedColumn unknown");
  is(columns.getColumnFor(null), null, testid + "getColumnFor null");
  is(columns.getColumnFor(tree), null, testid + "getColumnFor other");
}

function testtag_tree_TreeSelection(tree, testid, multiple)
{
  testid += " selection ";

  var selection = tree.view.selection;
  is(selection instanceof Components.interfaces.nsITreeSelection, true,
                testid + "selection is a TreeSelection");
  is(selection.single, !multiple, testid + "single");

  testtag_tree_TreeSelection_State(tree, testid + "initial", -1, []);
  is(selection.shiftSelectPivot, -1, testid + "initial shiftSelectPivot");

  selection.currentIndex = 2;
  testtag_tree_TreeSelection_State(tree, testid + "set currentIndex", 2, []);
  tree.currentIndex = 3;
  testtag_tree_TreeSelection_State(tree, testid + "set tree.currentIndex", 3, []);

  
  
  selection.select(1);
  testtag_tree_TreeSelection_State(tree, testid + "select 1", 1, [1]);
  selection.select(3);
  testtag_tree_TreeSelection_State(tree, testid + "select 2", 3, [3]);
  selection.select(3);
  testtag_tree_TreeSelection_State(tree, testid + "select same", 3, [3]);

  selection.currentIndex = 1;
  testtag_tree_TreeSelection_State(tree, testid + "set currentIndex with single selection", 1, [3]);

  tree.currentIndex = 2;
  testtag_tree_TreeSelection_State(tree, testid + "set tree.currentIndex with single selection", 2, [3]);

  
  
  selection.toggleSelect(2);
  testtag_tree_TreeSelection_State(tree, testid + "toggleSelect 1", 2, multiple ? [2, 3] : [3]);
  selection.toggleSelect(2);
  selection.toggleSelect(3);
  testtag_tree_TreeSelection_State(tree, testid + "toggleSelect 2", 3, []);

  
  
  selection.currentIndex = 1;
  selection.selectAll();
  testtag_tree_TreeSelection_State(tree, testid + "selectAll 1", 1, multiple ? [0, 1, 2, 3, 4, 5, 6 , 7] : []);
  selection.toggleSelect(2);
  testtag_tree_TreeSelection_State(tree, testid + "toggleSelect after selectAll", 2,
                                   multiple ? [0, 1, 3, 4, 5, 6, 7] : [2]);
  selection.clearSelection();
  testtag_tree_TreeSelection_State(tree, testid + "clearSelection", 2, []);
  selection.toggleSelect(3);
  selection.toggleSelect(1);
  if (multiple) {
    selection.selectAll();
    testtag_tree_TreeSelection_State(tree, testid + "selectAll 2", 1, [0, 1, 2, 3, 4, 5, 6, 7]);
  }
  selection.currentIndex = 2;
  selection.clearSelection();
  testtag_tree_TreeSelection_State(tree, testid + "clearSelection after selectAll", 2, []);

  
  

  is(selection.shiftSelectPivot, -1, testid + "shiftSelectPivot set to -1");

  
  
  selection.rangedSelect(1, 3, false);
  testtag_tree_TreeSelection_State(tree, testid + "rangedSelect no augment",
                                   multiple ? 3 : 2, multiple ? [1, 2, 3] : []);
  is(selection.shiftSelectPivot, multiple ? 1 : -1,
                testid + "shiftSelectPivot after rangedSelect no augment");
  if (multiple) {
    selection.select(1);
    selection.rangedSelect(0, 2, true);
    testtag_tree_TreeSelection_State(tree, testid + "rangedSelect augment", 2, [0, 1, 2]);
    is(selection.shiftSelectPivot, 0, testid + "shiftSelectPivot after rangedSelect augment");

    selection.clearRange(1, 3);
    testtag_tree_TreeSelection_State(tree, testid + "rangedSelect augment", 3, [0]);

    
    selection.rangedSelect(3, 1, false);
    testtag_tree_TreeSelection_State(tree, testid + "rangedSelect reverse", 1, [1, 2, 3]);
    is(selection.shiftSelectPivot, 3, testid + "shiftSelectPivot after rangedSelect reverse");

    
    selection.currentIndex = 0;
    testtag_tree_TreeSelection_State(tree, testid + "currentIndex with range selection", 0, [1, 2, 3]);
  }

  
  selection.rangedSelect(2, 2, false);
  testtag_tree_TreeSelection_State(tree, testid + "rangedSelect one row", 2, [2]);
  is(selection.shiftSelectPivot, 2, testid + "shiftSelectPivot after selecting one row");

  if (multiple) {
    selection.rangedSelect(2, 3, true);

    
    selection.rangedSelect(-1, 0, true);
    testtag_tree_TreeSelection_State(tree, testid + "rangedSelect -1 existing selection", 0, [0, 1, 2, 3]);
    is(selection.shiftSelectPivot, 2, testid + "shiftSelectPivot after -1 existing selection");

    selection.currentIndex = 2;
    selection.rangedSelect(-1, 0, false);
    testtag_tree_TreeSelection_State(tree, testid + "rangedSelect -1 from currentIndex", 0, [0, 1, 2]);
    is(selection.shiftSelectPivot, 2, testid + "shiftSelectPivot -1 from currentIndex");
  }

  







}

function testtag_tree_TreeSelection_UI(tree, testid, multiple)
{
  testid += " selection UI ";

  var selection = tree.view.selection;
  selection.clearSelection();
  selection.currentIndex = 0;
  tree.focus();

  var keydownFired = 0;
  var keypressFired = 0;
  function keydownListener(event)
  {
    keydownFired++;
  }
  function keypressListener(event) {
    keypressFired++;
  }

  
  
  
  
  
  window.addEventListener("keydown", keydownListener, false);
  window.addEventListener("keypress", keypressListener, false);

  synthesizeKeyExpectEvent("VK_DOWN", {}, tree, "!select", "key down");
  testtag_tree_TreeSelection_State(tree, testid + "key down", 1, [1], 0);

  synthesizeKeyExpectEvent("VK_UP", {}, tree, "!select", "key up");
  testtag_tree_TreeSelection_State(tree, testid + "key up", 0, [0], 0);

  synthesizeKeyExpectEvent("VK_UP", {}, tree, "!select", "key up at start");
  testtag_tree_TreeSelection_State(tree, testid + "key up at start", 0, [0], 0);

  
  
  selection.select(7);
  synthesizeKeyExpectEvent("VK_DOWN", {}, tree, "!select", "key down at end");
  testtag_tree_TreeSelection_State(tree, testid + "key down at end", 7, [7], 0);

  
  tree.treeBoxObject.scrollToRow(4);
  selection.select(4);
  synthesizeKeyExpectEvent("VK_UP", {}, tree, "!select", "key up with scroll");
  is(tree.treeBoxObject.getFirstVisibleRow(), 3, testid + "key up with scroll");

  tree.treeBoxObject.scrollToRow(0);
  selection.select(3);
  synthesizeKeyExpectEvent("VK_DOWN", {}, tree, "!select", "key down with scroll");
  is(tree.treeBoxObject.getFirstVisibleRow(), 1, testid + "key down with scroll");
  
  
  
  
  tree.treeBoxObject.scrollToRow(0);
  selection.select(1);
  synthesizeKeyExpectEvent("VK_DOWN", { accelKey: true }, tree, "!select", "key down with accel");
  testtag_tree_TreeSelection_State(tree, testid + "key down with accel", multiple ? 2 : 1, [1]);
  if (!multiple)
    is(tree.treeBoxObject.getFirstVisibleRow(), 1, testid + "key down with accel and scroll");

  tree.treeBoxObject.scrollToRow(4);
  selection.select(4);
  synthesizeKeyExpectEvent("VK_UP", { accelKey: true }, tree, "!select", "key up with accel");
  testtag_tree_TreeSelection_State(tree, testid + "key up with accel", multiple ? 3 : 4, [4]);
  if (!multiple)
    is(tree.treeBoxObject.getFirstVisibleRow(), 3, testid + "key up with accel and scroll");

  
  
  for (var t = 0; t < 3; t++) {
    var testidmod = (t == 2) ? " with accel" : (t == 1) ? " rev" : "";
    var keymod = (t == 2) ? { accelKey: true } : { };

    var moveselection = tree.pageUpOrDownMovesSelection;
    if (t == 2)
      moveselection = !moveselection;

    tree.treeBoxObject.scrollToRow(4);
    selection.currentIndex = 6;
    selection.select(6);
    var expected = moveselection ? 4 : 6;
    synthesizeKeyExpectEvent("VK_PAGE_UP", keymod, tree, "!select", "key page up");
    testtag_tree_TreeSelection_State(tree, testid + "key page up" + testidmod,
                                     expected, [expected], moveselection ? 4 : 0);

    expected = moveselection ? 0 : 6;
    synthesizeKeyExpectEvent("VK_PAGE_UP", keymod, tree, "!select", "key page up again");
    testtag_tree_TreeSelection_State(tree, testid + "key page up again" + testidmod,
                                     expected, [expected], 0);

    expected = moveselection ? 0 : 6;
    synthesizeKeyExpectEvent("VK_PAGE_UP", keymod, tree, "!select", "key page up at start");
    testtag_tree_TreeSelection_State(tree, testid + "key page up at start" + testidmod,
                                     expected, [expected], 0);

    tree.treeBoxObject.scrollToRow(0);
    selection.currentIndex = 1;
    selection.select(1);
    expected = moveselection ? 3 : 1;
    synthesizeKeyExpectEvent("VK_PAGE_DOWN", keymod, tree, "!select", "key page down");
    testtag_tree_TreeSelection_State(tree, testid + "key page down" + testidmod,
                                     expected, [expected], moveselection ? 0 : 4);

    expected = moveselection ? 7 : 1;
    synthesizeKeyExpectEvent("VK_PAGE_DOWN", keymod, tree, "!select", "key page down again");
    testtag_tree_TreeSelection_State(tree, testid + "key page down again" + testidmod,
                                     expected, [expected], 4);

    expected = moveselection ? 7 : 1;
    synthesizeKeyExpectEvent("VK_PAGE_DOWN", keymod, tree, "!select", "key page down at start");
    testtag_tree_TreeSelection_State(tree, testid + "key page down at start" + testidmod,
                                     expected, [expected], 4);

    if (t < 2)
      tree.pageUpOrDownMovesSelection = !tree.pageUpOrDownMovesSelection;
  }

  tree.treeBoxObject.scrollToRow(4);
  selection.select(6);
  synthesizeKeyExpectEvent("VK_HOME", {}, tree, "!select", "key home");
  testtag_tree_TreeSelection_State(tree, testid + "key home", 0, [0], 0);

  tree.treeBoxObject.scrollToRow(0);
  selection.select(1);
  synthesizeKeyExpectEvent("VK_END", {}, tree, "!select", "key end");
  testtag_tree_TreeSelection_State(tree, testid + "key end", 7, [7], 4);

  
  tree.treeBoxObject.scrollToRow(4);
  selection.select(6);
  synthesizeKeyExpectEvent("VK_HOME", { accelKey: true }, tree, "!select", "key home with accel");
  testtag_tree_TreeSelection_State(tree, testid + "key home with accel", multiple ? 0 : 6, [6], 0);

  tree.treeBoxObject.scrollToRow(0);
  selection.select(1);
  synthesizeKeyExpectEvent("VK_END", { accelKey: true }, tree, "!select", "key end with accel");
  testtag_tree_TreeSelection_State(tree, testid + "key end with accel", multiple ? 7 : 1, [1], 4);

  
  selection.select(1);
  var eventExpected = multiple ? "select" : "!select";
  synthesizeKeyExpectEvent("VK_DOWN", { shiftKey: true }, tree, eventExpected, "key shift down to select");
  testtag_tree_TreeSelection_State(tree, testid + "key shift down to select",
                                   multiple ? 2 : 1, multiple ? [1, 2] : [1]);
  is(selection.shiftSelectPivot, multiple ? 1 : -1,
                testid + "key shift down to select shiftSelectPivot");
  synthesizeKeyExpectEvent("VK_UP", { shiftKey: true }, tree, eventExpected, "key shift up to unselect");
  testtag_tree_TreeSelection_State(tree, testid + "key shift up to unselect", 1, [1]);
  is(selection.shiftSelectPivot, multiple ? 1 : -1,
                testid + "key shift up to unselect shiftSelectPivot");
  if (multiple) {
    synthesizeKeyExpectEvent("VK_UP", { shiftKey: true }, tree, "select", "key shift up to select");
    testtag_tree_TreeSelection_State(tree, testid + "key shift up to select", 0, [0, 1]);
    is(selection.shiftSelectPivot, 1, testid + "key shift up to select shiftSelectPivot");
    synthesizeKeyExpectEvent("VK_DOWN", { shiftKey: true }, tree, "select", "key shift down to unselect");
    testtag_tree_TreeSelection_State(tree, testid + "key shift down to unselect", 1, [1]);
    is(selection.shiftSelectPivot, 1, testid + "key shift down to unselect shiftSelectPivot");
  }

  
  
  
  var lastidx = tree.view.rowCount - 1;
  for (var t = 0; t < 2; t++) {
    var testidmod = (t == 0) ? "" : " rev";

    
    
    
    tree.treeBoxObject.scrollToRow(lastidx - 3);
    selection.currentIndex = 6;
    selection.select(6);
    synthesizeKeyExpectEvent("VK_PAGE_UP", { shiftKey: true }, tree, eventExpected, "key shift page up");
    testtag_tree_TreeSelection_State(tree, testid + "key shift page up" + testidmod,
                                     multiple ? 4 : 6, multiple ? [4, 5, 6] : [6]);
    if (multiple) {
      synthesizeKeyExpectEvent("VK_PAGE_UP", { shiftKey: true }, tree, "select", "key shift page up again");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page up again" + testidmod,
                                       0, [0, 1, 2, 3, 4, 5, 6]);
      
      synthesizeKeyExpectEvent("VK_PAGE_UP", { shiftKey: true }, tree, "!select", "key shift page up at start");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page up at start" + testidmod,
                                       0, [0, 1, 2, 3, 4, 5, 6]);
      
      synthesizeKeyExpectEvent("VK_PAGE_DOWN", { shiftKey: true }, tree, "select", "key shift page down deselect");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page down deselect" + testidmod,
                                       3, [3, 4, 5, 6]);
    }

    tree.treeBoxObject.scrollToRow(1);
    selection.currentIndex = 2;
    selection.select(2);
    synthesizeKeyExpectEvent("VK_PAGE_DOWN", { shiftKey: true }, tree, eventExpected, "key shift page down");
    testtag_tree_TreeSelection_State(tree, testid + "key shift page down" + testidmod,
                                     multiple ? 4 : 2, multiple ? [2, 3, 4] : [2]);
    if (multiple) {
      synthesizeKeyExpectEvent("VK_PAGE_DOWN", { shiftKey: true }, tree, "select", "key shift page down again");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page down again" + testidmod,
                                       7, [2, 3, 4, 5, 6, 7]);
      synthesizeKeyExpectEvent("VK_PAGE_DOWN", { shiftKey: true }, tree, "!select", "key shift page down at start");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page down at start" + testidmod,
                                       7, [2, 3, 4, 5, 6, 7]);
      synthesizeKeyExpectEvent("VK_PAGE_UP", { shiftKey: true }, tree, "select", "key shift page up deselect");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page up deselect" + testidmod,
                                       4, [2, 3, 4]);
    }

    
    
    if (multiple) {
      tree.treeBoxObject.scrollToRow(3);
      selection.currentIndex = 1;
      selection.select(1);
      synthesizeKeyExpectEvent("VK_PAGE_DOWN", { shiftKey: true }, tree, eventExpected,
                               "key shift page down with view scrolled down");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page down with view scrolled down" + testidmod,
                                       6, [1, 2, 3, 4, 5, 6], 3);

      tree.treeBoxObject.scrollToRow(2);
      selection.currentIndex = 6;
      selection.select(6);
      synthesizeKeyExpectEvent("VK_PAGE_UP", { shiftKey: true }, tree, eventExpected,
                               "key shift page up with view scrolled up");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page up with view scrolled up" + testidmod,
                                       2, [2, 3, 4, 5, 6], 2);

      tree.treeBoxObject.scrollToRow(2);
      selection.currentIndex = 0;
      selection.select(0);
      
      synthesizeKeyExpectEvent("VK_PAGE_UP", { shiftKey: true }, tree, "!select",
                               "key shift page up at start with view scrolled down");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page up at start with view scrolled down" + testidmod,
                                       0, [0], 0);

      tree.treeBoxObject.scrollToRow(0);
      selection.currentIndex = 7;
      selection.select(7);
      
      synthesizeKeyExpectEvent("VK_PAGE_DOWN", { shiftKey: true }, tree, "!select",
                               "key shift page down at end with view scrolled up");
      testtag_tree_TreeSelection_State(tree, testid + "key shift page down at end with view scrolled up" + testidmod,
                                       7, [7], 4);
    }

    tree.pageUpOrDownMovesSelection = !tree.pageUpOrDownMovesSelection;
  }

  tree.treeBoxObject.scrollToRow(4);
  selection.select(5);
  synthesizeKeyExpectEvent("VK_HOME", { shiftKey: true }, tree, eventExpected, "key shift home");
  testtag_tree_TreeSelection_State(tree, testid + "key shift home",
                                   multiple ? 0 : 5, multiple ? [0, 1, 2, 3, 4, 5] : [5], multiple ? 0 : 4);

  tree.treeBoxObject.scrollToRow(0);
  selection.select(3);
  synthesizeKeyExpectEvent("VK_END", { shiftKey: true }, tree, eventExpected, "key shift end");
  testtag_tree_TreeSelection_State(tree, testid + "key shift end",
                                   multiple ? 7 : 3, multiple ? [3, 4, 5, 6, 7] : [3], multiple ? 4 : 0);

  
  selection.select(2);
  selection.currentIndex = 4;
  synthesizeKeyExpectEvent(" ", {}, tree, "select", "key space on");
  
  testtag_tree_TreeSelection_State(tree, testid + "key space on", 4, multiple ? [2, 4] : [2]);

  if (multiple) {
    synthesizeKeyExpectEvent(" ", { accelKey: true }, tree, "select", "key space off");
    testtag_tree_TreeSelection_State(tree, testid + "key space off", 4, [2]);
  }

  
  tree.treeBoxObject.scrollToRow(0);
  selection.select(2);
  selection.currentIndex = 2;
  if (0) { 
    mouseOnCell(tree, 1, tree.columns[1], "mouse on row");
    testtag_tree_TreeSelection_State(tree, testid + "mouse on row", 1, [1], 0,
                                     tree.selType == "cell" ? tree.columns[1] : null);
  }

  
  sendKey("HOME");

  window.removeEventListener("keydown", keydownListener, false);
  window.removeEventListener("keypress", keypressListener, false);
  is(keydownFired, multiple ? 63 : 40, "keydown event wasn't fired properly");
  is(keypressFired, multiple ? 2 : 1, "keypress event wasn't fired properly");
}

function testtag_tree_UI_editing(tree, testid, rowInfo)
{
  testid += " editing UI ";

  
  var ecolumn = tree.columns[0];
  var rowIndex = 2;
  var inputField = tree.inputField;

  
  var wasEditable = tree.editable;
  if (!wasEditable)
    tree.editable = true;

  
  var row = rowInfo.rows[rowIndex];
  var wasOpen = null;
  if (tree.view.isContainer(row))
    wasOpen = tree.view.isContainerOpen(row);

  
  if (tree.selType == "cell")
  {
    tree.stopEditing(false);
    ok(!tree.editingColumn, "Should not be editing tree cell now");
    tree.view.selection.currentColumn = ecolumn;
    tree.currentIndex = rowIndex;

    const isMac = (navigator.platform.indexOf("Mac") >= 0);
    const StartEditingKey = isMac ? "RETURN" : "F2";
    sendKey(StartEditingKey);
    is(tree.editingColumn, ecolumn, "Should be editing tree cell now");
    sendKey("ESCAPE");
    ok(!tree.editingColumn, "Should not be editing tree cell now");
    is(tree.currentIndex, rowIndex, "Current index should not have changed");
    is(tree.view.selection.currentColumn, ecolumn, "Current column should not have changed");
  }

  mouseDblClickOnCell(tree, rowIndex, ecolumn, testid + "edit on double click");
  is(tree.editingColumn, ecolumn, testid + "editing column");
  is(tree.editingRow, rowIndex, testid + "editing row");

  
  if (wasOpen != null)
    is(tree.view.isContainerOpen(row), wasOpen, testid + "opened container node on edit");

  
  if (!wasEditable)
    tree.editable = false;

  var ci = tree.currentIndex;

  
  var testKey = function(key) {
    synthesizeKeyExpectEvent(key, {}, tree, "!select", "key " + key + " with editing");
    is(tree.editingRow == rowIndex && tree.editingColumn == ecolumn && tree.currentIndex == ci,
                  true, testid + "key " + key + " while editing");
  }

  testKey("VK_DOWN");
  testKey("VK_UP");
  testKey("VK_PAGE_DOWN");
  testKey("VK_PAGE_UP");
  testKey("VK_HOME");
  testKey("VK_END");

  
  
  
  
  

  
  tree.stopEditing(false);
}

function testtag_tree_TreeSelection_UI_cell(tree, testid, rowInfo)
{
  testid += " selection UI cell ";

  var columns = tree.columns;
  var firstcolumn = columns[0];
  var secondcolumn = columns[1];
  var lastcolumn = columns[columns.length - 1];
  var secondlastcolumn = columns[columns.length - 2];
  var selection = tree.view.selection;

  selection.clearSelection();
  selection.currentIndex = -1;
  selection.currentColumn = firstcolumn;
  is(selection.currentColumn, firstcolumn, testid + " first currentColumn");

  
  synthesizeKeyExpectEvent("VK_RIGHT", {}, tree, "!select", "key right no selection");
  testtag_tree_TreeSelection_State(tree, testid + "key right no selection", -1, [], null, firstcolumn);

  selection.currentColumn = secondcolumn;
  synthesizeKeyExpectEvent("VK_LEFT", {}, tree, "!select", "key left no selection");
  testtag_tree_TreeSelection_State(tree, testid + "key left no selection", -1, [], null, secondcolumn);

  selection.select(2);
  selection.currentIndex = 2;
  if (0) { 
    mouseOnCell(tree, 1, secondlastcolumn, "mouse on cell");
    testtag_tree_TreeSelection_State(tree, testid + "mouse on cell", 1, [1], null, secondlastcolumn);
  }

  tree.focus();
 
  
  tree.treeBoxObject.scrollToRow(0);
  selection.select(1);
  selection.currentIndex = 1;
  selection.currentColumn = secondcolumn;
  synthesizeKeyExpectEvent("VK_LEFT", {}, tree, "!select", "key left in second column");
  testtag_tree_TreeSelection_State(tree, testid + "key left in second column", 1, [1], 0, firstcolumn);

  synthesizeKeyExpectEvent("VK_LEFT", {}, tree, "!select", "key left in first column");
  testtag_tree_TreeSelection_State(tree, testid + "key left in first column", 1, [1], 0, firstcolumn);

  selection.currentColumn = secondlastcolumn;
  synthesizeKeyExpectEvent("VK_RIGHT", {}, tree, "!select", "key right in second last column");
  testtag_tree_TreeSelection_State(tree, testid + "key right in second last column", 1, [1], 0, lastcolumn);

  synthesizeKeyExpectEvent("VK_RIGHT", {}, tree, "!select", "key right in last column");
  testtag_tree_TreeSelection_State(tree, testid + "key right in last column", 1, [1], 0, lastcolumn);

  synthesizeKeyExpectEvent("VK_UP", {}, tree, "!select", "key up in second row");
  testtag_tree_TreeSelection_State(tree, testid + "key up in second row", 0, [0], 0, lastcolumn);

  synthesizeKeyExpectEvent("VK_UP", {}, tree, "!select", "key up in first row");
  testtag_tree_TreeSelection_State(tree, testid + "key up in first row", 0, [0], 0, lastcolumn);

  synthesizeKeyExpectEvent("VK_DOWN", {}, tree, "!select", "key down in first row");
  testtag_tree_TreeSelection_State(tree, testid + "key down in first row", 1, [1], 0, lastcolumn);

  var lastidx = tree.view.rowCount - 1;
  tree.treeBoxObject.scrollToRow(lastidx - 3);
  selection.select(lastidx);
  selection.currentIndex = lastidx;
  synthesizeKeyExpectEvent("VK_DOWN", {}, tree, "!select", "key down in last row");
  testtag_tree_TreeSelection_State(tree, testid + "key down in last row", lastidx, [lastidx], lastidx - 3, lastcolumn);

  synthesizeKeyExpectEvent("VK_HOME", {}, tree, "!select", "key home");
  testtag_tree_TreeSelection_State(tree, testid + "key home", 0, [0], 0, lastcolumn);

  synthesizeKeyExpectEvent("VK_END", {}, tree, "!select", "key end");
  testtag_tree_TreeSelection_State(tree, testid + "key end", lastidx, [lastidx], lastidx - 3, lastcolumn);

  for (var t = 0; t < 2; t++) {
    var testidmod = (t == 0) ? "" : " rev";

    
    
    tree.treeBoxObject.scrollToRow(lastidx - 3);
    selection.select(lastidx);
    selection.currentIndex = lastidx;
    var expectedrow = tree.pageUpOrDownMovesSelection ? lastidx - 3 : lastidx;
    synthesizeKeyExpectEvent("VK_PAGE_UP", {}, tree, "!select", "key page up");
    testtag_tree_TreeSelection_State(tree, testid + "key page up" + testidmod,
                                     expectedrow, [expectedrow],
                                     tree.pageUpOrDownMovesSelection ? lastidx - 3 : 0, lastcolumn);

    tree.treeBoxObject.scrollToRow(1);
    selection.select(1);
    selection.currentIndex = 1;
    var expectedrow = tree.pageUpOrDownMovesSelection ? 4 : 1;
    synthesizeKeyExpectEvent("VK_PAGE_DOWN", {}, tree, "!select", "key page down");
    testtag_tree_TreeSelection_State(tree, testid + "key page down" + testidmod,
                                     expectedrow, [expectedrow],
                                     tree.pageUpOrDownMovesSelection ? 1 : lastidx - 3, lastcolumn);

    tree.pageUpOrDownMovesSelection = !tree.pageUpOrDownMovesSelection;
  }

  
  secondcolumn.element.setAttribute("selectable", "false");
  secondcolumn.invalidate();
  is(secondcolumn.selectable, false, testid + "set selectable attribute");

  if (columns.length >= 3) {
    selection.select(3);
    selection.currentIndex = 3;
    
    selection.currentColumn = firstcolumn;
    synthesizeKeyExpectEvent("VK_RIGHT", {}, tree, "!select", "key right unselectable column");
    testtag_tree_TreeSelection_State(tree, testid + "key right unselectable column",
                                     3, [3], null, secondcolumn.getNext());

    synthesizeKeyExpectEvent("VK_LEFT", {}, tree, "!select", "key left unselectable column");
    testtag_tree_TreeSelection_State(tree, testid + "key left unselectable column",
                                     3, [3], null, firstcolumn);
  }

  secondcolumn.element.removeAttribute("selectable");
  secondcolumn.invalidate();
  is(secondcolumn.selectable, true, testid + "clear selectable attribute");

  
  selection.currentColumn = secondcolumn;
  firstcolumn.element.setAttribute("selectable", "false");
  firstcolumn.invalidate();
  synthesizeKeyExpectEvent("VK_LEFT", {}, tree, "!select", "key left unselectable first column");
  testtag_tree_TreeSelection_State(tree, testid + "key left unselectable first column",
                                   3, [3], null, secondcolumn);
  firstcolumn.element.removeAttribute("selectable");
  firstcolumn.invalidate();

  
  selection.currentColumn = secondlastcolumn;
  lastcolumn.element.setAttribute("selectable", "false");
  lastcolumn.invalidate();
  synthesizeKeyExpectEvent("VK_RIGHT", {}, tree, "!select", "key right unselectable last column");
  testtag_tree_TreeSelection_State(tree, testid + "key right unselectable last column",
                                   3, [3], null, secondlastcolumn);
  lastcolumn.element.removeAttribute("selectable");
  lastcolumn.invalidate();

  
  if (!rowInfo.rows[4].cells[1].selectable && columns.length >= 3) {
    
    selection.select(4);
    selection.currentIndex = 4;

    selection.currentColumn = firstcolumn;
    synthesizeKeyExpectEvent("VK_RIGHT", {}, tree, "!select", "key right unselectable cell");
    testtag_tree_TreeSelection_State(tree, testid + "key right unselectable cell",
                                     4, [4], null, secondcolumn.getNext());

    synthesizeKeyExpectEvent("VK_LEFT", {}, tree, "!select", "key left unselectable cell");
    testtag_tree_TreeSelection_State(tree, testid + "key left unselectable cell",
                                     4, [4], null, firstcolumn);

    tree.treeBoxObject.scrollToRow(1);
    selection.select(3);
    selection.currentIndex = 3;
    selection.currentColumn = secondcolumn;

    synthesizeKeyExpectEvent("VK_DOWN", {}, tree, "!select", "key down unselectable cell");
    testtag_tree_TreeSelection_State(tree, testid + "key down unselectable cell",
                                     5, [5], 2, secondcolumn);

    tree.treeBoxObject.scrollToRow(4);
    synthesizeKeyExpectEvent("VK_UP", {}, tree, "!select", "key up unselectable cell");
    testtag_tree_TreeSelection_State(tree, testid + "key up unselectable cell",
                                     3, [3], 3, secondcolumn);
  }

  
  sendKey("HOME");
}

function testtag_tree_TreeView(tree, testid, rowInfo)
{
  testid += " view ";

  var columns = tree.columns;
  var view = tree.view;

  is(view instanceof Components.interfaces.nsITreeView, true, testid + "view is a TreeView");
  is(view.rowCount, rowInfo.rows.length, testid + "rowCount");

  testtag_tree_TreeView_rows(tree, testid, rowInfo, 0);

  
  view.setCellText(0, columns[1], "Changed Value");
  is(view.getCellText(0, columns[1]), "Changed Value", "setCellText");

  view.setCellValue(1, columns[0], "Another Changed Value");
  is(view.getCellValue(1, columns[0]), "Another Changed Value", "setCellText");
}

function testtag_tree_TreeView_rows(tree, testid, rowInfo, startRow)
{
  var r;
  var columns = tree.columns;
  var view = tree.view;
  var length = rowInfo.rows.length;

  
  var checkRowMethods =
  {
    isContainer: function(row) { return row.container },
    isContainerOpen: function(row) { return false },
    isContainerEmpty: function(row) { return (row.children != null && row.children.rows.length == 0) },
    isSeparator: function(row) { return row.separator },
    getRowProperties: function(row) { return row.properties },
    getLevel: function(row) { return row.level },
    getParentIndex: function(row) { return row.parent },
    hasNextSibling: function(row) { return r < startRow + length - 1; }
  };

  var checkCellMethods =
  {
    getCellText: function(row, cell) { return cell.label },
    getCellValue: function(row, cell) { return cell.value },
    getCellProperties: function(row, cell) { return cell.properties },
    isEditable: function(row, cell) { return cell.editable },
    isSelectable: function(row, cell) { return cell.selectable },
    getImageSrc: function(row, cell) { return cell.image },
    getProgressMode: function(row, cell) { return cell.mode }
  };

  var failedMethods = { };
  var checkMethod, actual, expected;
  var containerInfo = null;
  var toggleOpenStateOK = true;

  for (r = startRow; r < length; r++) {
    var row = rowInfo.rows[r];
    for (var c = 0; c < row.cells.length; c++) {
      var cell = row.cells[c];

      for (checkMethod in checkCellMethods) {
        expected = checkCellMethods[checkMethod](row, cell);
        actual = view[checkMethod](r, columns[c]);
        if (actual !== expected) {
          failedMethods[checkMethod] = true;
          is(actual, expected, testid + "row " + r + " column " + c + " " + checkMethod + " is incorrect");
        }
      }
    }

    
    for (checkMethod in checkRowMethods) {
      expected = checkRowMethods[checkMethod](row, r);
      if (checkMethod == "hasNextSibling") {
        actual = view[checkMethod](r, r);
      }
      else {
        actual = view[checkMethod](r);
      }
      if (actual !== expected) {
        failedMethods[checkMethod] = true;
        is(actual, expected, testid + "row " + r + " " + checkMethod + " is incorrect");
      }
    }
















  }

  for (var failedMethod in failedMethods) {
    if (failedMethod in checkRowMethods)
      delete checkRowMethods[failedMethod];
    if (failedMethod in checkCellMethods)
      delete checkCellMethods[failedMethod];
  }

  for (checkMethod in checkRowMethods)
    is(checkMethod + " ok", checkMethod + " ok", testid + checkMethod);
  for (checkMethod in checkCellMethods)
    is(checkMethod + " ok", checkMethod + " ok", testid + checkMethod);
  if (toggleOpenStateOK)
    is("toggleOpenState ok", "toggleOpenState ok", testid + "toggleOpenState");
}

function testtag_tree_TreeView_rows_sort(tree, testid, rowInfo)
{
  
  var columnIndex = 0;
  var view = tree.view;
  var column = tree.columns[columnIndex];
  var columnElement = column.element;
  var sortkey = columnElement.getAttribute("sort");
  if (sortkey) {
    view.cycleHeader(column);
    is(tree.getAttribute("sort"), sortkey, "cycleHeader sort");
    is(tree.getAttribute("sortDirection"), "ascending", "cycleHeader sortDirection ascending");
    is(columnElement.getAttribute("sortDirection"), "ascending", "cycleHeader column sortDirection");
    is(columnElement.getAttribute("sortActive"), "true", "cycleHeader column sortActive");
    view.cycleHeader(column);
    is(tree.getAttribute("sortDirection"), "descending", "cycleHeader sortDirection descending");
    is(columnElement.getAttribute("sortDirection"), "descending", "cycleHeader column sortDirection descending");
    view.cycleHeader(column);
    is(tree.getAttribute("sortDirection"), "", "cycleHeader sortDirection natural");
    is(columnElement.getAttribute("sortDirection"), "", "cycleHeader column sortDirection natural");
    
  }

  
  var columns = getSortedColumnArray(tree);
  is(columnElement.getAttribute("sortDirection"), "",
     "cycleHeader column sortDirection");

  
  mouseClickOnColumnHeader(columns, columnIndex, 0, 1);
  is(columnElement.getAttribute("sortDirection"), "ascending",
     "single click cycleHeader column sortDirection ascending");

  
  mouseClickOnColumnHeader(columns, columnIndex, 0, 2);
  if (navigator.platform.indexOf("Win") == 0) {
    
    is(columnElement.getAttribute("sortDirection"), "descending",
       "double click cycleHeader column sortDirection descending");
    
    mouseClickOnColumnHeader(columns, columnIndex, 0, 1);
  }

  
  is(columnElement.getAttribute("sortDirection"), "",
     "cycleHeader column sortDirection");

  columnElement.setAttribute("sorthints", "twostate");
  view.cycleHeader(column);
  is(tree.getAttribute("sortDirection"), "ascending", "cycleHeader sortDirection ascending twostate");
  view.cycleHeader(column);
  is(tree.getAttribute("sortDirection"), "descending", "cycleHeader sortDirection ascending twostate");
  view.cycleHeader(column);
  is(tree.getAttribute("sortDirection"), "ascending", "cycleHeader sortDirection ascending twostate again");
  columnElement.removeAttribute("sorthints");
  view.cycleHeader(column);
  view.cycleHeader(column);

  is(columnElement.getAttribute("sortDirection"), "",
     "cycleHeader column sortDirection reset");
}






function testtag_tree_TreeSelection_State(tree, testid, current, selected, viewidx, column)
{
  var selection = tree.view.selection;

  if (!column)
    column = (tree.selType == "cell") ? tree.columns[0] : null;

  is(selection.count, selected.length, testid + " count");
  is(tree.currentIndex, current, testid + " currentIndex");
  is(selection.currentIndex, current, testid + " TreeSelection currentIndex");
  is(selection.currentColumn, column, testid + " currentColumn");
  if (viewidx !== null && viewidx !== undefined)
    is(tree.treeBoxObject.getFirstVisibleRow(), viewidx, testid + " first visible row");

  var actualSelected = [];
  var count = tree.view.rowCount;
  for (var s = 0; s < count; s++) {
    if (selection.isSelected(s))
      actualSelected.push(s);
  }

  is(compareArrays(selected, actualSelected), true, testid + " selection [" + selected + "]");

  actualSelected = [];
  var rangecount = selection.getRangeCount();
  for (var r = 0; r < rangecount; r++) {
    var start = {}, end = {};
    selection.getRangeAt(r, start, end);
    for (var rs = start.value; rs <= end.value; rs++)
      actualSelected.push(rs);
  }

  is(compareArrays(selected, actualSelected), true, testid + " range selection [" + selected + "]");
}

function testtag_tree_column_reorder()
{
  
  
  var testframe = window.parent.document.getElementById("testframe");
  if (testframe) {
    testframe.scrollIntoView();
  }

  var tree = document.getElementById("tree-column-reorder");
  var numColumns = tree.columns.count;

  var reference = [];
  for (var i = 0; i < numColumns; i++) {
    reference.push("col_" + i);
  }

  
  for (var i = 0; i < numColumns - 1; i++) {
    synthesizeColumnDrag(tree, i, i + 1, true);
    arrayMove(reference, i, i + 1, true);
    checkColumns(tree, reference, "drag first column right");
  }

  
  for (var i = numColumns - 1; i >= 1; i--) {
    synthesizeColumnDrag(tree, i, i - 1, false);
    arrayMove(reference, i, i - 1, false);
    checkColumns(tree, reference, "drag last column left");
  }

  
  for (var i = 1; i < numColumns; i++) {
    synthesizeColumnDrag(tree, i, i - 1, false);
    arrayMove(reference, i, i - 1, false);
    checkColumns(tree, reference, "drag each column left");
  }

  
  for (var i = numColumns - 2; i >= 0; i--) {
    synthesizeColumnDrag(tree, i, i + 1, true);
    arrayMove(reference, i, i + 1, true);
    checkColumns(tree, reference, "drag each column right");
  }

  
  for (var i = 0; i < numColumns - 5; i++) {
    synthesizeColumnDrag(tree, i, i + 5, true);
    arrayMove(reference, i, i + 5, true);
    checkColumns(tree, reference, "drag each column 5 to the right");
  }

  
  for (var i = numColumns - 6; i >= 5; i--) {
    synthesizeColumnDrag(tree, i, i - 5, false);
    arrayMove(reference, i, i - 5, false);
    checkColumns(tree, reference, "drag each column 5 to the left");
  }

  
  synthesizeColumnDrag(tree, 0, 0, true);
  checkColumns(tree, reference, "drag to itself");
  is(document.treecolDragging, null, "drag to itself completed");

  
  SimpleTest.finish();
}

function testtag_tree_wheel(aTree)
{
  const deltaModes = [
    WheelEvent.DOM_DELTA_PIXEL,  
    WheelEvent.DOM_DELTA_LINE,   
    WheelEvent.DOM_DELTA_PAGE    
  ];
  function helper(aStart, aDelta, aIntDelta, aDeltaMode)
  {
    aTree.treeBoxObject.scrollToRow(aStart);
    var expected = !aIntDelta ? aStart :
          aDeltaMode != WheelEvent.DOM_DELTA_PAGE ? aStart + aIntDelta :
          aIntDelta > 0 ? aStart + aTree.treeBoxObject.getPageLength() :
                          aStart - aTree.treeBoxObject.getPageLength();
    if (expected < 0) {
      expected = 0;
    }
    if (expected > aTree.view.rowCount - aTree.treeBoxObject.getPageLength()) {
      expected = aTree.view.rowCount - aTree.treeBoxObject.getPageLength();
    }
    synthesizeWheel(aTree.body, 1, 1,
                    { deltaMode: aDeltaMode, deltaY: aDelta,
                      lineOrPageDeltaY: aIntDelta });
    is(aTree.treeBoxObject.getFirstVisibleRow(), expected,
         "testtag_tree_wheel: vertical, starting " + aStart +
           " delta " + aDelta + " lineOrPageDelta " + aIntDelta +
           " aDeltaMode " + aDeltaMode);

    aTree.treeBoxObject.scrollToRow(aStart);
    
    synthesizeWheel(aTree.body, 1, 1,
                    { deltaMode: aDeltaMode, deltaX: aDelta,
                      lineOrPageDeltaX: aIntDelta });
    is(aTree.treeBoxObject.getFirstVisibleRow(), aStart,
         "testtag_tree_wheel: horizontal, starting " + aStart +
           " delta " + aDelta + " lineOrPageDelta " + aIntDelta +
           " aDeltaMode " + aDeltaMode);
  }

  var defaultPrevented = 0;

  function wheelListener(event) {
    defaultPrevented++;
  }
  window.addEventListener("wheel", wheelListener, false);

  deltaModes.forEach(function(aDeltaMode) {
    var delta = (aDeltaMode == WheelEvent.DOM_DELTA_PIXEL) ? 5.0 : 0.3;
    helper(2, -delta,  0, aDeltaMode);
    helper(2, -delta, -1, aDeltaMode);
    helper(2,  delta,  0, aDeltaMode);
    helper(2,  delta,  1, aDeltaMode);
    helper(2, -2 * delta,  0, aDeltaMode);
    helper(2, -2 * delta, -1, aDeltaMode);
    helper(2,  2 * delta,  0, aDeltaMode);
    helper(2,  2 * delta,  1, aDeltaMode);
  });

  window.removeEventListener("wheel", wheelListener, false);
  is(defaultPrevented, 48, "wheel event default prevented");
}

function synthesizeColumnDrag(aTree, aMouseDownColumnNumber, aMouseUpColumnNumber, aAfter)
{
  var columns = getSortedColumnArray(aTree);

  var down = columns[aMouseDownColumnNumber].element;
  var up   = columns[aMouseUpColumnNumber].element;

  
  
  var columnWidth = down.boxObject.width;
  var splitterHitWidth = columnWidth / 2;
  synthesizeMouse(down, splitterHitWidth, 3, { type: "mousedown"});

  var offsetX = 0;
  if (aAfter) {
    offsetX = columnWidth;
  }

  if (aMouseUpColumnNumber > aMouseDownColumnNumber) {
    for (var i = aMouseDownColumnNumber; i <= aMouseUpColumnNumber; i++) {
      var move = columns[i].element;
      synthesizeMouse(move, offsetX, 3, { type: "mousemove"});
    }
  }
  else {
    for (var i = aMouseDownColumnNumber; i >= aMouseUpColumnNumber; i--) {
      var move = columns[i].element;
      synthesizeMouse(move, offsetX, 3, { type: "mousemove"});
    }
  }

  synthesizeMouse(up, offsetX, 3, { type: "mouseup"});
}

function arrayMove(aArray, aFrom, aTo, aAfter)
{
  var o = aArray.splice(aFrom, 1)[0];
  if (aTo > aFrom) {
    aTo--;
  }

  if (aAfter) {
    aTo++;
  }

  aArray.splice(aTo, 0, o);
}

function getSortedColumnArray(aTree)
{
  var columns = aTree.columns;
  var a = [];
  for (var i = 0; i < columns.length; i++) {
    a.push(columns.getColumnAt(i));
  }

  a.sort(function(a, b) {
    var o1 = parseInt(a.element.getAttribute("ordinal"));
    var o2 = parseInt(b.element.getAttribute("ordinal"));
    return o1 - o2;
  });
  return a;
}

function checkColumns(aTree, aReference, aMessage)
{
  var columns = getSortedColumnArray(aTree);
  var ids = [];
  columns.forEach(function(e) {
    ids.push(e.element.id);
  });
  is(compareArrays(ids, aReference), true, aMessage);
}

function mouseOnCell(tree, row, column, testname)
{
  var rect = tree.boxObject.getCoordsForCellItem(row, column, "text");

  synthesizeMouseExpectEvent(tree.body, rect.x, rect.y, {}, tree, "select", testname);
}

function mouseClickOnColumnHeader(aColumns, aColumnIndex, aButton, aClickCount)
{
  var columnHeader = aColumns[aColumnIndex].element;
  var columnHeaderRect = columnHeader.getBoundingClientRect();
  var columnWidth = columnHeaderRect.right - columnHeaderRect.left;
  
  
  for (var i = 1; i <= aClickCount; i++) {
    
    synthesizeMouse(columnHeader, columnWidth / 2, 3,
                    { button: aButton,
                      clickCount: i }, null);
  }
}

function mouseDblClickOnCell(tree, row, column, testname)
{
  
  var selection = tree.view.selection;
  selection.select(row);
  tree.treeBoxObject.ensureRowIsVisible(row);

  
  var rect = tree.treeBoxObject.getCoordsForCellItem(row, column, "text");

  synthesizeMouse(tree.body, rect.x, rect.y, { clickCount: 2 }, null);
}

function compareArrays(arr1, arr2)
{
  if (arr1.length != arr2.length)
    return false;

  for (var i = 0; i < arr1.length; i++) {
    if (arr1[i] != arr2[i])
      return false;
  }

  return true;
}

function convertProperties(arr)
{
  var results = [];
  var count = arr.Count();
  for (var i = 0; i < count; i++)
    results.push(arr.GetElementAt(i).QueryInterface(Components.interfaces.nsIAtom).toString());

  results.sort();
  return results.join(" ");
}

function convertDOMtoTreeRowInfo(treechildren, level, rowidx)
{
  var obj = { rows: [] };

  var parentidx = rowidx.value;

  treechildren = treechildren.childNodes;
  for (var r = 0; r < treechildren.length; r++) {
    rowidx.value++;

    var treeitem = treechildren[r];
    if (treeitem.hasChildNodes()) {
      var treerow = treeitem.firstChild;
      var cellInfo = [];
      for (var c = 0; c < treerow.childNodes.length; c++) {
        var cell = treerow.childNodes[c];
        cellInfo.push({ label: "" + cell.getAttribute("label"),
                        value: cell.getAttribute("value"),
                        properties: cell.getAttribute("properties"),
                        editable: cell.getAttribute("editable") != "false",
                        selectable: cell.getAttribute("selectable") != "false",
                        image: cell.getAttribute("src"),
                        mode: cell.hasAttribute("mode") ? parseInt(cell.getAttribute("mode")) : 3 });
      }

      var descendants = treeitem.lastChild;
      var children = (treerow == descendants) ? null :
                     convertDOMtoTreeRowInfo(descendants, level + 1, rowidx);
      obj.rows.push({ cells: cellInfo,
                      properties: treerow.getAttribute("properties"),
                      container: treeitem.getAttribute("container") == "true",
                      separator: treeitem.localName == "treeseparator",
                      children: children,
                      level: level,
                      parent: parentidx });
    }
  }

  return obj;
}
