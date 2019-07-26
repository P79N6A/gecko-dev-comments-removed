



"use strict";

const kToolbarID = "test-toolbar";





add_task(function* testAddingToolbar() {
  let toolbar = document.createElement("toolbar");
  toolbar.setAttribute("mode", "full");
  toolbar.setAttribute("customizable", "true");
  toolbar.setAttribute("id", kToolbarID);

  CustomizableUI.registerArea(kToolbarID, {
     type: CustomizableUI.TYPE_TOOLBAR,
     legacy: false,
  })

  gNavToolbox.appendChild(toolbar);

  is(toolbar.getAttribute("mode"), "icons",
     "Toolbar should have its mode attribute set to icons.")

  toolbar.remove();
  CustomizableUI.unregisterArea(kToolbarID);
});