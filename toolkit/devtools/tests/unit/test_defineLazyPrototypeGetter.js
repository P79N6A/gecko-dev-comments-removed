





function Class() {}
DevToolsUtils.defineLazyPrototypeGetter(Class.prototype, "foo", () => []);


function run_test() {
  test_prototype_attributes();
  test_instance_attributes();
  test_multiple_instances();
  test_callback_receiver();
}

function test_prototype_attributes() {
  
  let descriptor = Object.getOwnPropertyDescriptor(Class.prototype, "foo");
  do_check_eq(typeof descriptor.get, "function");
  do_check_eq(descriptor.set, undefined);
  do_check_eq(descriptor.enumerable, false);
  do_check_eq(descriptor.configurable, true);
}

function test_instance_attributes() {
  
  
  let instance = new Class();
  do_check_false(instance.hasOwnProperty("foo"));
  instance.foo;
  do_check_true(instance.hasOwnProperty("foo"));

  
  
  let descriptor = Object.getOwnPropertyDescriptor(instance, "foo");
  do_check_true(descriptor.value instanceof Array);
  do_check_eq(descriptor.writable, true);
  do_check_eq(descriptor.enumerable, false);
  do_check_eq(descriptor.configurable, true);
}

function test_multiple_instances() {
  let instance1 = new Class();
  let instance2 = new Class();
  let foo1 = instance1.foo;
  let foo2 = instance2.foo;
  
  do_check_true(foo1 instanceof Array);
  do_check_true(foo2 instanceof Array);
  
  do_check_eq(instance1.foo, foo1);
  do_check_eq(instance2.foo, foo2);
  
  do_check_neq(foo1, foo2);
}

function test_callback_receiver() {
  function Foo() {};
  DevToolsUtils.defineLazyPrototypeGetter(Foo.prototype, "foo", function() {
    return this;
  });

  
  let instance = new Foo();
  do_check_eq(instance.foo, instance);
}
