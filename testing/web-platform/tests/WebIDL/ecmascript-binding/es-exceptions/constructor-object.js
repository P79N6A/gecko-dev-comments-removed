function run_test() {
    test(function() {
        
        
        
        
        
        
        assert_own_property(self, "DOMException",
                            "self does not have own property \"DOMException\"");
        var desc = Object.getOwnPropertyDescriptor(self, "DOMException");
        assert_false("get" in desc, "self's property \"DOMException\" has getter");
        assert_false("set" in desc, "self's property \"DOMException\" has setter");
        assert_true(desc.writable, "self's property \"DOMException\" is not writable");
        assert_false(desc.enumerable, "self's property \"DOMException\" is enumerable");
        assert_true(desc.configurable, "self's property \"DOMException\" is not configurable");

        
        
        assert_equals(Object.getPrototypeOf(self.DOMException), Error,
                      "prototype of self's property \"DOMException\" is not Error");

        
        
        
        
        
        
        

        
        
        assert_class_string(self.DOMException, "Function",
                            "class string of DOMException");

        
        
        
        
        
        
    }, "existence and properties of DOMException");

    test(function() {
        assert_own_property(self, "DOMException",
                            "self does not have own property \"DOMException\"");

        
        
        
        
        
        assert_own_property(self.DOMException, "prototype",
                            'exception "DOMException" does not have own property "prototype"');
        var desc = Object.getOwnPropertyDescriptor(self.DOMException, "prototype");
        assert_false("get" in desc, "DOMException.prototype has getter");
        assert_false("set" in desc, "DOMException.prototype has setter");
        assert_false(desc.writable, "DOMException.prototype is writable");
        assert_false(desc.enumerable, "DOMException.prototype is enumerable");
        assert_false(desc.configurable, "DOMException.prototype is configurable");

        
        
        
        assert_own_property(self, "Error",
                            'should inherit from Error, but self has no such property');
        assert_own_property(self.Error, "prototype",
                            'should inherit from Error, but that object has no "prototype" property');
        assert_equals(Object.getPrototypeOf(self.DOMException.prototype),
                      self.Error.prototype,
                      'prototype of DOMException.prototype is not Error.prototype');

        
        
        assert_class_string(self.DOMException.prototype, "DOMExceptionPrototype",
                            "class string of DOMException.prototype");
    }, "existence and properties of DOMException.prototype");

    test(function() {
        assert_false(self.DOMException.prototype.hasOwnProperty("name"),
                     "DOMException.prototype should not have an own \"name\" " +
                     "property.");
        assert_false(self.DOMException.prototype.hasOwnProperty("code"),
                     "DOMException.prototype should not have an own \"name\" " +
                     "property.");
    }, "existence of name and code properties on DOMException.prototype");

    test(function() {
        assert_own_property(self, "DOMException",
                            "self does not have own property \"DOMException\"");
        assert_own_property(self.DOMException, "prototype",
                            'interface "DOMException" does not have own property "prototype"');

        
        
        
        
        assert_own_property(self.DOMException.prototype, "constructor",
                            "DOMException" + '.prototype does not have own property "constructor"');
        var desc = Object.getOwnPropertyDescriptor(self.DOMException.prototype, "constructor");
        assert_false("get" in desc, "DOMException.prototype.constructor has getter");
        assert_false("set" in desc, "DOMException.prototype.constructor has setter");
        assert_true(desc.writable, "DOMException.prototype.constructor is not writable");
        assert_false(desc.enumerable, "DOMException.prototype.constructor is enumerable");
        assert_true(desc.configurable, "DOMException.prototype.constructor in not configurable");
        assert_equals(self.DOMException.prototype.constructor, self.DOMException,
                      "DOMException.prototype.constructor is not the same object as DOMException");
    }, "existence and properties of exception interface prototype object's \"constructor\" property");

    done();
}
