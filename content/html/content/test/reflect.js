
























function reflectString(aParameters)
{
  var element = aParameters.element;
  var contentAttr = typeof aParameters.attribute === "string"
                      ? aParameters.attribute : aParameters.attribute.content;
  var idlAttr = typeof aParameters.attribute === "string"
                  ? aParameters.attribute : aParameters.attribute.idl;
  var otherValues = aParameters.otherValues !== undefined
                      ? aParameters.otherValues : [];
  var treatNullAs = aParameters.extendedAttributes ?
        aParameters.extendedAttributes.TreatNullAs : null;

  ok(idlAttr in element,
     idlAttr + " should be an IDL attribute of this element");
  is(typeof element[idlAttr], "string",
     "'" + idlAttr + "' IDL attribute should be a string");

  
  is(element.getAttribute(contentAttr), null,
     "When not set, the content attribute should be null.");
  is(element[idlAttr], "",
     "When not set, the IDL attribute should return the empty string");

  



  element.setAttribute(contentAttr, null);
  is(element.getAttribute(contentAttr), "null",
     "null should have been stringified to 'null' for '" + contentAttr + "'");
  is(element[idlAttr], "null",
      "null should have been stringified to 'null' for '" + idlAttr + "'");
  element.removeAttribute(contentAttr);

  element[idlAttr] = null;
  if (treatNullAs == "EmptyString") {
    is(element.getAttribute(contentAttr), "",
       "null should have been stringified to '' for '" + contentAttr + "'");
    is(element[idlAttr], "",
       "null should have been stringified to '' for '" + idlAttr + "'");
  } else {
    is(element.getAttribute(contentAttr), "null",
       "null should have been stringified to 'null' for '" + contentAttr + "'");
    is(element[idlAttr], "null",
       "null should have been stringified to 'null' for '" + contentAttr + "'");
  }
  element.removeAttribute(contentAttr);

  
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
       "IDL attribute '" + idlAttr + "' should return the value it has been set to.");
    is(element.getAttribute(contentAttr), r,
       "Content attribute '" + contentAttr + "'should return the value it has been set to.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v;
    is(element[idlAttr], r,
       "IDL attribute '" + idlAttr + "' should return the value it has been set to.");
    is(element.getAttribute(contentAttr), r,
       "Content attribute '" + contentAttr + "' should return the value it has been set to.");
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

  for (var value of values) {
    element[attr] = value;
    is(element[attr], value, "." + attr + " should be equals " + value);
    is(element.getAttribute(attr), String(value),
       "@" + attr + " should be equals " + value);

    element.setAttribute(attr, value);
    is(element[attr], value, "." + attr + " should be equals " + value);
    is(element.getAttribute(attr), String(value),
       "@" + attr + " should be equals " + value);
  }

  
  element[attr] = -3000000000;
  is(element[attr], 1294967296, "." + attr + " should be equals to 1294967296");
  is(element.getAttribute(attr), "1294967296",
     "@" + attr + " should be equals to 1294967296");

  
  element.setAttribute(attr, -3000000000);
  is(element.getAttribute(attr), "-3000000000",
     "@" + attr + " should be equals to " + -3000000000);
  is(element[attr], defaultValue,
     "." + attr + " should be equals to " + defaultValue);

  var nonValidValues = [
    
    [ -2147483648, 2147483648 ],
    [ -1,          4294967295 ],
    [ 3147483647,  3147483647 ],
  ];

  for (var values of nonValidValues) {
    element[attr] = values[0];
    is(element.getAttribute(attr), String(values[1]),
       "@" + attr + " should be equals to " + values[1]);
    is(element[attr], defaultValue,
       "." + attr + " should be equals to " + defaultValue);
  }

  for (var values of nonValidValues) {
    element.setAttribute(attr, values[0]);
    is(element.getAttribute(attr), String(values[0]),
       "@" + attr + " should be equals to " + values[0]);
    is(element[attr], defaultValue,
       "." + attr + " should be equals to " + defaultValue);
  }

  
  var caught = false;
  try {
    element[attr] = 0;
  } catch(e) {
    caught = true;
    is(e.name, "IndexSizeError", "exception should be IndexSizeError");
    is(e.code, DOMException.INDEX_SIZE_ERR, "exception code should be INDEX_SIZE_ERR");
  }

  if (nonZero) {
    ok(caught, "an exception should have been caught");
  } else {
    ok(!caught, "no exception should have been caught");
  }

  
  element.setAttribute(attr, "0");
  is(element.getAttribute(attr), "0", "@" + attr + " should be equals to 0");
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
  var defaultValueInvalid = aParameters.defaultValue === undefined
                               ? "" : typeof aParameters.defaultValue === "string"
                                   ? aParameters.defaultValue : aParameters.defaultValue.invalid
  var defaultValueMissing = aParameters.defaultValue === undefined
                                ? "" : typeof aParameters.defaultValue === "string"
                                    ? aParameters.defaultValue : aParameters.defaultValue.missing
  var unsupportedValues = aParameters.unsupportedValues !== undefined
                            ? aParameters.unsupportedValues : [];
  var nullable = aParameters.nullable;

  ok(idlAttr in element, idlAttr + " should be an IDL attribute of this element");
  if (nullable) {
    
    is(typeof element[idlAttr], "object", "'" + idlAttr + "' IDL attribute should be null, which has typeof == object");
    ise(element[idlAttr], null, "'" + idlAttr + "' IDL attribute should be null");
  } else {
    is(typeof element[idlAttr], "string", "'" + idlAttr + "' IDL attribute should be a string");
  }

  if (nullable) {
    element.setAttribute(contentAttr, "something");
    
    is(typeof element[idlAttr], "string", "'" + idlAttr + "' IDL attribute should be a string");
  }

  
  element.removeAttribute(contentAttr);
  ise(element[idlAttr], defaultValueMissing,
      "When no attribute is set, the value should be the default value.");

  
  validValues.forEach(function (v) {
    element.setAttribute(contentAttr, v);
    ise(element[idlAttr], v,
        "'" + v + "' should be accepted as a valid value for " + idlAttr);
    ise(element.getAttribute(contentAttr), v,
        "Content attribute should return the value it has been set to.");
    element.removeAttribute(contentAttr);

    element.setAttribute(contentAttr, v.toUpperCase());
    ise(element[idlAttr], v,
        "Enumerated attributes should be case-insensitive.");
    ise(element.getAttribute(contentAttr), v.toUpperCase(),
        "Content attribute should not be lower-cased.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v;
    ise(element[idlAttr], v,
        "'" + v + "' should be accepted as a valid value for " + idlAttr);
    ise(element.getAttribute(contentAttr), v,
        "Content attribute should return the value it has been set to.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v.toUpperCase();
    ise(element[idlAttr], v,
        "Enumerated attributes should be case-insensitive.");
    ise(element.getAttribute(contentAttr), v.toUpperCase(),
        "Content attribute should not be lower-cased.");
    element.removeAttribute(contentAttr);
  });

  
  invalidValues.forEach(function (v) {
    element.setAttribute(contentAttr, v);
    ise(element[idlAttr], defaultValueInvalid,
        "When the content attribute is set to an invalid value, the default value should be returned.");
    ise(element.getAttribute(contentAttr), v,
        "Content attribute should not have been changed.");
    element.removeAttribute(contentAttr);

    element[idlAttr] = v;
    ise(element[idlAttr], defaultValueInvalid,
        "When the value is set to an invalid value, the default value should be returned.");
    ise(element.getAttribute(contentAttr), v,
        "Content attribute should not have been changed.");
    element.removeAttribute(contentAttr);
  });

  
  
  unsupportedValues.forEach(function (v) {
    element.setAttribute(contentAttr, v);
    todo_is(element[idlAttr], v,
            "'" + v + "' should be accepted as a valid value for " + idlAttr);
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
            "'" + v + "' should be accepted as a valid value for " + idlAttr);
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

  if (nullable) {
    ise(defaultValueMissing, null,
	"Missing default value should be null for nullable attributes");
    ok(validValues.length > 0, "We better have at least one valid value");
    element.setAttribute(contentAttr, validValues[0]);
    ok(element.hasAttribute(contentAttr),
       "Should have content attribute: we just set it");
    element[idlAttr] = null;
    ok(!element.hasAttribute(contentAttr),
       "Should have removed content attribute");
  }
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
    is(element.getAttribute(contentAttr), v.stringified,
       "Content attribute should return the stringified value it has been set to.");
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
    return String(value);
  }

  function stringToInteger(value, nonNegative, defaultValue) {
    
    var result = /^[ \t\n\f\r]*([\+\-]?[0-9]+)/.exec(value);
    if (result) {
      var resultInt = parseInt(result[1], 10);
      if ((nonNegative ? 0 : -0x80000000) <= resultInt && resultInt <= 0x7FFFFFFF) {
        
        return resultInt;
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
    } else {
      is(element[attr], intValue, element.localName +
        ".setAttribute(" + attr + ", " + v + "), " + element.localName + "[" + attr + "] ");
    }
    element.removeAttribute(attr);

    if (nonNegative && expectedIdlAttributeResult(v) < 0) {
      try {
        element[attr] = v;
        ok(false, element.localName + "[" + attr + "] = " + v + " should throw IndexSizeError");
      } catch(e) {
        is(e.name, "IndexSizeError", element.localName + "[" + attr + "] = " + v +
          " should throw IndexSizeError");
        is(e.code, DOMException.INDEX_SIZE_ERR, element.localName + "[" + attr + "] = " + v +
          " should throw INDEX_SIZE_ERR");
      }
    } else {
      element[attr] = v;
      if (expectedIdlAttributeResult(v) == -2147483648 && element[attr] == defaultValue) {
        
        todo_is(element[attr], expectedIdlAttributeResult(v), "Bug 586761: " + element.localName + "[" +
          attr + "] = " + v + ", " + element.localName + "[" + attr + "] ");
      } else {
        is(element[attr], expectedIdlAttributeResult(v), element.localName + "[" + attr + "] = " + v +
          ", " + element.localName + "[" + attr + "] ");
        is(element.getAttribute(attr), String(expectedIdlAttributeResult(v)),
           element.localName + "[" + attr + "] = " + v + ", " +
           element.localName + ".getAttribute(" + attr + ") ");
      }
    }
    element.removeAttribute(attr);
  });

  
  is(element.getAttribute(attr), null,
     "When not set, the content attribute should be null.");
  is(element[attr], defaultValue,
     "When not set, the IDL attribute should return default value.");
}










function reflectURL(aParameters)
{
  var element = aParameters.element;
  var contentAttr = typeof aParameters.attribute === "string"
                      ? aParameters.attribute : aParameters.attribute.content;
  var idlAttr = typeof aParameters.attribute === "string"
                  ? aParameters.attribute : aParameters.attribute.idl;

  element[idlAttr] = "";
  is(element[idlAttr], document.URL, "Empty string should resolve to document URL");
}
