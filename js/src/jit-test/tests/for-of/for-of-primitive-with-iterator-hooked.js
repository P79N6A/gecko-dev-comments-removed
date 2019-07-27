



"use strict";

load(libdir + 'asserts.js');
load(libdir + 'iteration.js');

Object.defineProperty(Boolean.prototype, Symbol.iterator,
{
  get() {
    assertEq(typeof this, "object",
             "GetMethod(obj, @@iterator) internally first performs ToObject");
    assertEq(this !== null, true,
             "this is really an object, not null");

    function FakeBooleanIterator()
    {
      assertEq(typeof this, "boolean",
               "iterator creation code must perform ToObject(false) before " +
               "doing a lookup for @@iterator");
      return { next() { return { done: true }; } };
    }

    return FakeBooleanIterator;
  },
  configurable: true
});

for (var i of false)
  assertEq(true, false, "not reached");
