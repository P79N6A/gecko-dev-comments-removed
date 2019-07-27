
















var behaviours = {
  menu: "win:activate-disabled-menuitem activate-disabled-menuitem-mousemove select-keynav-wraps select-extended-keynav",
  menulist: "allow-other-value other-value-clears-selection",
  listbox: "select-extended-keynav",
  richlistbox: "select-extended-keynav",
  radiogroup: "select-keynav-wraps dont-select-disabled allow-other-value",
  tabs: "select-extended-keynav mac:select-keynav-wraps allow-other-value selection-required keynav-leftright"
};

function behaviourContains(tag, behaviour)
{
  var platform = "none:";
  if (navigator.platform.indexOf("Mac") >= 0)
    platform = "mac:";
  else if (navigator.platform.indexOf("Win") >= 0)
    platform = "win:";
  else if (navigator.platform.indexOf("X") >= 0)
    platform = "gtk:";

  var re = new RegExp("\\s" + platform + behaviour + "\\s|\\s" + behaviour + "\\s");
  return re.test(" " + behaviours[tag] + " ");
}

function test_nsIDOMXULSelectControlElement(element, childtag, testprefix)
{
  var testid = (testprefix) ? testprefix + " " : "";
  testid += element.localName + " nsIDOMXULSelectControlElement ";

  
  var firstvalue = "first", secondvalue = "second", fourthvalue = "fourth";
  if (element.localName == "menulist" && element.editable) {
    firstvalue = "First Item";
    secondvalue = "Second Item"
    fourthvalue = "Fourth Item";
  }

  
  test_nsIDOMXULSelectControlElement_States(element, testid + "initial", 0, null, -1, "");

  test_nsIDOMXULSelectControlElement_init(element, testid);

  
  var firstitem = element.appendItem("First Item", "first");
  is(firstitem.localName, childtag,
                testid + "appendItem - first item is " + childtag);
  test_nsIDOMXULSelectControlElement_States(element, testid + "appendItem", 1, null, -1, "");

  is(firstitem.control, element, testid + "control");

  
  element.selectedIndex = 0;
  test_nsIDOMXULSelectControlElement_States(element, testid + "selectedIndex", 1, firstitem, 0, firstvalue);

  
  var seconditem = element.appendItem("Second Item", "second");
  test_nsIDOMXULSelectControlElement_States(element, testid + "appendItem 2", 2, firstitem, 0, firstvalue);

  
  element.selectedItem = seconditem;
  test_nsIDOMXULSelectControlElement_States(element, testid + "selectedItem", 2, seconditem, 1, secondvalue);

  
  var selectionRequired = behaviourContains(element.localName, "selection-required");
  element.selectedIndex = -1;
  test_nsIDOMXULSelectControlElement_States(element, testid + "selectedIndex 2", 2,
        selectionRequired ? seconditem : null, selectionRequired ? 1 : -1,
        selectionRequired ? secondvalue : "");

  
  element.selectedIndex = 1;
  element.selectedItem = null;
  test_nsIDOMXULSelectControlElement_States(element, testid + "selectedItem 2", 2,
        selectionRequired ? seconditem : null, selectionRequired ? 1 : -1,
        selectionRequired ? secondvalue : "");

  
  is(element.getIndexOfItem(firstitem), 0, testid + "getIndexOfItem - first item at index 0");
  is(element.getIndexOfItem(seconditem), 1, testid + "getIndexOfItem - second item at index 1");

  var otheritem = element.ownerDocument.createElement(childtag);
  is(element.getIndexOfItem(otheritem), -1, testid + "getIndexOfItem - other item not found");

  
  is(element.getItemAtIndex(0), firstitem, testid + "getItemAtIndex - index 0 is first item");
  is(element.getItemAtIndex(1), seconditem, testid + "getItemAtIndex - index 0 is second item");
  is(element.getItemAtIndex(-1), null, testid + "getItemAtIndex - index -1 is null");
  is(element.getItemAtIndex(2), null, testid + "getItemAtIndex - index 2 is null");

  
  element.value = firstvalue;
  test_nsIDOMXULSelectControlElement_States(element, testid + "set value 1", 2, firstitem, 0, firstvalue);
  element.value = secondvalue;
  test_nsIDOMXULSelectControlElement_States(element, testid + "set value 2", 2, seconditem, 1, secondvalue);
  
  
  
  element.value = "other";
  var allowOtherValue = behaviourContains(element.localName, "allow-other-value");
  var otherValueClearsSelection = behaviourContains(element.localName, "other-value-clears-selection");
  test_nsIDOMXULSelectControlElement_States(element, testid + "set value other", 2,
                                            otherValueClearsSelection ? null : seconditem,
                                            otherValueClearsSelection ? -1 : 1,
                                            allowOtherValue ? "other" : secondvalue);
  if (allowOtherValue)
    element.value = "";

  
  if (selectionRequired)
    element.value = secondvalue;
  else
    element.selectedIndex = -1;

  var removeditem = element.removeItemAt(0);
  is(removeditem, firstitem, testid + "removeItemAt return value");
  test_nsIDOMXULSelectControlElement_States(element, testid + "removeItemAt", 1,
        selectionRequired ? seconditem : null, selectionRequired ? 0 : -1,
        selectionRequired ? secondvalue : "");

  is(removeditem.control, undefined, testid + "control not set");

  var thirditem = element.appendItem("Third Item", "third");
  var fourthitem = element.appendItem("Fourth Item", fourthvalue);
  var fifthitem = element.appendItem("Fifth Item", "fifth");

  
  
  element.selectedItem = thirditem;
  is(element.removeItemAt(1), thirditem, testid + "removeItemAt 2 return value");

  
  
  var isnotradio = (element.localName != "radiogroup");
  
  isnotradio = false;
  if (isnotradio)
    test_nsIDOMXULSelectControlElement_States(element, testid + "removeItemAt 2", 3, fourthitem, 1, fourthvalue);

  
  
  element.selectedItem = fourthitem;
  element.removeItemAt(0);
  test_nsIDOMXULSelectControlElement_States(element, testid + "removeItemAt 3", 2, fourthitem, 0, fourthvalue);

  
  
  element.selectedItem = fifthitem;
  element.removeItemAt(1);
  if (isnotradio)
    test_nsIDOMXULSelectControlElement_States(element, testid + "removeItemAt 4", 1, fourthitem, 0, fourthvalue);

  
  is(element.removeItemAt(-1), null, testid + "removeItemAt 5 return value");
  if (isnotradio)
    test_nsIDOMXULSelectControlElement_States(element, testid + "removeItemAt 5", 1, fourthitem, 0, fourthvalue);

  
  is(element.removeItemAt(1), null, testid + "removeItemAt 6 return value");
  is("item removed", "item removed", testid + "removeItemAt 6");
  if (isnotradio)
    test_nsIDOMXULSelectControlElement_States(element, testid + "removeItemAt 6", 1, fourthitem, 0, fourthvalue);

  
  element.selectedIndex = 0;
  test_nsIDOMXULSelectControlElement_insertItemAt(element, 0, 0, testid, 5);
  test_nsIDOMXULSelectControlElement_insertItemAt(element, 2, 2, testid, 6);
  test_nsIDOMXULSelectControlElement_insertItemAt(element, -1, 3, testid, 7);
  test_nsIDOMXULSelectControlElement_insertItemAt(element, 6, 4, testid, 8);

  element.selectedIndex = 0;
  fourthitem.disabled = true;
  element.selectedIndex = 1;
  test_nsIDOMXULSelectControlElement_States(element, testid + "selectedIndex disabled", 5, fourthitem, 1, fourthvalue);

  element.selectedIndex = 0;
  element.selectedItem = fourthitem;
  test_nsIDOMXULSelectControlElement_States(element, testid + "selectedIndex disabled", 5, fourthitem, 1, fourthvalue);

  
  while (element.itemCount)
    element.removeItemAt(0);
  if (isnotradio)
    test_nsIDOMXULSelectControlElement_States(element, testid + "remove all", 0, null, -1,
                                              allowOtherValue ? "number8" : "");
}

