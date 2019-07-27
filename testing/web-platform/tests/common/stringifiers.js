
function test_stringifier_attribute(aObject, aAttribute, aIsUnforgeable) {
  
  test(function() {
    [null, undefined].forEach(function(v) {
      assert_throws(new TypeError(), function() {
        aObject.toString.call(v);
      });
    });
  });

  

  
  test(function() {
    assert_false("Window" in window && aObject instanceof window.Window);
    [{}, window].forEach(function(v) {
      assert_throws(new TypeError(), function() {
        aObject.toString.call(v)
      });
    });
  });

  
  var expected_value;
  test(function() {
    expected_value = aObject[aAttribute];
    assert_equals(aObject[aAttribute], expected_value,
                  "The attribute " + aAttribute + " should be pure.");
  });

  var test_error = { name: "test" };
  test(function() {
    if (!aIsUnforgeable) {
      Object.defineProperty(aObject, aAttribute, {
        configurable: true,
        get: function() { throw test_error; }
      });
    }
    assert_equals(aObject.toString(), expected_value);
  });

  test(function() {
    if (!aIsUnforgeable) {
      Object.defineProperty(aObject, aAttribute, {
        configurable: true,
        value: { toString: function() { throw test_error; } }
      });
    }
    assert_equals(aObject.toString(), expected_value);
  });
}
