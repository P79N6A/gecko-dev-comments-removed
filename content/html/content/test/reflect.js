























function reflectString(aParameters)
{
  var element = aParameters.element;
  var contentAttr = typeof aParameters.attribute === "string"
                      ? aParameters.attribute : aParameters.attribute.content;
  var idlAttr = typeof aParameters.attribute === "string"
                  ? aParameters.attribute : aParameters.attribute.idl;
  var otherValues = aParameters.otherValues !== undefined
                      ? aParameters.otherValues : [];

  ok(idlAttr in element,
     idlAttr + " should be an IDL attribute of this element");
  is(typeof element[idlAttr], "string",
     idlAttr + " IDL attribute should be a string");

  
  is(element.getAttribute(contentAttr), null,
     "When not set, the content attribute should be null.");
  is(element[idlAttr], "",
     "When not set, the IDL attribute should return the empty string");

  



  element.setAttribute(contentAttr, null);
  todo_is(element.getAttribute(contentAttr), "null",
     "null should have been stringified to 'null'");
  todo_is(element[idlAttr], "null",
     "null should have been stringified to 'null'");
  element.removeAttribute(contentAttr);

  element[idlAttr] = null;
  
  if (element.localName == "textarea" && idlAttr == "wrap") {
    is(element.getAttribute(contentAttr), "null",
       "null should have been stringified to 'null'");
    is(element[idlAttr], "null", "null should have been stringified to 'null'");
    element.removeAttribute(contentAttr);
  } else {
    todo_is(element.getAttribute(contentAttr), "null",
       "null should have been stringified to 'null'");
    todo_is(element[idlAttr], "null",
       "null should have been stringified to 'null'");
    element.removeAttribute(contentAttr);
  }

  
  var stringsToTest = [
    
    [ "", "" ],
    [ "null", "null" ],
    [ "undefined", "undefined" ],
    [ "foo", "foo" ],
    [ contentAttr, contentAttr ],
    [ idlAttr, idlAttr ],
    
    
    [ undefined, "undefined" ],
    [ true, "true" ],
    [ false, "false" ],
    [ 42, "42" ],
    
    [ { toString: function() { return "foo" } },
      "foo" ],
    [ { valueOf: function() { return "foo" } },
      "[object Object]" ],
    [ { valueOf: function() { return "quux" },
       toString: undefined },
      "quux" ],
    [ { valueOf: function() { return "foo" },
        toString: function() { return "bar" } },
      "bar" ]
  ];

  otherValues.forEach(function(v) { stringsToTest.push([v, v]) });

  stringsToTest.forEach(function([v, r]) {
    element.setAttribute(contentAttr, v);
    is(element[idlAttr], r,
       "IDL attribute should return the value it has been set to.");
    is(element.getAttribute(contentAttr), r,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v;
    is(element[idlAttr], r,
       "IDL attribute should return the value it has been set to.");
    is(element.getAttribute(contentAttr), r,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(contentAttr);
  });

  
  is(element.getAttribute(contentAttr), null,
     "When not set, the content attribute should be null.");
  is(element[idlAttr], "",
     "When not set, the IDL attribute should return the empty string");
}











function reflectUnsignedInt(aParameters)
{
  var element = aParameters.element;
  var attr = aParameters.attribute;
  var nonZero = aParameters.nonZero;
  var defaultValue = aParameters.defaultValue;

  if (defaultValue === undefined) {
    if (nonZero) {
      defaultValue = 1;
    } else {
      defaultValue = 0;
    }
  }

  ok(attr in element, attr + " should be an IDL attribute of this element");
  is(typeof element[attr], "number", attr + " IDL attribute should be a number");

  
  is(element[attr], defaultValue, "default value should be " + defaultValue);
  ok(!element.hasAttribute(attr), attr + " shouldn't be present");

  var values = [ 1, 3, 42, 2147483647 ];

  for each (var value in values) {
    element[attr] = value;
    is(element[attr], value, "." + attr + " should be equals " + value);
    is(element.getAttribute(attr), value,
       "@" + attr + " should be equals " + value);

    element.setAttribute(attr, value);
    is(element[attr], value, "." + attr + " should be equals " + value);
    is(element.getAttribute(attr), value,
       "@" + attr + " should be equals " + value);
  }

  
  element[attr] = -3000000000;
  is(element[attr], 1294967296, "." + attr + " should be equals to 1294967296");
  is(element.getAttribute(attr), 1294967296,
     "@" + attr + " should be equals to 1294967296");

  
  element.setAttribute(attr, -3000000000);
  is(element.getAttribute(attr), -3000000000,
     "@" + attr + " should be equals to " + -3000000000);
  is(element[attr], defaultValue,
     "." + attr + " should be equals to " + defaultValue);

  var nonValidValues = [
    
    [ -2147483648, 2147483648 ],
    [ -1,          4294967295 ],
    [ 3147483647,  3147483647 ],
  ];

  for each (var values in nonValidValues) {
    element[attr] = values[0];
    is(element.getAttribute(attr), values[1],
       "@" + attr + " should be equals to " + values[1]);
    is(element[attr], defaultValue,
       "." + attr + " should be equals to " + defaultValue);
  }

  for each (var values in nonValidValues) {
    element.setAttribute(attr, values[0]);
    is(element.getAttribute(attr), values[0],
       "@" + attr + " should be equals to " + values[0]);
    is(element[attr], defaultValue,
       "." + attr + " should be equals to " + defaultValue);
  }

  
  var caught = false;
  try {
    element[attr] = 0;
  } catch(e) {
    caught = true;
    is(e.code, DOMException.INDEX_SIZE_ERR, "exception should be INDEX_SIZE_ERR");
  }

  if (nonZero) {
    ok(caught, "an exception should have been caught");
  } else {
    ok(!caught, "no exception should have been caught");
  }

  
  element.setAttribute(attr, 0);
  is(element.getAttribute(attr), 0, "@" + attr + " should be equals to 0");
  if (nonZero) {
    is(element[attr], defaultValue,
       "." + attr + " should be equals to " + defaultValue);
  } else {
    is(element[attr], 0, "." + attr + " should be equals to 0");
  }
}















function reflectLimitedEnumerated(aParameters)
{
  var element = aParameters.element;
  var contentAttr = typeof aParameters.attribute === "string"
                      ? aParameters.attribute : aParameters.attribute.content;
  var idlAttr = typeof aParameters.attribute === "string"
                  ? aParameters.attribute : aParameters.attribute.idl;
  var validValues = aParameters.validValues;
  var invalidValues = aParameters.invalidValues;
  var defaultValue = aParameters.defaultValue !== undefined
                       ? aParameters.defaultValue : "";
  var unsupportedValues = aParameters.unsupportedValues !== undefined
                            ? aParameters.unsupportedValues : [];

  ok(idlAttr in element, idlAttr + " should be an IDL attribute of this element");
  is(typeof element[idlAttr], "string", idlAttr + " IDL attribute should be a string");

  
  element.removeAttribute(contentAttr);
  is(element[idlAttr], defaultValue,
     "When no attribute is set, the value should be the default value.");

  
  validValues.forEach(function (v) {
    element.setAttribute(contentAttr, v);
    is(element[idlAttr], v,
       v + " should be accepted as a valid value for " + idlAttr);
    is(element.getAttribute(contentAttr), v,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(contentAttr);

    element.setAttribute(contentAttr, v.toUpperCase());
    is(element[idlAttr], v,
       "Enumerated attributes should be case-insensitive.");
    is(element.getAttribute(contentAttr), v.toUpperCase(),
       "Content attribute should not be lower-cased.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v;
    is(element[idlAttr], v,
       v + " should be accepted as a valid value for " + idlAttr);
    is(element.getAttribute(contentAttr), v,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v.toUpperCase();
    is(element[idlAttr], v,
       "Enumerated attributes should be case-insensitive.");
    is(element.getAttribute(contentAttr), v.toUpperCase(),
       "Content attribute should not be lower-cased.");
    element.removeAttribute(contentAttr);
  });

  
  invalidValues.forEach(function (v) {
    element.setAttribute(contentAttr, v);
    is(element[idlAttr], defaultValue,
       "When the content attribute is set to an invalid value, the default value should be returned.");
    is(element.getAttribute(contentAttr), v,
       "Content attribute should not have been changed.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v;
    is(element[idlAttr], defaultValue,
       "When the value is set to an invalid value, the default value should be returned.");
    is(element.getAttribute(contentAttr), v,
       "Content attribute should not have been changed.");
    element.removeAttribute(contentAttr);
  });

  
  
  unsupportedValues.forEach(function (v) {
    element.setAttribute(contentAttr, v);
    todo_is(element[idlAttr], v,
            v + " should be accepted as a valid value for " + idlAttr);
    is(element.getAttribute(contentAttr), v,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(contentAttr);

    element.setAttribute(contentAttr, v.toUpperCase());
    todo_is(element[idlAttr], v,
            "Enumerated attributes should be case-insensitive.");
    is(element.getAttribute(contentAttr), v.toUpperCase(),
       "Content attribute should not be lower-cased.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v;
    todo_is(element[idlAttr], v,
            v + " should be accepted as a valid value for " + idlAttr);
    is(element.getAttribute(contentAttr), v,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v.toUpperCase();
    todo_is(element[idlAttr], v,
            "Enumerated attributes should be case-insensitive.");
    is(element.getAttribute(contentAttr), v.toUpperCase(),
       "Content attribute should not be lower-cased.");
    element.removeAttribute(contentAttr);
  });
}










function reflectBoolean(aParameters)
{
  var element = aParameters.element;
  var contentAttr = typeof aParameters.attribute === "string"
                      ? aParameters.attribute : aParameters.attribute.content;
  var idlAttr = typeof aParameters.attribute === "string"
                  ? aParameters.attribute : aParameters.attribute.idl;

  ok(idlAttr in element,
     idlAttr + " should be an IDL attribute of this element");
  is(typeof element[idlAttr], "boolean",
     idlAttr + " IDL attribute should be a boolean");

  
  is(element.getAttribute(contentAttr), null,
     "When not set, the content attribute should be null.");
  is(element[idlAttr], false,
     "When not set, the IDL attribute should return false");

  






  var valuesToTest = [
    { value: true, stringified: "true", result: true },
    { value: false, stringified: "false", result: false },
    { value: "true", stringified: "true", result: true },
    { value: "false", stringified: "false", result: true },
    { value: "foo", stringified: "foo", result: true },
    { value: idlAttr, stringified: idlAttr, result: true },
    { value: contentAttr, stringified: contentAttr, result: true },
    { value: "null", stringified: "null", result: true },
    { value: "undefined", stringified: "undefined", result: true },
    { value: "", stringified: "", result: false },
    { value: undefined, stringified: "undefined", result: false },
    { value: null, stringified: "null", result: false },
    { value: +0, stringified: "0", result: false },
    { value: -0, stringified: "0", result: false },
    { value: NaN, stringified: "NaN", result: false },
    { value: 42, stringified: "42", result: true },
    { value: Infinity, stringified: "Infinity", result: true },
    { value: -Infinity, stringified: "-Infinity", result: true },
    
    { value: { toString: function() { return "foo" } }, stringified: "foo",
      result: true },
    { value: { valueOf: function() { return "foo" } },
      stringified: "[object Object]", result: true },
    { value: { valueOf: function() { return "quux" }, toString: undefined },
      stringified: "quux", result: true },
    { value: { valueOf: function() { return "foo" },
               toString: function() { return "bar" } }, stringified: "bar",
      result: true },
    { value: { valueOf: function() { return false } },
      stringified: "[object Object]", result: true },
    { value: { foo: false, bar: false }, stringified: "[object Object]",
      result: true },
    { value: { }, stringified: "[object Object]", result: true },
  ];

  valuesToTest.forEach(function(v) {
    element.setAttribute(contentAttr, v.value);
    is(element[idlAttr], true,
       "IDL attribute should return always return 'true' if the content attribute has been set");
    if (v.value === null) {
      
      todo(element.getAttribute(contentAttr), v.stringified,
           "Content attribute should return the stringified value it has been set to.");
    } else {
      is(element.getAttribute(contentAttr), v.stringified,
         "Content attribute should return the stringified value it has been set to.");
    }
    element.removeAttribute(contentAttr);

    element[idlAttr] = v.value;
    is(element[idlAttr], v.result, "IDL attribute should return " + v.result);
    is(element.getAttribute(contentAttr), v.result ? "" : null,
       v.result ? "Content attribute should return the empty string."
                : "Content attribute should return null.");
    is(element.hasAttribute(contentAttr), v.result,
       v.result ? contentAttr + " should not be present"
                : contentAttr + " should be present");
    element.removeAttribute(contentAttr);
  });

  
  is(element.getAttribute(contentAttr), null,
     "When not set, the content attribute should be null.");
  is(element[contentAttr], false,
     "When not set, the IDL attribute should return false");
}











function reflectInt(aParameters)
{
  
  function expectedGetAttributeResult(value) {
    return (value !== null) ? String(value) : "";
  }

  function stringToInteger(value, nonNegative, defaultValue) {
    if (nonNegative === false) {
      
      var result = /^[ \t\n\f\r]*([\+\-]?[0-9]+)/.exec(value);
      if (result) {
        if (-0x80000000 <= result[1] && result[1] <= 0x7FFFFFFF) {
          
          return result[1];
        }
      }
    } else {
      var result = /^[ \t\n\f\r]*(\+?[0-9]+)/.exec(value);
      if (result) {
        if (0 <= result[1] && result[1] <= 0x7FFFFFFF) {
          
          return result[1];
        }
      }
    }
    return defaultValue;
  }

  
  function expectedIdlAttributeResult(value) {
    
    return value << 0;
  }

  var element = aParameters.element;
  var attr = aParameters.attribute;
  var nonNegative = aParameters.nonNegative;

  var defaultValue = aParameters.defaultValue !== undefined
                      ? aParameters.defaultValue
                      : nonNegative ? -1 : 0;

  ok(attr in element, attr + " should be an IDL attribute of this element");
  is(typeof element[attr], "number", attr + " IDL attribute should be a number");

  
  is(element[attr], defaultValue, "default value should be " + defaultValue);
  ok(!element.hasAttribute(attr), attr + " shouldn't be present");

  




  var valuesToTest = [
    
    0, 1, 55555, 2147483647, +42,
    
    "0", "1", "777777", "2147483647", "+42",
    
    -0, -1, -3333, -2147483648,
    
    "-0", "-1", "-222", "-2147483647", "-2147483648",
    
    -2147483649, -3000000000, -4294967296, 2147483649, 4000000000, -4294967297,
    
    "     1111111", "  23456   ",
    
    "", " ", "+", "-", "foo", "+foo", "-foo", "+     foo", "-     foo", "+-2", "-+2", "++2", "--2", "hello1234", "1234hello",
    "444 world 555", "why 567 what", "-3 nots", "2e5", "300e2", "42+-$", "+42foo", "-514not", "\vblah", "0x10FFFF", "-0xABCDEF",
    
    1.2345, 42.0, 3456789.1, -2.3456, -6789.12345, -2147483649.1234,
    
    "1.2345", "42.0", "3456789.1", "-2.3456", "-6789.12345", "-2147483649.1234",
    
    undefined, null, NaN, Infinity, -Infinity,
  ];

  valuesToTest.forEach(function(v) {
    var intValue = stringToInteger(v, nonNegative, defaultValue);

    element.setAttribute(attr, v);

    is(element.getAttribute(attr), expectedGetAttributeResult(v), element.localName + ".setAttribute(" +
      attr + ", " + v + "), " + element.localName + ".getAttribute(" + attr + ") ");

    if (intValue == -2147483648 && element[attr] == defaultValue) {
      
      todo_is(element[attr], intValue, "Bug 586761: " + element.localName +
        ".setAttribute(value, " + v + "), " + element.localName + "[" + attr + "] ");
    } else if ((v === "-0" || v == "-0xABCDEF") && nonNegative) {
      
      todo_is(element[attr], intValue, "Bug 688093: " + element.localName +
        ".setAttribute(" + attr + ", " + v + "), " + element.localName + "[" + attr + "] ");
    } else {
      is(element[attr], intValue, element.localName +
        ".setAttribute(" + attr + ", " + v + "), " + element.localName + "[" + attr + "] ");
    }
    element.removeAttribute(attr);

    if (nonNegative && expectedIdlAttributeResult(v) < 0) {
      try {
        element[attr] = v;
        ok(false, element.localName + "[" + attr + "] = " + v + " should throw NS_ERROR_DOM_INDEX_SIZE_ERR");
      } catch(e) {
        is(e.code, DOMException.INDEX_SIZE_ERR, element.localName + "[" + attr + "] = " + v +
          " should throw NS_ERROR_DOM_INDEX_SIZE_ERR");
      }
    } else {
      element[attr] = v;
      if (expectedIdlAttributeResult(v) == -2147483648 && element[attr] == defaultValue) {
        
        todo_is(element[attr], expectedIdlAttributeResult(v), "Bug 586761: " + element.localName + "[" +
          attr + "] = " + v + ", " + element.localName + "[" + attr + "] ");
      } else {
        is(element[attr], expectedIdlAttributeResult(v), element.localName + "[" + attr + "] = " + v +
          ", " + element.localName + "[" + attr + "] ");
        is(element.getAttribute(attr), expectedIdlAttributeResult(v), element.localName + "[" + attr +
          "] = " + v + ", " + element.localName + ".getAttribute(" + attr + ") ");
      }
    }
    element.removeAttribute(attr);
  });

  
  is(element.getAttribute(attr), null,
     "When not set, the content attribute should be null.");
  is(element[attr], defaultValue,
     "When not set, the IDL attribute should return default value.");
}