function test_nsIDOMXULSelectControlElement_init(element, testprefix)
{
  
  var isEditable = (element.localName == "menulist" && element.editable);

  var id = element.id;
  element = document.getElementById(id + "-initwithvalue");
  if (element) {
    var seconditem = element.getItemAtIndex(1);
    test_nsIDOMXULSelectControlElement_States(element, testprefix + " value initialization",
                                              3, seconditem, 1,
                                              isEditable ? seconditem.label : seconditem.value);
  }

  element = document.getElementById(id + "-initwithselected");
  if (element) {
    var thirditem = element.getItemAtIndex(2);
    test_nsIDOMXULSelectControlElement_States(element, testprefix + " selected initialization",
                                              3, thirditem, 2,
                                              isEditable ? thirditem.label : thirditem.value);
  }
}

function test_nsIDOMXULSelectControlElement_States(element, testid,
                                                   expectedcount, expecteditem,
                                                   expectedindex, expectedvalue)
{
  
  var count = element.itemCount;
  is(count, expectedcount, testid + " item count");
  is(element.selectedItem, expecteditem, testid + " selectedItem");
  is(element.selectedIndex, expectedindex, testid + " selectedIndex");
  is(element.value, expectedvalue, testid + " value");
  if (element.selectedItem) {
    is(element.selectedItem.selected, true,
                  testid + " selectedItem marked as selected");
  }
}

