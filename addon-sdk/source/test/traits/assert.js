



"use strict";

var BaseAssert = require("sdk/test/assert").Assert;








function equivalentDescriptors(actual, expected) {
  return (actual.conflict && expected.conflict) ||
         (actual.required && expected.required) ||
         equalDescriptors(actual, expected);
}

function equalDescriptors(actual, expected) {
  return actual.get === expected.get &&
         actual.set === expected.set &&
         actual.value === expected.value &&
         !!actual.enumerable === !!expected.enumerable &&
         !!actual.configurable === !!expected.configurable &&
         !!actual.writable === !!expected.writable;
}





function containsSet(source, target) {
  return source.some(function(element) {
    return 0 > target.indexOf(element);
  });
}




function equivalentSets(source, target) {
  return containsSet(source, target) && containsSet(target, source);
}






function findNonEquivalentPropertyName(source, target) {
  var value = null;
  Object.getOwnPropertyNames(source).some(function(key) {
    var areEquivalent = false;
    if (!equivalentDescriptors(source[key], target[key])) {
      value = key;
      areEquivalent = true;
    }
    return areEquivalent;
  });
  return value;
}

var AssertDescriptor = {
  equalTraits: {
    value: function equivalentTraits(actual, expected, message) {
      var difference;
      var actualKeys = Object.getOwnPropertyNames(actual);
      var expectedKeys = Object.getOwnPropertyNames(expected);

      if (equivalentSets(actualKeys, expectedKeys)) {
        this.fail({
          operator: "equalTraits",
          message: "Traits define different properties",
          actual: actualKeys.sort().join(","),
          expected: expectedKeys.sort().join(","),
        });
      }
      else if ((difference = findNonEquivalentPropertyName(actual, expected))) {
        this.fail({
          operator: "equalTraits",
          message: "Traits define non-equivalent property `" + difference + "`",
          actual: actual[difference],
          expected: expected[difference]
        });
      }
      else {
        this.pass(message || "Traits are equivalent.");
      }
    }
  }
};

exports.Assert = function Assert() {
  return Object.create(BaseAssert.apply(null, arguments), AssertDescriptor);
};
