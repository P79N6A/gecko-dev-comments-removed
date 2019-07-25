























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
  todo_is(element.getAttribute(contentAttr), "null",
     "null should have been stringified to 'null'");
  todo_is(element[idlAttr], "null",
     "null should have been stringified to 'null'");
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
  var attr = aParameters.attribute;
  var validValues = aParameters.validValues;
  var invalidValues = aParameters.invalidValues;
  var defaultValue = aParameters.defaultValue !== undefined
    ? aParameters.defaultValue : "";
  var unsupportedValues = aParameters.unsupportedValues !== undefined
    ? aParameters.unsupportedValues : [];

  ok(attr in element, attr + " should be an IDL attribute of this element");
  is(typeof element[attr], "string", attr + " IDL attribute should be a string");

  
  element.removeAttribute(attr);
  is(element[attr], defaultValue,
     "When no attribute is set, the value should be the default value.");

  
  validValues.forEach(function (v) {
    element.setAttribute(attr, v);
    is(element[attr], v,
       v + " should be accepted as a valid value for " + attr);
    is(element.getAttribute(attr), v,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(attr);

    element.setAttribute(attr, v.toUpperCase());
    is(element[attr], v,
       "Enumerated attributes should be case-insensitive.");
    is(element.getAttribute(attr), v.toUpperCase(),
       "Content attribute should not be lower-cased.");
    element.removeAttribute(attr);

    element[attr] = v;
    is(element[attr], v,
       v + " should be accepted as a valid value for " + attr);
    is(element.getAttribute(attr), v,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(attr);

    element[attr] = v.toUpperCase();
    is(element[attr], v,
       "Enumerated attributes should be case-insensitive.");
    is(element.getAttribute(attr), v.toUpperCase(),
       "Content attribute should not be lower-cased.");
    element.removeAttribute(attr);
  });

  
  invalidValues.forEach(function (v) {
    element.setAttribute(attr, v);
    is(element[attr], defaultValue,
       "When the content attribute is set to an invalid value, the default value should be returned.");
    is(element.getAttribute(attr), v,
       "Content attribute should not have been changed.");
    element.removeAttribute(attr);

    element[attr] = v;
    is(element[attr], defaultValue,
       "When the value is set to an invalid value, the default value should be returned.");
    is(element.getAttribute(attr), v,
       "Content attribute should not have been changed.");
    element.removeAttribute(attr);
  });

  
  
  unsupportedValues.forEach(function (v) {
    element.setAttribute(attr, v);
    todo_is(element[attr], v,
            v + " should be accepted as a valid value for " + attr);
    is(element.getAttribute(attr), v,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(attr);

    element.setAttribute(attr, v.toUpperCase());
    todo_is(element[attr], v,
            "Enumerated attributes should be case-insensitive.");
    is(element.getAttribute(attr), v.toUpperCase(),
       "Content attribute should not be lower-cased.");
    element.removeAttribute(attr);

    element[attr] = v;
    todo_is(element[attr], v,
            v + " should be accepted as a valid value for " + attr);
    is(element.getAttribute(attr), v,
       "Content attribute should return the value it has been set to.");
    element.removeAttribute(attr);

    element[attr] = v.toUpperCase();
    todo_is(element[attr], v,
            "Enumerated attributes should be case-insensitive.");
    is(element.getAttribute(attr), v.toUpperCase(),
       "Content attribute should not be lower-cased.");
    element.removeAttribute(attr);
  });
}