function test_nsIDOMXULSelectControlElement_insertItemAt(element, index, expectedindex, testid, number)
{
  var expectedCount = element.itemCount;
  var expectedSelItem = element.selectedItem;
  var expectedSelIndex = element.selectedIndex;
  var expectedSelValue = element.value;

  var newitem = element.insertItemAt(index, "Item " + number, "number" + number);
  is(element.getIndexOfItem(newitem), expectedindex,
                testid + "insertItemAt " + expectedindex + " - get inserted item");
  expectedCount++;
  if (expectedSelIndex >= expectedindex)
    expectedSelIndex++;

  test_nsIDOMXULSelectControlElement_States(element, testid + "insertItemAt " + index,
                                           expectedCount, expectedSelItem,
                                           expectedSelIndex, expectedSelValue);
  return newitem;
}








function test_nsIDOMXULSelectControlElement_UI(element, testprefix)
{
  var testid = (testprefix) ? testprefix + " " : "";
  testid += element.localName + " nsIDOMXULSelectControlElement UI ";

  while (element.itemCount)
    element.removeItemAt(0);

  var firstitem = element.appendItem("First Item", "first");
  var seconditem = element.appendItem("Second Item", "second");

  
  synthesizeMouseExpectEvent(firstitem, 2, 2, {}, element, "select", testid + "mouse select");
  test_nsIDOMXULSelectControlElement_States(element, testid + "mouse select", 2, firstitem, 0, "first");

  synthesizeMouseExpectEvent(seconditem, 2, 2, {}, element, "select", testid + "mouse select 2");
  test_nsIDOMXULSelectControlElement_States(element, testid + "mouse select 2", 2, seconditem, 1, "second");
 
  
  element.selectedIndex = 1;
  element.focus();

  var navLeftRight = behaviourContains(element.localName, "keynav-leftright");
  var backKey = navLeftRight ? "VK_LEFT" : "VK_UP";
  var forwardKey = navLeftRight ? "VK_RIGHT" : "VK_DOWN";

  
  synthesizeKeyExpectEvent(backKey, {}, element, "select", testid + "key up");
  test_nsIDOMXULSelectControlElement_States(element, testid + "key up", 2, firstitem, 0, "first");

  var keyWrap = behaviourContains(element.localName, "select-keynav-wraps");

  var expectedItem = keyWrap ? seconditem : firstitem;
  var expectedIndex = keyWrap ? 1 : 0;
  var expectedValue = keyWrap ? "second" : "first";
  synthesizeKeyExpectEvent(backKey, {}, keyWrap ? element : null, "select", testid + "key up 2");
  test_nsIDOMXULSelectControlElement_States(element, testid + "key up 2", 2,
    expectedItem, expectedIndex, expectedValue);

  element.selectedIndex = 0;
  synthesizeKeyExpectEvent(forwardKey, {}, element, "select", testid + "key down");
  test_nsIDOMXULSelectControlElement_States(element, testid + "key down", 2, seconditem, 1, "second");

  expectedItem = keyWrap ? firstitem : seconditem;
  expectedIndex = keyWrap ? 0 : 1;
  expectedValue = keyWrap ? "first" : "second";
  synthesizeKeyExpectEvent(forwardKey, {}, keyWrap ? element : null, "select", testid + "key down 2");
  test_nsIDOMXULSelectControlElement_States(element, testid + "key down 2", 2,
    expectedItem, expectedIndex, expectedValue);

  var thirditem = element.appendItem("Third Item", "third");
  var fourthitem = element.appendItem("Fourth Item", "fourth");
  if (behaviourContains(element.localName, "select-extended-keynav")) {
    var fifthitem = element.appendItem("Fifth Item", "fifth");
    var sixthitem = element.appendItem("Sixth Item", "sixth");

    synthesizeKeyExpectEvent("VK_END", {}, element, "select", testid + "key end");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key end", 6, sixthitem, 5, "sixth");

    synthesizeKeyExpectEvent("VK_HOME", {}, element, "select", testid + "key home");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key home", 6, firstitem, 0, "first");

    synthesizeKeyExpectEvent("VK_PAGE_DOWN", {}, element, "select", testid + "key page down");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key page down", 6, fourthitem, 3, "fourth");
    synthesizeKeyExpectEvent("VK_PAGE_DOWN", {}, element, "select", testid + "key page down to end");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key page down to end", 6, sixthitem, 5, "sixth");

    synthesizeKeyExpectEvent("VK_PAGE_UP", {}, element, "select", testid + "key page up");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key page up", 6, thirditem, 2, "third");
    synthesizeKeyExpectEvent("VK_PAGE_UP", {}, element, "select", testid + "key page up to start");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key page up to start", 6, firstitem, 0, "first");

    element.removeItemAt(5);
    element.removeItemAt(4);
  }

  
  element.selectedIndex = 0;
  seconditem.disabled = true;
  
  var dontSelectDisabled = (behaviourContains(element.localName, "dont-select-disabled"));

  
  synthesizeMouseExpectEvent(seconditem, 2, 2, {}, element,
                             dontSelectDisabled ? "!select" : "select",
                             testid + "mouse select disabled");
  test_nsIDOMXULSelectControlElement_States(element, testid + "mouse select disabled", 4,
    dontSelectDisabled ? firstitem: seconditem, dontSelectDisabled ? 0 : 1,
    dontSelectDisabled ? "first" : "second");

  if (dontSelectDisabled) {
    
    synthesizeKeyExpectEvent(forwardKey, {}, element, "select", testid + "key down disabled");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key down disabled", 4, thirditem, 2, "third");

    synthesizeKeyExpectEvent(backKey, {}, element, "select", testid + "key up disabled");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key up disabled", 4, firstitem, 0, "first");

    element.selectedIndex = 2;
    firstitem.disabled = true;

    synthesizeKeyExpectEvent(backKey, {}, keyWrap ? element : null, "select", testid + "key up disabled 2");
    expectedItem = keyWrap ? fourthitem : thirditem;
    expectedIndex = keyWrap ? 3 : 2;
    expectedValue = keyWrap ? "fourth" : "third";
    test_nsIDOMXULSelectControlElement_States(element, testid + "key up disabled 2", 4,
      expectedItem, expectedIndex, expectedValue);
  }
  else {
    
    element.selectedIndex = 0;
    synthesizeKeyExpectEvent(forwardKey, {}, element, "select", testid + "key down disabled");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key down disabled", 4, seconditem, 1, "second");
    synthesizeKeyExpectEvent(forwardKey, {}, element, "select", testid + "key down disabled again");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key down disabled again", 4, thirditem, 2, "third");

    synthesizeKeyExpectEvent(backKey, {}, element, "select", testid + "key up disabled");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key up disabled", 4, seconditem, 1, "second");
    synthesizeKeyExpectEvent(backKey, {}, element, "select", testid + "key up disabled again");
    test_nsIDOMXULSelectControlElement_States(element, testid + "key up disabled again", 4, firstitem, 0, "first");
  }
}
