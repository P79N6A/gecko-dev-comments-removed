





















function assert_promise_rejects(promise, code, description) {
  return promise.then(
    function() {
      throw 'assert_promise_rejects: ' + description + ' Promise did not reject.';
    },
    function(e) {
      if (code !== undefined) {
        assert_throws(code, function() { throw e; }, description);
      }
    });
}
















self.assert_object_equals = function(actual, expected, description) {
  var object_stack = [];

  function _is_equal(actual, expected, prefix) {
    if (typeof actual !== 'object') {
      assert_equals(actual, expected, prefix);
      return;
    }
    assert_true(typeof expected === 'object', prefix);
    assert_equals(object_stack.indexOf(actual), -1,
                  prefix + ' must not contain cyclic references.');

    object_stack.push(actual);

    Object.getOwnPropertyNames(expected).forEach(function(property) {
        assert_own_property(actual, property, prefix);
        _is_equal(actual[property], expected[property],
                  prefix + '.' + property);
      });
    Object.getOwnPropertyNames(actual).forEach(function(property) {
        assert_own_property(expected, property, prefix);
      });

    object_stack.pop();
  }

  function _brand(object) {
    return Object.prototype.toString.call(object).match(/^\[object (.*)\]$/)[1];
  }

  _is_equal(actual, expected,
            (description ? description + ': ' : '') + _brand(expected));
};



function assert_object_in_array(actual, expected_array, description) {
  assert_true(expected_array.some(function(element) {
      try {
        assert_object_equals(actual, element);
        return true;
      } catch (e) {
        return false;
      }
    }), description);
}






function assert_array_equivalent(actual, expected, description) {
  assert_true(Array.isArray(actual), description);
  assert_equals(actual.length, expected.length, description);
  expected.forEach(function(expected_element) {
      
      
      
      
      assert_object_in_array(expected_element, actual, description);
    });
}




function assert_array_objects_equals(actual, expected, description) {
  assert_true(Array.isArray(actual), description);
  assert_equals(actual.length, expected.length, description);
  actual.forEach(function(value, index) {
      assert_object_equals(value, expected[index],
                           description + ' : object[' + index + ']');
    });
}











function assert_will_be_idl_attribute(object, attribute_name, description) {
  assert_true(typeof object === "object", description);

  assert_true("hasOwnProperty" in object, description);

  
  
  

  assert_true(attribute_name in object, description);
}




function stringifyDOMObject(object)
{
    function deepCopy(src) {
        if (typeof src != "object")
            return src;
        var dst = Array.isArray(src) ? [] : {};
        for (var property in src) {
            dst[property] = deepCopy(src[property]);
        }
        return dst;
    }
    return JSON.stringify(deepCopy(object));
}
