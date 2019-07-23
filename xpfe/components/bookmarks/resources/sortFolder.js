




































function onAccept() {
  var sortOptions = window.arguments[0];
  sortOptions.accepted = true;
  sortOptions.sortBy = document.getElementById("sortBy").value;
  sortOptions.sortOrder = document.getElementById("sortOrder").value;
  sortOptions.sortFoldersFirst = document.getElementById("sortFoldersFirst").checked;
  sortOptions.sortRecursively = document.getElementById("sortRecursively").checked;
}

function onSortByChanged() {
  var sortBy = document.getElementById("sortBy");
  var sortFoldersFirst = document.getElementById("sortFoldersFirst");
  sortFoldersFirst.checked = sortBy.value == "Name";
}
