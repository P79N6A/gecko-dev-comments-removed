






const PAGECONTENT =
  "<html><body onload='document.body.firstChild.focus()'><select>" +
  "  <optgroup label='First Group'>" +
  "    <option value=One>One" +
  "    <option value=Two>Two" +
  "  </optgroup>" +
  "  <option value=Three>Three" +
  "  <optgroup label='Second Group' disabled='true'>" +
  "    <option value=Four>Four" +
  "    <option value=Five>Five" +
  "  </optgroup>" +
  "  <option value=Six disabled='true'>Six" +
  "  <optgroup label='Third Group'>" +
  "    <option value=Seven>Seven" +
  "    <option value=Eight>Eight" +
  "  </optgroup>" +
  "</body></html>";

function openSelectPopup(selectPopup)
{
  return new Promise((resolve, reject) => {
    selectPopup.addEventListener("popupshown", function popupListener(event) {
      selectPopup.removeEventListener("popupshown", popupListener, false)
      resolve();
    }, false);
    setTimeout(() => EventUtils.synthesizeKey("KEY_ArrowDown", { altKey: true, code: "ArrowDown" }), 1500);
  });
}

function hideSelectPopup(selectPopup)
{
  return new Promise((resolve, reject) => {
    selectPopup.addEventListener("popuphidden", function popupListener(event) {
      selectPopup.removeEventListener("popuphidden", popupListener, false)
      resolve();
    }, false);
    EventUtils.synthesizeKey("KEY_Enter", { code: "Enter" });
  });
}

add_task(function*() {
  let tab = gBrowser.selectedTab = gBrowser.addTab();
  let browser = gBrowser.getBrowserForTab(tab);
  yield promiseTabLoadEvent(tab, "data:text/html," + escape(PAGECONTENT));

  yield SimpleTest.promiseFocus(browser.contentWindow);

  let menulist = document.getElementById("ContentSelectDropdown");
  let selectPopup = menulist.menupopup;

  yield openSelectPopup(selectPopup);

  let isWindows = navigator.platform.indexOf("Win") >= 0;

  is(menulist.selectedIndex, 1, "Initial selection");
  is(selectPopup.firstChild.localName, "menucaption", "optgroup is caption");
  is(selectPopup.firstChild.getAttribute("label"), "First Group", "optgroup label");
  is(selectPopup.childNodes[1].localName, "menuitem", "option is menuitem");
  is(selectPopup.childNodes[1].getAttribute("label"), "One", "option label");

  EventUtils.synthesizeKey("KEY_ArrowDown", { code: "ArrowDown" });
  is(menulist.menuBoxObject.activeChild, menulist.getItemAtIndex(2), "Select item 2");
  is(menulist.selectedIndex, isWindows ? 2 : 1, "Select item 2 selectedIndex");

  EventUtils.synthesizeKey("KEY_ArrowDown", { code: "ArrowDown" });
  is(menulist.menuBoxObject.activeChild, menulist.getItemAtIndex(3), "Select item 3");
  is(menulist.selectedIndex, isWindows ? 3 : 1, "Select item 3 selectedIndex");

  EventUtils.synthesizeKey("KEY_ArrowDown", { code: "ArrowDown" });

  
  let expectedIndex = isWindows ? 5 : 9;
     
  is(menulist.menuBoxObject.activeChild, menulist.getItemAtIndex(expectedIndex),
     "Skip optgroup header and disabled items select item 7");
  is(menulist.selectedIndex, isWindows ? 5 : 1, "Select or skip disabled item selectedIndex");

  for (let i = 0; i < 10; i++) {
    is(menulist.getItemAtIndex(i).disabled, i >= 4 && i <= 7, "item " + i + " disabled")
  }

  EventUtils.synthesizeKey("KEY_ArrowUp", { code: "ArrowUp" });
  is(menulist.menuBoxObject.activeChild, menulist.getItemAtIndex(3), "Select item 3 again");
  is(menulist.selectedIndex, isWindows ? 3 : 1, "Select item 3 selectedIndex");

  yield hideSelectPopup(selectPopup);

  is(menulist.selectedIndex, 3, "Item 3 still selected");

  gBrowser.removeCurrentTab();
});

