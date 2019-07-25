



function reflectUnsignedInt(aElement, aAttr, aNonNull, aDefault)
{
  function checkGetter(aElement, aAttr, aValue)
  {
    is(aElement[aAttr], aValue, "." + aAttr + " should be equals " + aValue);
    is(aElement.getAttribute(aAttr), aValue,
       "@" + aAttr + " should be equals " + aValue);
  }

  if (!aDefault) {
    if (aNonNull) {
      aDefault = 1;
    } else {
      aDefault = 0;
    }
  }

  
  is(aElement[aAttr], aDefault, "default value should be " + aDefault);
  ok(!aElement.hasAttribute(aAttr), aAttr + " shouldn't be present");

  var values = [ 1, 3, 42, 2147483647 ];

  for each (var value in values) {
    aElement[aAttr] = value;
    checkGetter(aElement, aAttr, value);
  }

  for each (var value in values) {
    aElement.setAttribute(aAttr, value);
    checkGetter(aElement, aAttr, value);
  }

  
  aElement[aAttr] = -3000000000;
  checkGetter(aElement, aAttr, 1294967296);
  
  aElement.setAttribute(aAttr, -3000000000);
  is(aElement.getAttribute(aAttr), -3000000000,
     "@" + aAttr + " should be equals to " + -3000000000);
  is(aElement[aAttr], aDefault,
     "." + aAttr + " should be equals to " + aDefault);

  var nonValidValues = [
    
    [ -2147483648, 2147483648 ],
    [ -1,          4294967295 ],
    [ 3147483647,  3147483647 ],
  ];

  for each (var values in nonValidValues) {
    aElement[aAttr] = values[0];
    is(aElement.getAttribute(aAttr), values[1],
       "@" + aAttr + " should be equals to " + values[1]);
    is(aElement[aAttr], aDefault,
       "." + aAttr + " should be equals to " + aDefault);
  }

  for each (var values in nonValidValues) {
    aElement.setAttribute(aAttr, values[0]);
    is(aElement.getAttribute(aAttr), values[0],
       "@" + aAttr + " should be equals to " + values[0]);
    is(aElement[aAttr], aDefault,
       "." + aAttr + " should be equals to " + aDefault);
  }

  
  var caught = false;
  try {
    aElement[aAttr] = 0;
  } catch(e) {
    caught = true;
    is(e.code, DOMException.INDEX_SIZE_ERR, "exception should be INDEX_SIZE_ERR");
  }

  if (aNonNull) {
    ok(caught, "an exception should have been caught");
  } else {
    ok(!caught, "no exception should have been caught");
  }

  
  aElement.setAttribute(aAttr, 0);
  is(aElement.getAttribute(aAttr), 0, "@" + aAttr + " should be equals to 0");
  if (aNonNull) {
    is(aElement[aAttr], aDefault,
       "." + aAttr + " should be equals to " + aDefault);
  } else {
    is(aElement[aAttr], 0, "." + aAttr + " should be equals to 0");
  }
}







function reflectLimitedEnumerated(aElement, aAttr, aSupportedValues,
                                  aUnsupportedValues)
{
  aSupportedValues.forEach(function (v) {
    aElement.setAttribute(aAttr, v);
    is(aElement[aAttr], v);
    is(aElement.getAttribute(aAttr), v);
    aElement.removeAttribute(aAttr);

    aElement.setAttribute(aAttr, v.toUpperCase());
    is(aElement[aAttr], v);
    is(aElement.getAttribute(aAttr), v.toUpperCase());
    aElement.removeAttribute(aAttr);

    aElement[aAttr] = v;
    is(aElement[aAttr], v);
    is(aElement.getAttribute(aAttr), v);
    aElement.removeAttribute(aAttr);

    aElement[aAttr] = v.toUpperCase();
    is(aElement[aAttr], v);
    is(aElement.getAttribute(aAttr), v.toUpperCase());
    aElement.removeAttribute(aAttr);
  });
  ["cheesecake"].concat(aUnsupportedValues).forEach(function (v) {
    aElement.setAttribute(aAttr, v);
    is(aElement[aAttr], "");
    is(aElement.getAttribute(aAttr), v);
    aElement.removeAttribute(aAttr);

    aElement[aAttr] = v;
    is(aElement[aAttr], "");
    is(aElement.getAttribute(aAttr), v);
    aElement.removeAttribute(aAttr);
  });
}

