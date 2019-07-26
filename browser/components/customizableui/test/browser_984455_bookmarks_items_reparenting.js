



"use strict";

let gNavBar = document.getElementById(CustomizableUI.AREA_NAVBAR);
let gOverflowList = document.getElementById(gNavBar.getAttribute("overflowtarget"));

const kBookmarksButton = "bookmarks-menu-button";
const kBookmarksItems = "personal-bookmarks";
const kOriginalWindowWidth = window.outerWidth;
const kSmallWidth = 400;





function bookmarksMenuPanelShown() {
  let deferred = Promise.defer();
  let bookmarksMenuPopup = document.getElementById("BMB_bookmarksPopup");
  let onTransitionEnd = (e) => {
    if (e.target == bookmarksMenuPopup) {
      bookmarksMenuPopup.removeEventListener("transitionend", onTransitionEnd);
      deferred.resolve();
    }
  }
  bookmarksMenuPopup.addEventListener("transitionend", onTransitionEnd);
  return deferred.promise;
}









function checkPlacesContextMenu(aItemWithContextMenu) {
  return Task.spawn(function* () {
    let contextMenu = document.getElementById("placesContext");
    let newBookmarkItem = document.getElementById("placesContext_new:bookmark");
    info("Waiting for context menu on " + aItemWithContextMenu.id);
    let shownPromise = popupShown(contextMenu);
    EventUtils.synthesizeMouseAtCenter(aItemWithContextMenu,
                                       {type: "contextmenu", button: 2});
    yield shownPromise;

    ok(!newBookmarkItem.hasAttribute("disabled"),
       "New bookmark item shouldn't be disabled");

    info("Closing context menu");
    yield closePopup(contextMenu);
  });
}






function checkSpecialContextMenus() {
  return Task.spawn(function* () {
    let contextMenu = document.getElementById("placesContext");
    let bookmarksMenuButton = document.getElementById(kBookmarksButton);
    let bookmarksMenuPopup = document.getElementById("BMB_bookmarksPopup");

    const kSpecialItemIDs = {
      "BMB_bookmarksToolbar": "BMB_bookmarksToolbarPopup",
      "BMB_unsortedBookmarks": "BMB_unsortedBookmarksPopup",
    };

    
    
    let shownPromise = bookmarksMenuPanelShown();
    let dropmarker = document.getAnonymousElementByAttribute(bookmarksMenuButton,
                                                             "anonid", "dropmarker");
    EventUtils.synthesizeMouseAtCenter(dropmarker, {});
    info("Waiting for bookmarks menu popup to show after clicking dropmarker.")
    yield shownPromise;

    for (let menuID in kSpecialItemIDs) {
      let menuItem = document.getElementById(menuID);
      let menuPopup = document.getElementById(kSpecialItemIDs[menuID]);
      info("Waiting to open menu for " + menuID);
      let shownPromise = popupShown(menuPopup);
      EventUtils.synthesizeMouseAtCenter(menuItem, {});
      yield shownPromise;

      yield checkPlacesContextMenu(menuPopup);
      info("Closing menu for " + menuID);
      yield closePopup(menuPopup);
    }

    info("Closing bookmarks menu");
    yield closePopup(bookmarksMenuPopup);
  });
}







function closePopup(aPopup) {
  let hiddenPromise = popupHidden(aPopup);
  EventUtils.synthesizeKey("VK_ESCAPE", {});
  return hiddenPromise;
}






function checkBookmarksItemsChevronContextMenu() {
  return Task.spawn(function*() {
    let chevronPopup = document.getElementById("PlacesChevronPopup");
    let shownPromise = popupShown(chevronPopup);
    let chevron = document.getElementById("PlacesChevron");
    EventUtils.synthesizeMouseAtCenter(chevron, {});
    info("Waiting for bookmark toolbar item chevron popup to show");
    yield shownPromise;
    yield waitForCondition(() => {
      for (let child of chevronPopup.children) {
        if (child.style.visibility != "hidden")
          return true;
      }
    });
    yield checkPlacesContextMenu(chevronPopup);
    info("Waiting for bookmark toolbar item chevron popup to close");
    yield closePopup(chevronPopup);
  });
}






function overflowEverything() {
  info("Waiting for overflow");
  window.resizeTo(kSmallWidth, window.outerHeight);
  return waitForCondition(() => gNavBar.hasAttribute("overflowing"));
}






function stopOverflowing() {
  info("Waiting until we stop overflowing");
  window.resizeTo(kOriginalWindowWidth, window.outerHeight);
  return waitForCondition(() => !gNavBar.hasAttribute("overflowing"));
}






function checkOverflowing(aID) {
  ok(!gNavBar.querySelector("#" + aID),
     "Item with ID " + aID + " should no longer be in the gNavBar");
  let item = gOverflowList.querySelector("#" + aID);
  ok(item, "Item with ID " + aID + " should be overflowing");
  is(item.getAttribute("overflowedItem"), "true",
     "Item with ID " + aID + " should have overflowedItem attribute");
}






function checkNotOverflowing(aID) {
  ok(!gOverflowList.querySelector("#" + aID),
     "Item with ID " + aID + " should no longer be overflowing");
  let item = gNavBar.querySelector("#" + aID);
  ok(item, "Item with ID " + aID + " should be in the nav bar");
  ok(!item.hasAttribute("overflowedItem"),
     "Item with ID " + aID + " should not have overflowedItem attribute");
}





add_task(function* testOverflowingBookmarksButtonContextMenu() {
  ok(!gNavBar.hasAttribute("overflowing"), "Should start with a non-overflowing toolbar.");
  ok(CustomizableUI.inDefaultState, "Should start in default state.");

  
  
  yield checkSpecialContextMenus();

  yield overflowEverything();
  checkOverflowing(kBookmarksButton);

  yield stopOverflowing();
  checkNotOverflowing(kBookmarksButton);

  yield checkSpecialContextMenus();
});





add_task(function* testOverflowingBookmarksItemsContextMenu() {
  info("Ensuring panel is ready.");
  yield PanelUI.ensureReady();

  let bookmarksToolbarItems = document.getElementById(kBookmarksItems);
  gCustomizeMode.addToToolbar(bookmarksToolbarItems);
  yield checkPlacesContextMenu(bookmarksToolbarItems);

  yield overflowEverything();
  checkOverflowing(kBookmarksItems)

  gCustomizeMode.addToPanel(bookmarksToolbarItems);

  yield stopOverflowing();

  gCustomizeMode.addToToolbar(bookmarksToolbarItems);
  yield checkPlacesContextMenu(bookmarksToolbarItems);
});





add_task(function* testOverflowingBookmarksItemsChevronContextMenu() {
  
  
  let bookmarksToolbarItems = document.getElementById(kBookmarksItems);
  gCustomizeMode.addToToolbar(bookmarksToolbarItems);

  
  
  
  let placesToolbarItems = document.getElementById("PlacesToolbarItems");
  let placesChevron = document.getElementById("PlacesChevron");
  placesToolbarItems.style.maxWidth = "10px";
  info("Waiting for chevron to no longer be collapsed");
  yield waitForCondition(() => !placesChevron.collapsed);

  yield checkBookmarksItemsChevronContextMenu();

  yield overflowEverything();
  checkOverflowing(kBookmarksItems);

  yield stopOverflowing();
  checkNotOverflowing(kBookmarksItems);

  yield checkBookmarksItemsChevronContextMenu();

  placesToolbarItems.style.removeProperty("max-width");
});

add_task(function* asyncCleanup() {
  window.resizeTo(kOriginalWindowWidth, window.outerHeight);
  yield resetCustomization();
});
