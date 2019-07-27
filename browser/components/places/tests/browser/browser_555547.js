



"use strict";

add_task(function* test() {
  let sidebarBox = document.getElementById("sidebar-box");
  is(sidebarBox.hidden, true, "The sidebar should be hidden");

  
  let toolbar = document.getElementById("PersonalToolbar");
  let wasCollapsed = toolbar.collapsed;
  if (wasCollapsed) {
    yield promiseSetToolbarVisibility(toolbar, true);
  }

  let sidebar = yield promiseLoadedSidebar("viewBookmarksSidebar");
  registerCleanupFunction(() => {
    SidebarUI.hide();
  });

  
  let tree = sidebar.contentDocument.getElementById("bookmarks-view");
  tree.focus();

  let controller = doGetPlacesControllerForCommand("placesCmd_copy");
  let treeController = tree.controllers
                           .getControllerForCommand("placesCmd_copy");
  ok(controller == treeController, "tree controller was returned");

  
  
  let toolbarItems = document.getElementById("PlacesToolbarItems");
  EventUtils.synthesizeMouse(toolbarItems.childNodes[0],
                             4, 4, { type: "contextmenu", button: 2 },
                             window);
  controller = doGetPlacesControllerForCommand("placesCmd_copy");
  let toolbarController = document.getElementById("PlacesToolbar")
                                  .controllers
                                  .getControllerForCommand("placesCmd_copy");
  ok(controller == toolbarController, "the toolbar controller was returned");

  document.getElementById("placesContext").hidePopup();

  
  tree.focus();
  controller = doGetPlacesControllerForCommand("placesCmd_copy");
  ok(controller == treeController, "tree controller was returned");

  if (wasCollapsed) {
    yield promiseSetToolbarVisibility(toolbar, false);
  }
});

function promiseLoadedSidebar(cmd) {
  return new Promise(resolve => {
    let sidebar = document.getElementById("sidebar");
    sidebar.addEventListener("load", function onLoad() {
      sidebar.removeEventListener("load", onLoad, true);
      resolve(sidebar);
    }, true);

    SidebarUI.show(cmd);
  });
}
