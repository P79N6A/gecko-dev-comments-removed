






































function test()
{
  ok(window.InspectorUI, "InspectorUI variable exists");
  ok(!InspectorUI.inspecting, "Inspector is not highlighting");
  is(InspectorStore.length, 0, "InspectorStore is empty");
  ok(InspectorStore.isEmpty(), "InspectorStore is empty (confirmed)");
  is(typeof InspectorStore.store, "object",
    "InspectorStore.store is an object");

  ok(InspectorStore.addStore("foo"), "addStore('foo') returns true");

  is(InspectorStore.length, 1, "InspectorStore.length = 1");
  ok(!InspectorStore.isEmpty(), "InspectorStore is not empty");
  is(typeof InspectorStore.store.foo, "object", "store.foo is an object");

  ok(InspectorStore.addStore("fooBar"), "addStore('fooBar') returns true");

  is(InspectorStore.length, 2, "InspectorStore.length = 2");
  is(typeof InspectorStore.store.fooBar, "object", "store.fooBar is an object");

  ok(!InspectorStore.addStore("fooBar"), "addStore('fooBar') returns false");

  ok(InspectorStore.deleteStore("fooBar"),
    "deleteStore('fooBar') returns true");

  is(InspectorStore.length, 1, "InspectorStore.length = 1");
  ok(!InspectorStore.store.fooBar, "store.fooBar is deleted");

  ok(!InspectorStore.deleteStore("fooBar"),
    "deleteStore('fooBar') returns false");

  ok(!InspectorStore.hasID("fooBar"), "hasID('fooBar') returns false");

  ok(InspectorStore.hasID("foo"), "hasID('foo') returns true");

  ok(InspectorStore.setValue("foo", "key1", "val1"), "setValue() returns true");

  ok(!InspectorStore.setValue("fooBar", "key1", "val1"),
    "setValue() returns false");

  is(InspectorStore.getValue("foo", "key1"), "val1",
    "getValue() returns the correct value");

  is(InspectorStore.store.foo.key1, "val1", "store.foo.key1 = 'val1'");

  ok(!InspectorStore.getValue("fooBar", "key1"),
    "getValue() returns null for unknown store");

  ok(!InspectorStore.getValue("fooBar", "key1"),
    "getValue() returns null for unknown store");

  ok(InspectorStore.deleteValue("foo", "key1"),
    "deleteValue() returns true for known value");

  ok(!InspectorStore.store.foo.key1, "deleteValue() removed the value.");

  ok(!InspectorStore.deleteValue("fooBar", "key1"),
    "deleteValue() returns false for unknown store.");

  ok(!InspectorStore.deleteValue("foo", "key1"),
    "deleteValue() returns false for unknown value.");

  ok(InspectorStore.deleteStore("foo"), "deleteStore('foo') returns true");

  ok(InspectorStore.isEmpty(), "InspectorStore is empty");
}

