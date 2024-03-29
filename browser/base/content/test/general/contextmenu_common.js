var lastElement;

function openContextMenuFor(element, shiftkey, waitForSpellCheck) {
    
    is(SpecialPowers.wrap(contextMenu).state, "closed", "checking if popup is closed");

    if (lastElement)
      lastElement.blur();
    element.focus();

    
    
    function actuallyOpenContextMenuFor() {
      lastElement = element;
      var eventDetails = { type : "contextmenu", button : 2, shiftKey : shiftkey };
      synthesizeMouse(element, 2, 2, eventDetails, element.ownerDocument.defaultView);
    }

    if (waitForSpellCheck) {
      var { onSpellCheck } = SpecialPowers.Cu.import("resource://gre/modules/AsyncSpellCheckTestHelper.jsm", {});
      onSpellCheck(element, actuallyOpenContextMenuFor);
    }
    else {
      actuallyOpenContextMenuFor();
    }
}

function closeContextMenu() {
    contextMenu.hidePopup();
}

function getVisibleMenuItems(aMenu, aData) {
    var items = [];
    var accessKeys = {};
    for (var i = 0; i < aMenu.childNodes.length; i++) {
        var item = aMenu.childNodes[i];
        if (item.hidden)
            continue;

        var key = item.accessKey;
        if (key)
            key = key.toLowerCase();

        var isGenerated = item.hasAttribute("generateditemid");

        if (item.nodeName == "menuitem") {
            var isSpellSuggestion = item.className == "spell-suggestion";
            if (isSpellSuggestion) {
              is(item.id, "", "child menuitem #" + i + " is a spelling suggestion");
            } else if (isGenerated) {
              is(item.id, "", "child menuitem #" + i + " is a generated item");
            } else {
              ok(item.id, "child menuitem #" + i + " has an ID");
            }
            var label = item.getAttribute("label");
            ok(label.length, "menuitem " + item.id + " has a label");
            if (isSpellSuggestion) {
              is(key, "", "Spell suggestions shouldn't have an access key");
              items.push("*" + label);
            } else if (isGenerated) {
              items.push("+" + label);
            } else if (item.id.indexOf("spell-check-dictionary-") != 0 &&
                       item.id != "spell-no-suggestions" &&
                       item.id != "spell-add-dictionaries-main") {
              ok(key, "menuitem " + item.id + " has an access key");
              if (accessKeys[key])
                  ok(false, "menuitem " + item.id + " has same accesskey as " + accessKeys[key]);
              else
                  accessKeys[key] = item.id;
            }
            if (!isSpellSuggestion && !isGenerated) {
              items.push(item.id);
            }
            if (isGenerated) {
              var p = {};
              p.type = item.getAttribute("type");
              p.icon = item.getAttribute("image");
              p.checked = item.hasAttribute("checked");
              p.disabled = item.hasAttribute("disabled");
              items.push(p);
            } else {
              items.push(!item.disabled);
            }
        } else if (item.nodeName == "menuseparator") {
            ok(true, "--- seperator id is " + item.id);
            items.push("---");
            items.push(null);
        } else if (item.nodeName == "menu") {
            if (isGenerated) {
                item.id = "generated-submenu-" + aData.generatedSubmenuId++;
            }
            ok(item.id, "child menu #" + i + " has an ID");
            if (!isGenerated) {
                ok(key, "menu has an access key");
                if (accessKeys[key])
                    ok(false, "menu " + item.id + " has same accesskey as " + accessKeys[key]);
                else
                    accessKeys[key] = item.id;
            }
            items.push(item.id);
            items.push(!item.disabled);
            
            
            items.push([]);
            items.push(null);
        } else if (item.nodeName == "menugroup") {
            ok(item.id, "child menugroup #" + i + " has an ID");
            items.push(item.id);
            items.push(!item.disabled);
            var menugroupChildren = [];
            for (var child of item.children) {
                if (child.hidden)
                    continue;

                menugroupChildren.push([child.id, !child.disabled]);
            }
            items.push(menugroupChildren);
            items.push(null);
        } else {
            ok(false, "child #" + i + " of menu ID " + aMenu.id +
                      " has an unknown type (" + item.nodeName + ")");
        }
    }
    return items;
}

