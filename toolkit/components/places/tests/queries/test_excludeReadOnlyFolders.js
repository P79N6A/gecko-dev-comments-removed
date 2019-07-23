










































var testData = [
  
  { isInQuery: true, isFolder: true, title: "Folder 1",
    parentFolder: bmsvc.toolbarFolder },

  
  { isInQuery: false, isFolder: true, title: "Folder 2 RO",
    parentFolder: bmsvc.toolbarFolder, readOnly: true }
];

function run_test() {
  populateDB(testData);

  var query = histsvc.getNewQuery();
  query.setFolders([bmsvc.toolbarFolder], 1);

  
  var options = histsvc.getNewQueryOptions();
  options.excludeQueries = true;
  options.excludeReadOnlyFolders = true;

  
  var result = histsvc.executeQuery(query, options);
  var root = result.root;
  displayResultSet(root);
  
  do_check_eq(1, root.childCount);
  do_check_eq("Folder 1", root.getChild(0).title);

  root.containerOpen = false;
}
