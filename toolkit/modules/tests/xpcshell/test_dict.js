



Components.utils.import("resource://gre/modules/Dict.jsm");




function test_get_set_has_del() {
  let dict = new Dict({foo: "bar"});
  dict.set("baz", 200);
  do_check_eq(dict.get("foo"), "bar");
  do_check_eq(dict.get("baz"), 200);
  do_check_true(dict.has("foo"));
  do_check_true(dict.has("baz"));
  
  do_check_true(dict.del("foo"));
  do_check_true(dict.del("baz"));
  do_check_false(dict.has("foo"));
  do_check_false(dict.has("baz"));
  
  do_check_false(dict.del("foo"));
  do_check_false(dict.del("baz"));
}




function test_get_default() {
  let dict = new Dict();
  do_check_true(dict.get("foo") === undefined);
  do_check_eq(dict.get("foo", "bar"), "bar");
}




function test_collisions_with_builtins() {
  let dict = new Dict();
  
  do_check_false(dict.has("toString"));
  do_check_false(dict.has("watch"));
  do_check_false(dict.has("__proto__"));

  
  dict.set("toString", "toString");
  dict.set("watch", "watch");
  
  
  dict.set("__proto__", {prototest: "prototest"});

  
  do_check_true(dict.has("toString"));
  do_check_true(dict.has("watch"));
  do_check_true(dict.has("__proto__"));
  
  do_check_false(dict.has("prototest"));
}




function test_count() {
  let dict = new Dict({foo: "bar"});
  do_check_eq(dict.count, 1);
  dict.set("baz", "quux");
  do_check_eq(dict.count, 2);
  
  dict.set("baz", "quux2");
  do_check_eq(dict.count, 2);

  do_check_true(dict.del("baz"));
  do_check_eq(dict.count, 1);
  
  do_check_false(dict.del("not"));
  do_check_eq(dict.count, 1);
  do_check_true(dict.del("foo"));
  do_check_eq(dict.count, 0);
}




function test_copy() {
  let obj = {};
  let dict1 = new Dict({foo: "bar", baz: obj});
  let dict2 = dict1.copy();
  do_check_eq(dict2.get("foo"), "bar");
  do_check_eq(dict2.get("baz"), obj);
  
  dict1.del("foo");
  do_check_false(dict1.has("foo"));
  do_check_true(dict2.has("foo"));
  dict2.set("test", 400);
  do_check_true(dict2.has("test"));
  do_check_false(dict1.has("test"));

  
  dict1.get("baz").prop = "proptest";
  do_check_eq(dict2.get("baz").prop, "proptest");
}


function _check_lists(keys, values, items) {
  do_check_eq(keys.length, 2);
  do_check_true(keys.indexOf("x") != -1);
  do_check_true(keys.indexOf("y") != -1);

  do_check_eq(values.length, 2);
  do_check_true(values.indexOf("a") != -1);
  do_check_true(values.indexOf("b") != -1);

  
  
  do_check_eq(items.length, 2);
  do_check_eq(items[0].length, 2);
  do_check_eq(items[1].length, 2);
  let ix = (items[0][0] == "x") ? 0 : 1;
  let iy = (ix == 0) ? 1 : 0;
  do_check_eq(items[ix][0], "x");
  do_check_eq(items[ix][1], "a");
  do_check_eq(items[iy][0], "y");
  do_check_eq(items[iy][1], "b");
}




function test_listers() {
  let dict = new Dict({"x": "a", "y": "b"});
  let keys = dict.listkeys();
  let values = dict.listvalues();
  let items = dict.listitems();
  _check_lists(keys, values, items);
}




function test_iterators() {
  let dict = new Dict({"x": "a", "y": "b"});
  
  let keys = [x for (x in dict.keys)];
  let values = [x for (x in dict.values)];
  let items = [x for (x in dict.items)];
  _check_lists(keys, values, items);
}




function test_set_property_strict() {
  "use strict";
  var dict = new Dict();
  var thrown = false;
  try {
    dict.foo = "bar";
  }
  catch (ex) {
    thrown = true;
  }
  do_check_true(thrown);
}




function test_set_property_non_strict() {
  let dict = new Dict();
  dict.foo = "bar";
  do_check_false("foo" in dict);
  let realget = dict.get;
  dict.get = "baz";
  do_check_eq(dict.get, realget);
}




function test_set_property_lazy_getter() {
  let thunkCalled = false;

  let setThunk = function(dict) {
    thunkCalled = false;
    dict.setAsLazyGetter("foo", function() {
      thunkCalled = true;
      return "bar";
    });
  };

  let dict = new Dict();
  setThunk(dict);

  
  
  do_check_true(dict.has("foo"));
  do_check_false(thunkCalled);
  do_check_true(dict.isLazyGetter("foo"));

  
  
  do_check_eq(dict.get("foo"), "bar");
  do_check_true(thunkCalled);
  do_check_false(dict.isLazyGetter("foo"));

  
  thunkCalled = false;
  do_check_eq(dict.get("foo"), "bar");
  do_check_false(thunkCalled);
  do_check_false(dict.isLazyGetter("foo"));

  
  dict = new Dict();
  setThunk(dict);
  do_check_true(dict.isLazyGetter("foo"));

  let listvalues = dict.listvalues();
  do_check_false(dict.isLazyGetter("foo"));
  do_check_true(thunkCalled);
  do_check_true(listvalues.length, 1);
  do_check_eq(listvalues[0], "bar");

  thunkCalled = false;

  
  listvalues = dict.listvalues();
  do_check_false(dict.isLazyGetter("foo"));
  do_check_false(thunkCalled);
  do_check_true(listvalues.length, 1);
  do_check_eq(listvalues[0], "bar");

  
  dict = new Dict();
  setThunk(dict);
  let values = dict.values;

  
  do_check_true(dict.isLazyGetter("foo"));
  do_check_false(thunkCalled);
  do_check_eq(values.next(), "bar");
  do_check_true(thunkCalled);

  thunkCalled = false;
  do_check_false(dict.isLazyGetter("foo"));
  do_check_eq(dict.get("foo"), "bar");
  do_check_false(thunkCalled);
}


function _sort_comp_arr(arr1,arr2){
  arr1.sort();
  arr2.sort();
  do_check_eq(arr1.toString(),arr2.toString());
}




function test_construct_dict_from_json_string() {
  let d1 = new Dict({a:1, b:2, c:"foobar"});
  let d2 = new Dict(JSON.stringify(({a:1, b:2, c:"foobar"})));
  _sort_comp_arr(d1.listkeys(),d2.listkeys());
  do_check_eq(d1.get("a"), d2.get("a"));
  do_check_eq(d1.get("b"), d2.get("b"));
  do_check_eq(d1.get("c"), d2.get("c"));
}




function test_serialize_dict_to_json_string() {
  let d1 = new Dict({a:1, b:2, c:"foobar"});
  let d2 = new Dict(d1.toJSON());
  _sort_comp_arr(d1.listkeys(),d2.listkeys());
  do_check_eq(d1.get("a"), d2.get("a"));
  do_check_eq(d1.get("b"), d2.get("b"));
  do_check_eq(d1.get("c"), d2.get("c"));
}

var tests = [
  test_get_set_has_del,
  test_get_default,
  test_collisions_with_builtins,
  test_count,
  test_copy,
  test_listers,
  test_iterators,
  test_set_property_strict,
  test_set_property_non_strict,
  test_set_property_lazy_getter,
  test_construct_dict_from_json_string,
  test_serialize_dict_to_json_string
];

function run_test() {
  for (let [, test] in Iterator(tests))
    test();
}
