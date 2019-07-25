








































var EventUtils = {};
try {
  
  
  Components.utils.import('resource://mozmill/stdlib/EventUtils.js', EventUtils);
} catch (e) {
  Components.utils.import('resource://mozmill/modules/EventUtils.js', EventUtils);
}

var MODULE_NAME = 'WidgetsAPI';

const gTimeout = 5000;















function clickTreeCell(controller, tree, rowIndex, columnIndex, eventDetails)
{
  tree = tree.getNode();

  var selection = tree.view.selection;
  selection.select(rowIndex);
  tree.treeBoxObject.ensureRowIsVisible(rowIndex);

  
  var x = {}, y = {}, width = {}, height = {};
  var column = tree.columns[columnIndex];
  tree.treeBoxObject.getCoordsForCellItem(rowIndex, column, "text",
                                           x, y, width, height);

  controller.sleep(0);
  EventUtils.synthesizeMouse(tree.body, x.value + 4, y.value + 4,
                             eventDetails, tree.ownerDocument.defaultView);
  controller.sleep(0);
}