function checkContextMenu(expectedItems) {
    is(contextMenu.state, "open", "checking if popup is open");
    var data = { generatedSubmenuId: 1 };
    checkMenu(contextMenu, expectedItems, data);
}

function checkMenuItem(actualItem, actualEnabled, expectedItem, expectedEnabled, index) {
    is(actualItem, expectedItem,
       "checking item #" + index/2 + " (" + expectedItem + ") name");

    if (typeof expectedEnabled == "object" && expectedEnabled != null ||
        typeof actualEnabled == "object" && actualEnabled != null) {

        ok(!(actualEnabled == null), "actualEnabled is not null");
        ok(!(expectedEnabled == null), "expectedEnabled is not null");
        is(typeof actualEnabled, typeof expectedEnabled, "checking types");

        if (typeof actualEnabled != typeof expectedEnabled ||
            actualEnabled == null || expectedEnabled == null)
          return;

        is(actualEnabled.type, expectedEnabled.type,
           "checking item #" + index/2 + " (" + expectedItem + ") type attr value");
        var icon = actualEnabled.icon;
        if (icon) {
          var tmp = "";
          var j = icon.length - 1;
          while (j && icon[j] != "/") {
            tmp = icon[j--] + tmp;
          }
          icon = tmp;
        }
        is(icon, expectedEnabled.icon,
           "checking item #" + index/2 + " (" + expectedItem + ") icon attr value");
        is(actualEnabled.checked, expectedEnabled.checked,
           "checking item #" + index/2 + " (" + expectedItem + ") has checked attr");
        is(actualEnabled.disabled, expectedEnabled.disabled,
           "checking item #" + index/2 + " (" + expectedItem + ") has disabled attr");
    } else if (expectedEnabled != null)
        is(actualEnabled, expectedEnabled,
           "checking item #" + index/2 + " (" + expectedItem + ") enabled state");
}














function checkMenu(menu, expectedItems, data) {
    var actualItems = getVisibleMenuItems(menu, data);
    
    for (var i = 0; i < expectedItems.length; i+=2) {
        var actualItem   = actualItems[i];
        var actualEnabled = actualItems[i + 1];
        var expectedItem = expectedItems[i];
        var expectedEnabled = expectedItems[i + 1];
        if (expectedItem instanceof Array) {
            ok(true, "Checking submenu/menugroup...");
            var previousId = expectedItems[i - 2]; 
            var previousItem = menu.getElementsByAttribute("id", previousId)[0];
            ok(previousItem, (previousItem ? previousItem.nodeName : "item") + " with previous id (" + previousId + ") found");
            if (previousItem && previousItem.nodeName == "menu") {
              ok(previousItem, "got a submenu element of id='" + previousId + "'");
              is(previousItem.nodeName, "menu", "submenu element of id='" + previousId +
                                           "' has expected nodeName");
              checkMenu(previousItem.menupopup, expectedItem, data, i);
            } else if (previousItem && previousItem.nodeName == "menugroup") {
              ok(expectedItem.length, "menugroup must not be empty");
              for (var j = 0; j < expectedItem.length / 2; j++) {
                checkMenuItem(actualItems[i][j][0], actualItems[i][j][1], expectedItem[j*2], expectedItem[j*2+1], i+j*2);
              }
              i += j;
            } else {
              ok(false, "previous item is not a menu or menugroup");
            }
        } else {
            checkMenuItem(actualItem, actualEnabled, expectedItem, expectedEnabled, i);
        }
    }
    
    is(actualItems.length, expectedItems.length, "checking expected number of menu entries");
}
