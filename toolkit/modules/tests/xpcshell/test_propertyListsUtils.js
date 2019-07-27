


"use strict";

Components.utils.import("resource://gre/modules/PropertyListUtils.jsm");

function checkValue(aPropertyListObject, aType, aValue) {
  do_check_eq(PropertyListUtils.getObjectType(aPropertyListObject), aType);
  if (aValue !== undefined) {
    
    let strictEqualityCheck = function(a, b) {
      do_check_eq(typeof(a), typeof(b));
      do_check_eq(a, b);
    };

    if (typeof(aPropertyListObject) == "object")
      strictEqualityCheck(aPropertyListObject.valueOf(), aValue.valueOf());
    else
      strictEqualityCheck(aPropertyListObject, aValue);
  }
}

function checkLazyGetterValue(aObject, aPropertyName, aType, aValue) {
  let descriptor = Object.getOwnPropertyDescriptor(aObject, aPropertyName);
  do_check_eq(typeof(descriptor.get), "function");
  do_check_eq(typeof(descriptor.value), "undefined");
  checkValue(aObject[aPropertyName], aType, aValue);
  descriptor = Object.getOwnPropertyDescriptor(aObject, aPropertyName);
  do_check_eq(typeof(descriptor.get), "undefined");
  do_check_neq(typeof(descriptor.value), "undefined");
}

function checkMainPropertyList(aPropertyListRoot) {
  const PRIMITIVE = PropertyListUtils.TYPE_PRIMITIVE;

  checkValue(aPropertyListRoot, PropertyListUtils.TYPE_DICTIONARY);

  
  Assert.ok(aPropertyListRoot.has("Boolean"));
  Assert.ok(!aPropertyListRoot.has("Nonexistent"));

  checkValue(aPropertyListRoot.get("Boolean"), PRIMITIVE, false);

  let array = aPropertyListRoot.get("Array");
  checkValue(array, PropertyListUtils.TYPE_ARRAY);
  do_check_eq(array.length, 8);

  
  

  
  checkLazyGetterValue(array, 0, PRIMITIVE, "abc");
  
  checkLazyGetterValue(array, 1, PRIMITIVE, new Array(1001).join("a"));
  
  checkLazyGetterValue(array, 2, PRIMITIVE, "\u05D0\u05D0\u05D0");
  
  checkLazyGetterValue(array, 3, PRIMITIVE, new Array(1001).join("\u05D0"));
  
  checkLazyGetterValue(array, 4, PRIMITIVE,
                       "\uD800\uDC00\uD800\uDC00\uD800\uDC00");

  
  checkLazyGetterValue(array, 5, PropertyListUtils.TYPE_DATE,
                       new Date("2011-12-31T11:15:23Z"));

  
  checkLazyGetterValue(array, 6, PropertyListUtils.TYPE_UINT8_ARRAY);
  let dataAsString = [String.fromCharCode(b) for each (b in array[6])].join("");
  do_check_eq(dataAsString, "2011-12-31T11:15:33Z");

  
  let dict = array[7];
  checkValue(dict, PropertyListUtils.TYPE_DICTIONARY);
  checkValue(dict.get("Negative Number"), PRIMITIVE, -400);
  checkValue(dict.get("Real Number"), PRIMITIVE, 2.71828183);
  checkValue(dict.get("Big Int"),
             PropertyListUtils.TYPE_INT64,
             "9007199254740993");
  checkValue(dict.get("Negative Big Int"),
             PropertyListUtils.TYPE_INT64,
             "-9007199254740993");
}

function readPropertyList(aFile, aCallback) {
  PropertyListUtils.read(aFile, function(aPropertyListRoot) {
    
    
    
    
    do_check_true(aPropertyListRoot !== null);
    aCallback(aPropertyListRoot);
    run_next_test();
  });
}

function run_test() {
  add_test(readPropertyList.bind(this,
    do_get_file("propertyLists/bug710259_propertyListBinary.plist", false),
    checkMainPropertyList));
  add_test(readPropertyList.bind(this,
    do_get_file("propertyLists/bug710259_propertyListXML.plist", false),
    checkMainPropertyList));
  run_next_test();
}
