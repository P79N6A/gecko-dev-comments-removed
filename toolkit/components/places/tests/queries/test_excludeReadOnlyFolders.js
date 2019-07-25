










































var testData = [
  
  { isInQuery: true, isFolder: true, title: "Folder 1",
    parentFolder: PlacesUtils.toolbarFolderId },

  
  { isInQuery: false, isFolder: true, title: "Folder 2 RO",
    parentFolder: PlacesUtils.toolbarFolderId, readOnly: true }
];

function run_test() {
  populateDB(testData);

  var query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.toolbarFolderId], 1);

  
  var options = PlacesUtils.history.getNewQueryOptions();
  options.excludeQueries = true;
  options.excludeReadOnlyFolders = true;

  
  var result = PlacesUtils.history.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  displayResultSet(root);
  
  do_check_eq(1, root.childCount);
  do_check_eq("Folder 1", root.getChild(0).title);

  root.containerOpen = false;
}
