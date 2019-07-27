



"use strict";




let test = asyncTest(function*() {
  let projecteditor = yield addProjectEditorTabForTempDirectory();
  let TEMP_PATH = projecteditor.project.allPaths()[0];

  is (getTempFile("").path, TEMP_PATH, "Temp path is set correctly.");

  ok (projecteditor.currentEditor, "There is an editor for projecteditor");
  let resources = projecteditor.project.allResources();

  is (
    resources.map(r=>r.basename).join("|"),
    TEMP_FOLDER_NAME + "|css|styles.css|data|img|icons|128x128.png|16x16.png|32x32.png|vector.svg|fake.png|js|script.js|index.html|LICENSE|README.md",
    "Resources came through in proper order"
  );

  for (let i = 0; i < resources.length; i++){
    yield selectAndRefocusFile(projecteditor, resources[i]);
  }
});

function selectAndRefocusFile(projecteditor, resource) {
  ok (resource && resource.path, "A valid resource has been passed in for selection " + (resource && resource.path));
  projecteditor.projectTree.selectResource(resource);

  if (resource.isDir) {
    return;
  }

  let [editorCreated, editorLoaded, editorActivated] = yield promise.all([
    onceEditorCreated(projecteditor),
    onceEditorLoad(projecteditor),
    onceEditorActivated(projecteditor)
  ]);

  if (projecteditor.currentEditor.editor) {
    
    
    let treeContainer = projecteditor.projectTree.getViewContainer(getDirectoryInStore(resource));
    treeContainer.line.click();
    EventUtils.synthesizeMouseAtCenter(treeContainer.elt, {}, treeContainer.elt.ownerDocument.defaultView);
    let waitForTreeSelect = onTreeSelection(projecteditor);
    projecteditor.currentEditor.focus();
    yield waitForTreeSelect;

    is (projecteditor.projectTree.getSelectedResource(), resource, "The resource gets reselected in the tree");
  }
}


function getDirectoryInStore(resource) {
  return resource.store.root.childrenSorted.filter(r=>r.isDir)[0];
}

function onTreeSelection(projecteditor) {
  let def = promise.defer();
  projecteditor.projectTree.on("selection", function selection() {
    projecteditor.projectTree.off("focus", selection);
    def.resolve();
  });
  return def.promise;
}
