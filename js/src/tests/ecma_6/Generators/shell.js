




function assertFalse(a) { assertEq(a, false) }
function assertTrue(a) { assertEq(a, true) }
function assertNotEq(found, not_expected) { assertFalse(found === expected) }
function assertIteratorResult(value, done, result) {
    assertDeepEq(result, { value: value, done: done });
}
