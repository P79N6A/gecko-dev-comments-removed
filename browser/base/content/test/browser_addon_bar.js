



































function test() {
  waitForExplicitFinish();

  let addonbar = document.getElementById("addon-bar");
  ok(addonbar.collapsed, "addon bar is collapsed by default");

  let topMenu, toolbarMenu;

  function onTopMenuShown(event) {
    event.currentTarget.removeEventListener("popupshown", arguments.callee, false);
    
    let toolbarMenu = document.getElementById("appmenu_customizeMenu") ||
                      document.getElementById("viewToolbarsMenu").firstElementChild;
    toolbarMenu.addEventListener("popupshown", onToolbarMenuShown, false);
    toolbarMenu.openPopup();
  }

  function onToolbarMenuShown(event) {
    event.currentTarget.removeEventListener("popupshown", arguments.callee, false);

    
    let menuitem = document.getElementById("toggle_addon-bar");
    ok(menuitem, "found the menu item");
    is(menuitem.getAttribute("checked"), "false", "menuitem is not checked by default");

    
    
    menuitem.setAttribute("checked", "true");
    menuitem.click();

    
    is(addonbar.getAttribute("collapsed"), "false", "addon bar is visible after executing the command");
    is(menuitem.getAttribute("checked"), "true", "menuitem is checked after executing the command");
    finish();
  }

  
  topMenu = document.getElementById("appmenu-popup") ||
            document.getElementById("menu_viewPopup");
  topMenu.addEventListener("popupshown", onTopMenuShown, false);
  topMenu.openPopup();
}
