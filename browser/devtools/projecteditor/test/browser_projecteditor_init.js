



"use strict";



function test() {
  info ("Initializing projecteditor");
  addProjectEditorTab().then((projecteditor) => {
    ok (projecteditor, "Load callback has been called");
    ok (projecteditor.shells, "ProjectEditor has shells");
    ok (projecteditor.project, "ProjectEditor has a project");
    finish();
  });
}

