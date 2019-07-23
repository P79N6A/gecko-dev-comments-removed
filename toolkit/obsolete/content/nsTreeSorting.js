











































function RefreshSort()
{
  var current_column = find_sort_column();
  SortColumnElement(current_column);
}


function SortInNewDirection(direction)
{
  var current_column = find_sort_column();
  if (direction == "ascending")
    direction = "natural";
  else if (direction == "descending")
    direction = "ascending";
  else if (direction == "natural")
    direction = "descending";
  current_column.setAttribute("sortDirection", direction);
  SortColumnElement(current_column);
}

function SortColumn(columnId)
{
  var column = document.getElementById(columnId);
  SortColumnElement(column);
}

function SortColumnElement(column)
{
  var tree = column.parentNode.parentNode;
  var col = tree.columns.getColumnFor(column);
  tree.view.cycleHeader(col);
}


function find_sort_column()
{
  var columns = document.getElementsByTagName('treecol');
  var i = 0;
  var column;
  while ((column = columns.item(i++)) != null) {
      if (column.getAttribute('sortDirection'))
          return column;
  }
  return columns.item(0);
}


function find_sort_direction(column)
{
  var sortDirection = column.getAttribute('sortDirection');
  return (sortDirection ? sortDirection : "natural");
}






function update_sort_menuitems(column, direction)
{
  var unsorted_menuitem = document.getElementById("unsorted_menuitem");
  var sort_ascending = document.getElementById('sort_ascending');
  var sort_descending = document.getElementById('sort_descending');

  
  
  
  if ((!unsorted_menuitem) || (!sort_ascending) || (!sort_descending))
    return;

  if (direction == "natural") {
    unsorted_menuitem.setAttribute('checked','true');
    sort_ascending.setAttribute('disabled','true');
    sort_descending.setAttribute('disabled','true');
    sort_ascending.removeAttribute('checked');
    sort_descending.removeAttribute('checked');
  } else {
    sort_ascending.removeAttribute('disabled');
    sort_descending.removeAttribute('disabled');
    if (direction == "ascending") {
      sort_ascending.setAttribute('checked','true');
    } else {
      sort_descending.setAttribute('checked','true');
    }

    var columns = document.getElementsByTagName('treecol');
    var i = 0;
    var column_node = columns[i];
    var column_name = column.id;
    var menuitem = document.getElementById('fill_after_this_node');
    menuitem = menuitem.nextSibling
    while (1) {
      var name = menuitem.getAttribute('column_id');
      if (!name) break;
      if (column_name == name) {
        menuitem.setAttribute('checked', 'true');
        break;
      }
      menuitem = menuitem.nextSibling;
      column_node = columns[++i];
      if (column_node && column_node.tagName == "splitter") {
        column_node = columns[++i];
      }
    }
  }
  enable_sort_menuitems();
}

function enable_sort_menuitems()
{
  var columns = document.getElementsByTagName('treecol');
  var menuitem = document.getElementById('fill_after_this_node');
  menuitem = menuitem.nextSibling
  for (var i = 0; (i < columns.length) && menuitem; ++i) {
    var column_node = columns[i];
    if (column_node.getAttribute("hidden") == "true")
      menuitem.setAttribute("disabled", "true");
    else
      menuitem.removeAttribute("disabled");
    menuitem = menuitem.nextSibling;
  }
}

function fillViewMenu(popup)
{
  var fill_after = document.getElementById('fill_after_this_node');
  var fill_before = document.getElementById('fill_before_this_node');
  var strBundle = document.getElementById('sortBundle');
  var sortString;
  if (strBundle)
    sortString = strBundle.getString('SortMenuItems');
  if (!sortString)
    sortString = "Sorted by %COLNAME%";
    
  var firstTime = (fill_after.nextSibling == fill_before);
  if (firstTime) {
    var columns = document.getElementsByTagName('treecol');
    for (var i = 0; i < columns.length; ++i) {
      var column = columns[i];
      
      var column_name = column.getAttribute("label");
      var column_accesskey = column.getAttribute("accesskey");
      var item = document.createElement("menuitem");
      if (column_accesskey)
        item.setAttribute("accesskey", column_accesskey);
      item.setAttribute("type", "radio");
      item.setAttribute("name", "sort_column");
      if (column_name == "")
        column_name = column.getAttribute("display");
      var name = sortString.replace(/%COLNAME%/g, column_name);
      item.setAttribute("label", name);
      item.setAttribute("oncommand", "SortColumn('" + column.id + "');");
      item.setAttribute("column_id", column.id);
      popup.insertBefore(item, fill_before);
    }
  }
  var sort_column = find_sort_column();
  var sort_direction = find_sort_direction(sort_column);
  update_sort_menuitems(sort_column, sort_direction);
}
