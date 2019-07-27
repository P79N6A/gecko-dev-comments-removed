















































(function(){
"use strict";

function constValue (cnt) {
    if (cnt.type === "null") return null;
    if (cnt.type === "NaN") return NaN;
    if (cnt.type === "Infinity") return cnt.negative ? -Infinity : Infinity;
    return cnt.value;
}



self.IdlArray = function()

{
    




    this.members = {};

    









    this.objects = {};

    















    this.partials = [];
    this["implements"] = {};
};


IdlArray.prototype.add_idls = function(raw_idls)

{
    
    this.internal_add_idls(WebIDL2.parse(raw_idls));
};


IdlArray.prototype.add_untested_idls = function(raw_idls)

{
    
    var parsed_idls = WebIDL2.parse(raw_idls);
    for (var i = 0; i < parsed_idls.length; i++)
    {
        parsed_idls[i].untested = true;
        if ("members" in parsed_idls[i])
        {
            for (var j = 0; j < parsed_idls[i].members.length; j++)
            {
                parsed_idls[i].members[j].untested = true;
            }
        }
    }
    this.internal_add_idls(parsed_idls);
};


IdlArray.prototype.internal_add_idls = function(parsed_idls)

{
    







    parsed_idls.forEach(function(parsed_idl)
    {
        if (parsed_idl.type == "interface" && parsed_idl.partial)
        {
            this.partials.push(parsed_idl);
            return;
        }

        if (parsed_idl.type == "implements")
        {
            if (!(parsed_idl.target in this["implements"]))
            {
                this["implements"][parsed_idl.target] = [];
            }
            this["implements"][parsed_idl.target].push(parsed_idl["implements"]);
            return;
        }

        parsed_idl.array = this;
        if (parsed_idl.name in this.members)
        {
            throw "Duplicate identifier " + parsed_idl.name;
        }
        switch(parsed_idl.type)
        {
        case "interface":
            this.members[parsed_idl.name] =
                new IdlInterface(parsed_idl,  false);
            break;

        case "dictionary":
            
            
            this.members[parsed_idl.name] = new IdlDictionary(parsed_idl);
            break;

        case "typedef":
            this.members[parsed_idl.name] = new IdlTypedef(parsed_idl);
            break;

        case "callback":
            
            console.log("callback not yet supported");
            break;

        case "enum":
            this.members[parsed_idl.name] = new IdlEnum(parsed_idl);
            break;

        case "callback interface":
            this.members[parsed_idl.name] =
                new IdlInterface(parsed_idl,  true);
            break;

        default:
            throw parsed_idl.name + ": " + parsed_idl.type + " not yet supported";
        }
    }.bind(this));
};


IdlArray.prototype.add_objects = function(dict)

{
    
    for (var k in dict)
    {
        if (k in this.objects)
        {
            this.objects[k] = this.objects[k].concat(dict[k]);
        }
        else
        {
            this.objects[k] = dict[k];
        }
    }
};


IdlArray.prototype.prevent_multiple_testing = function(name)

{
    
    this.members[name].prevent_multiple_testing = true;
};


IdlArray.prototype.recursively_get_implements = function(interface_name)

{
    









    var ret = this["implements"][interface_name];
    if (ret === undefined)
    {
        return [];
    }
    for (var i = 0; i < this["implements"][interface_name].length; i++)
    {
        ret = ret.concat(this.recursively_get_implements(ret[i]));
        if (ret.indexOf(ret[i]) != ret.lastIndexOf(ret[i]))
        {
            throw "Circular implements statements involving " + ret[i];
        }
    }
    return ret;
};


IdlArray.prototype.test = function()

{
    

    
    
    this.partials.forEach(function(parsed_idl)
    {
        if (!(parsed_idl.name in this.members)
        || !(this.members[parsed_idl.name] instanceof IdlInterface))
        {
            throw "Partial interface " + parsed_idl.name + " with no original interface";
        }
        if (parsed_idl.extAttrs)
        {
            parsed_idl.extAttrs.forEach(function(extAttr)
            {
                this.members[parsed_idl.name].extAttrs.push(extAttr);
            }.bind(this));
        }
        parsed_idl.members.forEach(function(member)
        {
            this.members[parsed_idl.name].members.push(new IdlInterfaceMember(member));
        }.bind(this));
    }.bind(this));
    this.partials = [];

    for (var lhs in this["implements"])
    {
        this.recursively_get_implements(lhs).forEach(function(rhs)
        {
            var errStr = lhs + " implements " + rhs + ", but ";
            if (!(lhs in this.members)) throw errStr + lhs + " is undefined.";
            if (!(this.members[lhs] instanceof IdlInterface)) throw errStr + lhs + " is not an interface.";
            if (!(rhs in this.members)) throw errStr + rhs + " is undefined.";
            if (!(this.members[rhs] instanceof IdlInterface)) throw errStr + rhs + " is not an interface.";
            this.members[rhs].members.forEach(function(member)
            {
                this.members[lhs].members.push(new IdlInterfaceMember(member));
            }.bind(this));
        }.bind(this));
    }
    this["implements"] = {};

    
    for (var name in this.members)
    {
        this.members[name].test();
        if (name in this.objects)
        {
            this.objects[name].forEach(function(str)
            {
                this.members[name].test_object(str);
            }.bind(this));
        }
    }
};


IdlArray.prototype.assert_type_is = function(value, type)

{
    






    if (type.idlType == "any")
    {
        
        return;
    }

    if (type.nullable && value === null)
    {
        
        return;
    }

    if (type.array)
    {
        
        return;
    }

    if (type.sequence)
    {
        assert_true(Array.isArray(value), "is not array");
        if (!value.length)
        {
            
            return;
        }
        this.assert_type_is(value[0], type.idlType.idlType);
        return;
    }

    type = type.idlType;

    switch(type)
    {
        case "void":
            assert_equals(value, undefined);
            return;

        case "boolean":
            assert_equals(typeof value, "boolean");
            return;

        case "byte":
            assert_equals(typeof value, "number");
            assert_equals(value, Math.floor(value), "not an integer");
            assert_true(-128 <= value && value <= 127, "byte " + value + " not in range [-128, 127]");
            return;

        case "octet":
            assert_equals(typeof value, "number");
            assert_equals(value, Math.floor(value), "not an integer");
            assert_true(0 <= value && value <= 255, "octet " + value + " not in range [0, 255]");
            return;

        case "short":
            assert_equals(typeof value, "number");
            assert_equals(value, Math.floor(value), "not an integer");
            assert_true(-32768 <= value && value <= 32767, "short " + value + " not in range [-32768, 32767]");
            return;

        case "unsigned short":
            assert_equals(typeof value, "number");
            assert_equals(value, Math.floor(value), "not an integer");
            assert_true(0 <= value && value <= 65535, "unsigned short " + value + " not in range [0, 65535]");
            return;

        case "long":
            assert_equals(typeof value, "number");
            assert_equals(value, Math.floor(value), "not an integer");
            assert_true(-2147483648 <= value && value <= 2147483647, "long " + value + " not in range [-2147483648, 2147483647]");
            return;

        case "unsigned long":
            assert_equals(typeof value, "number");
            assert_equals(value, Math.floor(value), "not an integer");
            assert_true(0 <= value && value <= 4294967295, "unsigned long " + value + " not in range [0, 4294967295]");
            return;

        case "long long":
            assert_equals(typeof value, "number");
            return;

        case "unsigned long long":
        case "DOMTimeStamp":
            assert_equals(typeof value, "number");
            assert_true(0 <= value, "unsigned long long is negative");
            return;

        case "float":
        case "double":
        case "DOMHighResTimeStamp":
        case "unrestricted float":
        case "unrestricted double":
            
            assert_equals(typeof value, "number");
            return;

        case "DOMString":
        case "ByteString":
        case "USVString":
            
            assert_equals(typeof value, "string");
            return;

        case "object":
            assert_true(typeof value == "object" || typeof value == "function", "wrong type: not object or function");
            return;
    }

    if (!(type in this.members))
    {
        throw "Unrecognized type " + type;
    }

    if (this.members[type] instanceof IdlInterface)
    {
        
        
        
        
        
        assert_true(typeof value == "object" || typeof value == "function", "wrong type: not object or function");
        if (value instanceof Object
        && !this.members[type].has_extended_attribute("NoInterfaceObject")
        && type in self)
        {
            assert_true(value instanceof self[type], "not instanceof " + type);
        }
    }
    else if (this.members[type] instanceof IdlEnum)
    {
        assert_equals(typeof value, "string");
    }
    else if (this.members[type] instanceof IdlDictionary)
    {
        
    }
    else if (this.members[type] instanceof IdlTypedef)
    {
        
    }
    else
    {
        throw "Type " + type + " isn't an interface or dictionary";
    }
};



function IdlObject() {}
IdlObject.prototype.test = function()

{
    



};


IdlObject.prototype.has_extended_attribute = function(name)

{
    



    return this.extAttrs.some(function(o)
    {
        return o.name == name;
    });
};





function IdlDictionary(obj)

{
    




    
    this.name = obj.name;

    
    this.members = obj.members;

    



    this.base = obj.inheritance;
}


IdlDictionary.prototype = Object.create(IdlObject.prototype);


function IdlInterface(obj, is_callback) {
    




    
    this.name = obj.name;

    
    this.array = obj.array;

    




    this.untested = obj.untested;

    
    this.extAttrs = obj.extAttrs;

    
    this.members = obj.members.map(function(m){return new IdlInterfaceMember(m); });
    if (this.has_extended_attribute("Unforgeable")) {
        this.members
            .filter(m => !m["static"] && (m.type == "attribute" || m.type == "operation"))
            .forEach(m => m.isUnforgeable = true);
    }

    



    this.base = obj.inheritance;

    this._is_callback = is_callback;
}
IdlInterface.prototype = Object.create(IdlObject.prototype);
IdlInterface.prototype.is_callback = function()

{
    return this._is_callback;
};


IdlInterface.prototype.has_constants = function()

{
    return this.members.some(function(member) {
        return member.type === "const";
    });
};


IdlInterface.prototype.is_global = function()

{
    return this.extAttrs.some(function(attribute) {
        return attribute.name === "Global" ||
               attribute.name === "PrimaryGlobal";
    });
};


IdlInterface.prototype.test = function()

{
    if (this.has_extended_attribute("NoInterfaceObject"))
    {
        
        
        
        return;
    }

    if (!this.untested)
    {
        
        
        this.test_self();
    }
    
    
    
    
    
    
    
    this.test_members();
};


IdlInterface.prototype.test_self = function()

{
    test(function()
    {
        
        

        
        
        
        
        
        
        
        
        
        
        if (this.is_callback() && !this.has_constants()) {
            return;
        }

        
        
        assert_own_property(self, this.name,
                            "self does not have own property " + format_value(this.name));
        var desc = Object.getOwnPropertyDescriptor(self, this.name);
        assert_false("get" in desc, "self's property " + format_value(this.name) + " has getter");
        assert_false("set" in desc, "self's property " + format_value(this.name) + " has setter");
        assert_true(desc.writable, "self's property " + format_value(this.name) + " is not writable");
        assert_false(desc.enumerable, "self's property " + format_value(this.name) + " is enumerable");
        assert_true(desc.configurable, "self's property " + format_value(this.name) + " is not configurable");

        if (this.is_callback()) {
            
            
            assert_equals(Object.getPrototypeOf(self[this.name]), Object.prototype,
                          "prototype of self's property " + format_value(this.name) + " is not Object.prototype");

            return;
        }

        
        
        
        

        
        

        
        
        

        
        
        
        

        
        
        

        
        
        
        assert_class_string(self[this.name], "Function", "class string of " + this.name);

        
        
        var prototype = Object.getPrototypeOf(self[this.name]);
        if (this.base) {
            
            
            
            var has_interface_object =
                !this.array
                     .members[this.base]
                     .has_extended_attribute("NoInterfaceObject");
            if (has_interface_object) {
                assert_own_property(self, this.base,
                                    'should inherit from ' + this.base +
                                    ', but self has no such property');
                assert_equals(prototype, self[this.base],
                              'prototype of ' + this.name + ' is not ' +
                              this.base);
            }
        } else {
            
            
            
            assert_equals(prototype, Function.prototype,
                          "prototype of self's property " + format_value(this.name) + " is not Function.prototype");
        }

        if (!this.has_extended_attribute("Constructor")) {
            
            
            
            
            
            assert_throws(new TypeError(), function() {
                self[this.name]();
            }.bind(this), "interface object didn't throw TypeError when called as a function");
            assert_throws(new TypeError(), function() {
                new self[this.name]();
            }.bind(this), "interface object didn't throw TypeError when called as a constructor");
        }
    }.bind(this), this.name + " interface: existence and properties of interface object");

    if (!this.is_callback()) {
        test(function() {
            
            

            assert_own_property(self, this.name,
                                "self does not have own property " + format_value(this.name));

            
            
            
            
            assert_own_property(self[this.name], "length");
            var desc = Object.getOwnPropertyDescriptor(self[this.name], "length");
            assert_false("get" in desc, this.name + ".length has getter");
            assert_false("set" in desc, this.name + ".length has setter");
            assert_false(desc.writable, this.name + ".length is writable");
            assert_false(desc.enumerable, this.name + ".length is enumerable");
            assert_true(desc.configurable, this.name + ".length is not configurable");

            var constructors = this.extAttrs
                .filter(function(attr) { return attr.name == "Constructor"; });
            var expected_length;
            if (!constructors.length) {
                
                
                expected_length = 0;
            } else {
                
                
                
                expected_length = constructors.map(function(attr) {
                    return attr.arguments ? attr.arguments.filter(function(arg) {
                        return !arg.optional;
                    }).length : 0;
                })
                .reduce(function(m, n) { return Math.min(m, n); });
            }
            assert_equals(self[this.name].length, expected_length, "wrong value for " + this.name + ".length");
        }.bind(this), this.name + " interface object length");
    }

    

    test(function()
    {
        
        

        if (this.is_callback() && !this.has_constants()) {
            return;
        }

        assert_own_property(self, this.name,
                            "self does not have own property " + format_value(this.name));

        if (this.is_callback()) {
            assert_false("prototype" in self[this.name],
                         this.name + ' should not have a "prototype" property');
            return;
        }

        
        
        
        
        
        
        
        assert_own_property(self[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');
        var desc = Object.getOwnPropertyDescriptor(self[this.name], "prototype");
        assert_false("get" in desc, this.name + ".prototype has getter");
        assert_false("set" in desc, this.name + ".prototype has setter");
        assert_false(desc.writable, this.name + ".prototype is writable");
        assert_false(desc.enumerable, this.name + ".prototype is enumerable");
        assert_false(desc.configurable, this.name + ".prototype is configurable");

        
        
        
        
        
        

        
        
        
        
        
        
        
        
        
        
        
        
        
        if (this.name === "Window") {
            assert_class_string(Object.getPrototypeOf(self[this.name].prototype),
                                'WindowProperties',
                                'Class name for prototype of Window' +
                                '.prototype is not "WindowProperties"');
        } else {
            var inherit_interface, inherit_interface_has_interface_object;
            if (this.base) {
                inherit_interface = this.base;
                inherit_interface_has_interface_object =
                    !this.array
                         .members[inherit_interface]
                         .has_extended_attribute("NoInterfaceObject");
            } else if (this.has_extended_attribute('ArrayClass')) {
                inherit_interface = 'Array';
                inherit_interface_has_interface_object = true;
            } else {
                inherit_interface = 'Object';
                inherit_interface_has_interface_object = true;
            }
            if (inherit_interface_has_interface_object) {
                assert_own_property(self, inherit_interface,
                                    'should inherit from ' + inherit_interface + ', but self has no such property');
                assert_own_property(self[inherit_interface], 'prototype',
                                    'should inherit from ' + inherit_interface + ', but that object has no "prototype" property');
                assert_equals(Object.getPrototypeOf(self[this.name].prototype),
                              self[inherit_interface].prototype,
                              'prototype of ' + this.name + '.prototype is not ' + inherit_interface + '.prototype');
            } else {
                
                
                
                assert_class_string(Object.getPrototypeOf(self[this.name].prototype),
                                    inherit_interface + 'Prototype',
                                    'Class name for prototype of ' + this.name +
                                    '.prototype is not "' + inherit_interface + 'Prototype"');
            }
        }

        
        
        
        assert_class_string(self[this.name].prototype, this.name + "Prototype",
                            "class string of " + this.name + ".prototype");
        
        
        if (!this.has_stringifier()) {
            assert_equals(String(self[this.name].prototype), "[object " + this.name + "Prototype]",
                    "String(" + this.name + ".prototype)");
        }
    }.bind(this), this.name + " interface: existence and properties of interface prototype object");

    test(function()
    {
        if (this.is_callback() && !this.has_constants()) {
            return;
        }

        assert_own_property(self, this.name,
                            "self does not have own property " + format_value(this.name));

        if (this.is_callback()) {
            assert_false("prototype" in self[this.name],
                         this.name + ' should not have a "prototype" property');
            return;
        }

        assert_own_property(self[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');

        
        
        
        
        
        assert_own_property(self[this.name].prototype, "constructor",
                            this.name + '.prototype does not have own property "constructor"');
        var desc = Object.getOwnPropertyDescriptor(self[this.name].prototype, "constructor");
        assert_false("get" in desc, this.name + ".prototype.constructor has getter");
        assert_false("set" in desc, this.name + ".prototype.constructor has setter");
        assert_true(desc.writable, this.name + ".prototype.constructor is not writable");
        assert_false(desc.enumerable, this.name + ".prototype.constructor is enumerable");
        assert_true(desc.configurable, this.name + ".prototype.constructor in not configurable");
        assert_equals(self[this.name].prototype.constructor, self[this.name],
                      this.name + '.prototype.constructor is not the same object as ' + this.name);
    }.bind(this), this.name + ' interface: existence and properties of interface prototype object\'s "constructor" property');
};


IdlInterface.prototype.test_member_const = function(member)

{
    test(function()
    {
        if (this.is_callback() && !this.has_constants()) {
            return;
        }

        assert_own_property(self, this.name,
                            "self does not have own property " + format_value(this.name));

        
        
        
        assert_own_property(self[this.name], member.name);
        
        
        
        assert_equals(self[this.name][member.name], constValue(member.value),
                      "property has wrong value");
        
        
        var desc = Object.getOwnPropertyDescriptor(self[this.name], member.name);
        assert_false("get" in desc, "property has getter");
        assert_false("set" in desc, "property has setter");
        assert_false(desc.writable, "property is writable");
        assert_true(desc.enumerable, "property is not enumerable");
        assert_false(desc.configurable, "property is configurable");
    }.bind(this), this.name + " interface: constant " + member.name + " on interface object");
    
    
    test(function()
    {
        if (this.is_callback() && !this.has_constants()) {
            return;
        }

        assert_own_property(self, this.name,
                            "self does not have own property " + format_value(this.name));

        if (this.is_callback()) {
            assert_false("prototype" in self[this.name],
                         this.name + ' should not have a "prototype" property');
            return;
        }

        assert_own_property(self[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');

        assert_own_property(self[this.name].prototype, member.name);
        assert_equals(self[this.name].prototype[member.name], constValue(member.value),
                      "property has wrong value");
        var desc = Object.getOwnPropertyDescriptor(self[this.name], member.name);
        assert_false("get" in desc, "property has getter");
        assert_false("set" in desc, "property has setter");
        assert_false(desc.writable, "property is writable");
        assert_true(desc.enumerable, "property is not enumerable");
        assert_false(desc.configurable, "property is configurable");
    }.bind(this), this.name + " interface: constant " + member.name + " on interface prototype object");
};



IdlInterface.prototype.test_member_attribute = function(member)

{
    test(function()
    {
        if (this.is_callback() && !this.has_constants()) {
            return;
        }

        assert_own_property(self, this.name,
                            "self does not have own property " + format_value(this.name));
        assert_own_property(self[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');

        if (member["static"]) {
            assert_own_property(self[this.name], member.name,
                "The interface object must have a property " +
                format_value(member.name));
        } else if (this.is_global()) {
            assert_own_property(self, member.name,
                "The global object must have a property " +
                format_value(member.name));
            assert_false(member.name in self[this.name].prototype,
                "The prototype object must not have a property " +
                format_value(member.name));

            
            
            
            var gotValue;
            var propVal;
            try {
                propVal = self[member.name];
                gotValue = true;
            } catch (e) {
                gotValue = false;
            }
            if (gotValue) {
                var getter = Object.getOwnPropertyDescriptor(self, member.name).get;
                assert_equals(typeof(getter), "function",
                              format_value(member.name) + " must have a getter");
                assert_equals(propVal, getter.call(undefined),
                              "Gets on a global should not require an explicit this");
            }
            this.do_interface_attribute_asserts(self, member);
        } else {
            assert_true(member.name in self[this.name].prototype,
                "The prototype object must have a property " +
                format_value(member.name));

            if (!member.has_extended_attribute("LenientThis")) {
                assert_throws(new TypeError(), function() {
                    self[this.name].prototype[member.name];
                }.bind(this), "getting property on prototype object must throw TypeError");
            } else {
                assert_equals(self[this.name].prototype[member.name], undefined,
                              "getting property on prototype object must return undefined");
            }
            this.do_interface_attribute_asserts(self[this.name].prototype, member);
        }
    }.bind(this), this.name + " interface: attribute " + member.name);
};


IdlInterface.prototype.test_member_operation = function(member)

{
    test(function()
    {
        if (this.is_callback() && !this.has_constants()) {
            return;
        }

        assert_own_property(self, this.name,
                            "self does not have own property " + format_value(this.name));

        if (this.is_callback()) {
            assert_false("prototype" in self[this.name],
                         this.name + ' should not have a "prototype" property');
            return;
        }

        assert_own_property(self[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');

        
        
        
        
        
        
        
        
        var memberHolderObject;
        if (member["static"]) {
            assert_own_property(self[this.name], member.name,
                    "interface object missing static operation");
            memberHolderObject = self[this.name];
        } else if (this.is_global()) {
            assert_own_property(self, member.name,
                    "global object missing non-static operation");
            memberHolderObject = self;
        } else {
            assert_own_property(self[this.name].prototype, member.name,
                    "interface prototype object missing non-static operation");
            memberHolderObject = self[this.name].prototype;
        }

        this.do_member_operation_asserts(memberHolderObject, member);
    }.bind(this), this.name + " interface: operation " + member.name +
    "(" + member.arguments.map(function(m) { return m.idlType.idlType; }) +
    ")");
};


IdlInterface.prototype.do_member_operation_asserts = function(memberHolderObject, member)

{
    var operationUnforgeable = member.isUnforgeable;
    var desc = Object.getOwnPropertyDescriptor(memberHolderObject, member.name);
    
    
    
    assert_false("get" in desc, "property has getter");
    assert_false("set" in desc, "property has setter");
    assert_equals(desc.writable, !operationUnforgeable,
                  "property should be writable if and only if not unforgeable");
    assert_true(desc.enumerable, "property is not enumerable");
    assert_equals(desc.configurable, !operationUnforgeable,
                  "property should be configurable if and only if not unforgeable");
    
    
    assert_equals(typeof memberHolderObject[member.name], "function",
                  "property must be a function");
    
    
    
    
    
    
    
    assert_equals(memberHolderObject[member.name].length,
        member.arguments.filter(function(arg) {
            return !arg.optional;
        }).length,
        "property has wrong .length");

    
    var args = member.arguments.map(function(arg) {
        return create_suitable_object(arg.idlType);
    });

    
    
    
    
    
    
    
    
    
    
    if (!member["static"]) {
        if (!this.is_global() &&
            memberHolderObject[member.name] != self[member.name])
        {
            assert_throws(new TypeError(), function() {
                memberHolderObject[member.name].apply(null, args);
            }, "calling operation with this = null didn't throw TypeError");
        }

        
        
        
        
        
        assert_throws(new TypeError(), function() {
            memberHolderObject[member.name].apply({}, args);
        }, "calling operation with this = {} didn't throw TypeError");
    }
}


IdlInterface.prototype.test_member_stringifier = function(member)

{
    test(function()
    {
        if (this.is_callback() && !this.has_constants()) {
            return;
        }

        assert_own_property(self, this.name,
                            "self does not have own property " + format_value(this.name));

        if (this.is_callback()) {
            assert_false("prototype" in self[this.name],
                         this.name + ' should not have a "prototype" property');
            return;
        }

        assert_own_property(self[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');

        
        var interfacePrototypeObject = self[this.name].prototype;
        assert_own_property(self[this.name].prototype, "toString",
                "interface prototype object missing non-static operation");

        var stringifierUnforgeable = member.isUnforgeable;
        var desc = Object.getOwnPropertyDescriptor(interfacePrototypeObject, "toString");
        
        
        
        assert_false("get" in desc, "property has getter");
        assert_false("set" in desc, "property has setter");
        assert_equals(desc.writable, !stringifierUnforgeable,
                      "property should be writable if and only if not unforgeable");
        assert_true(desc.enumerable, "property is not enumerable");
        assert_equals(desc.configurable, !stringifierUnforgeable,
                      "property should be configurable if and only if not unforgeable");
        
        
        assert_equals(typeof interfacePrototypeObject.toString, "function",
                      "property must be a function");
        
        
        assert_equals(interfacePrototypeObject.toString.length, 0,
            "property has wrong .length");

        
        assert_throws(new TypeError(), function() {
            self[this.name].prototype.toString.apply(null, []);
        }, "calling stringifier with this = null didn't throw TypeError");

        
        
        
        
        
        assert_throws(new TypeError(), function() {
            self[this.name].prototype.toString.apply({}, []);
        }, "calling stringifier with this = {} didn't throw TypeError");
    }.bind(this), this.name + " interface: stringifier");
};


IdlInterface.prototype.test_members = function()

{
    for (var i = 0; i < this.members.length; i++)
    {
        var member = this.members[i];
        if (member.untested) {
            continue;
        }

        switch (member.type) {
        case "const":
            this.test_member_const(member);
            break;

        case "attribute":
            
            
            if (!member.isUnforgeable)
            {
                this.test_member_attribute(member);
            }
            break;

        case "operation":
            
            
            
            
            if (member.name) {
                if (!member.isUnforgeable)
                {
                    this.test_member_operation(member);
                }
            } else if (member.stringifier) {
                this.test_member_stringifier(member);
            }
            break;

        default:
            
            break;
        }
    }
};


IdlInterface.prototype.test_object = function(desc)

{
    var obj, exception = null;
    try
    {
        obj = eval(desc);
    }
    catch(e)
    {
        exception = e;
    }

    
    
    var expected_typeof = this.members.some(function(member)
    {
        return member.legacycaller
            || ("idlType" in member && member.idlType.legacycaller)
            || ("idlType" in member && typeof member.idlType == "object"
            && "idlType" in member.idlType && member.idlType.idlType == "legacycaller");
    }) ? "function" : "object";

    this.test_primary_interface_of(desc, obj, exception, expected_typeof);
    var current_interface = this;
    while (current_interface)
    {
        if (!(current_interface.name in this.array.members))
        {
            throw "Interface " + current_interface.name + " not found (inherited by " + this.name + ")";
        }
        if (current_interface.prevent_multiple_testing && current_interface.already_tested)
        {
            return;
        }
        current_interface.test_interface_of(desc, obj, exception, expected_typeof);
        current_interface = this.array.members[current_interface.base];
    }
};


IdlInterface.prototype.test_primary_interface_of = function(desc, obj, exception, expected_typeof)

{
    
    
    
    
    if (!this.has_extended_attribute("NoInterfaceObject")
    && (typeof obj != expected_typeof || obj instanceof Object))
    {
        test(function()
        {
            assert_equals(exception, null, "Unexpected exception when evaluating object");
            assert_equals(typeof obj, expected_typeof, "wrong typeof object");
            assert_own_property(self, this.name,
                                "self does not have own property " + format_value(this.name));
            assert_own_property(self[this.name], "prototype",
                                'interface "' + this.name + '" does not have own property "prototype"');

            
            
            
            
            assert_equals(Object.getPrototypeOf(obj),
                          self[this.name].prototype,
                          desc + "'s prototype is not " + this.name + ".prototype");
        }.bind(this), this.name + " must be primary interface of " + desc);
    }

    
    
    
    test(function()
    {
        assert_equals(exception, null, "Unexpected exception when evaluating object");
        assert_equals(typeof obj, expected_typeof, "wrong typeof object");
        assert_class_string(obj, this.name, "class string of " + desc);
        if (!this.has_stringifier())
        {
            assert_equals(String(obj), "[object " + this.name + "]", "String(" + desc + ")");
        }
    }.bind(this), "Stringification of " + desc);
};


IdlInterface.prototype.test_interface_of = function(desc, obj, exception, expected_typeof)

{
    
    this.already_tested = true;

    for (var i = 0; i < this.members.length; i++)
    {
        var member = this.members[i];
        if (member.type == "attribute" && member.isUnforgeable)
        {
            test(function()
            {
                assert_equals(exception, null, "Unexpected exception when evaluating object");
                assert_equals(typeof obj, expected_typeof, "wrong typeof object");
                this.do_interface_attribute_asserts(obj, member);
            }.bind(this), this.name + " interface: " + desc + ' must have own property "' + member.name + '"');
        }
        else if (member.type == "operation" &&
                 member.name &&
                 member.isUnforgeable)
        {
            test(function()
            {
                assert_equals(exception, null, "Unexpected exception when evaluating object");
                assert_equals(typeof obj, expected_typeof, "wrong typeof object");
                assert_own_property(obj, member.name,
                                    "Doesn't have the unforgeable operation property");
                this.do_member_operation_asserts(obj, member);
            }.bind(this), this.name + " interface: " + desc + ' must have own property "' + member.name + '"');
        }
        else if ((member.type == "const"
        || member.type == "attribute"
        || member.type == "operation")
        && member.name)
        {
            test(function()
            {
                assert_equals(exception, null, "Unexpected exception when evaluating object");
                assert_equals(typeof obj, expected_typeof, "wrong typeof object");
                if (!member["static"]) {
                    if (!this.is_global()) {
                        assert_inherits(obj, member.name);
                    } else {
                        assert_own_property(obj, member.name);
                    }

                    if (member.type == "const")
                    {
                        assert_equals(obj[member.name], constValue(member.value));
                    }
                    if (member.type == "attribute")
                    {
                        
                        
                        
                        var property, thrown = false;
                        try
                        {
                            property = obj[member.name];
                        }
                        catch (e)
                        {
                            thrown = true;
                        }
                        if (!thrown)
                        {
                            this.array.assert_type_is(property, member.idlType);
                        }
                    }
                    if (member.type == "operation")
                    {
                        assert_equals(typeof obj[member.name], "function");
                    }
                }
            }.bind(this), this.name + " interface: " + desc + ' must inherit property "' + member.name + '" with the proper type (' + i + ')');
        }
        
        
        
        if (member.type == "operation" && member.name && member.arguments.length)
        {
            test(function()
            {
                assert_equals(exception, null, "Unexpected exception when evaluating object");
                assert_equals(typeof obj, expected_typeof, "wrong typeof object");
                if (!member["static"]) {
                    if (!this.is_global() && !member.isUnforgeable) {
                        assert_inherits(obj, member.name);
                    } else {
                        assert_own_property(obj, member.name);
                    }
                }
                else
                {
                    assert_false(member.name in obj);
                }
                var args = [];
                for (var i = 0; i < member.arguments.length; i++)
                {
                    if (member.arguments[i].optional)
                    {
                        break;
                    }
                    assert_throws(new TypeError(), function()
                    {
                        obj[member.name].apply(obj, args);
                    }.bind(this), "Called with " + i + " arguments");

                    args.push(create_suitable_object(member.arguments[i].idlType));
                }
            }.bind(this), this.name + " interface: calling " + member.name +
            "(" + member.arguments.map(function(m) { return m.idlType.idlType; }) +
            ") on " + desc + " with too few arguments must throw TypeError");
        }
    }
};


IdlInterface.prototype.has_stringifier = function()

{
    if (this.members.some(function(member) { return member.stringifier; })) {
        return true;
    }
    if (this.base &&
        this.array.members[this.base].has_stringifier()) {
        return true;
    }
    return false;
};


IdlInterface.prototype.do_interface_attribute_asserts = function(obj, member)

{
    
    

    
    
    

    
    
    
    

    
    assert_own_property(obj, member.name);

    
    
    
    
    
    
    var desc = Object.getOwnPropertyDescriptor(obj, member.name);
    assert_false("value" in desc, 'property descriptor has value but is supposed to be accessor');
    assert_false("writable" in desc, 'property descriptor has "writable" field but is supposed to be accessor');
    assert_true(desc.enumerable, "property is not enumerable");
    if (member.isUnforgeable)
    {
        assert_false(desc.configurable, "[Unforgeable] property must not be configurable");
    }
    else
    {
        assert_true(desc.configurable, "property must be configurable");
    }


    
    
    assert_equals(typeof desc.get, "function", "getter must be Function");

    
    if (!member["static"]) {
        
        
        
        
        if (!member.has_extended_attribute("LenientThis")) {
            assert_throws(new TypeError(), function() {
                desc.get.call({});
            }.bind(this), "calling getter on wrong object type must throw TypeError");
        } else {
            assert_equals(desc.get.call({}), undefined,
                          "calling getter on wrong object type must return undefined");
        }
    }

    
    
    assert_equals(desc.get.length, 0, "getter length must be 0");


    
    
    if (member.readonly
    && !member.has_extended_attribute("PutForwards")
    && !member.has_extended_attribute("Replaceable"))
    {
        
        
        
        assert_equals(desc.set, undefined, "setter must be undefined for readonly attributes");
    }
    else
    {
        
        
        assert_equals(typeof desc.set, "function", "setter must be function for PutForwards, Replaceable, or non-readonly attributes");

        
        if (!member["static"]) {
            
            
            
            
            
            
            if (!member.has_extended_attribute("LenientThis")) {
                assert_throws(new TypeError(), function() {
                    desc.set.call({});
                }.bind(this), "calling setter on wrong object type must throw TypeError");
            } else {
                assert_equals(desc.set.call({}), undefined,
                              "calling setter on wrong object type must return undefined");
            }
        }

        
        
        assert_equals(desc.set.length, 1, "setter length must be 1");
    }
}



function IdlInterfaceMember(obj)

{
    




    for (var k in obj)
    {
        this[k] = obj[k];
    }
    if (!("extAttrs" in this))
    {
        this.extAttrs = [];
    }

    this.isUnforgeable = this.has_extended_attribute("Unforgeable");
}


IdlInterfaceMember.prototype = Object.create(IdlObject.prototype);


function create_suitable_object(type)

{
    




    if (type.nullable)
    {
        return null;
    }
    switch (type.idlType)
    {
        case "any":
        case "boolean":
            return true;

        case "byte": case "octet": case "short": case "unsigned short":
        case "long": case "unsigned long": case "long long":
        case "unsigned long long": case "float": case "double":
        case "unrestricted float": case "unrestricted double":
            return 7;

        case "DOMString":
        case "ByteString":
        case "USVString":
            return "foo";

        case "object":
            return {a: "b"};

        case "Node":
            return document.createTextNode("abc");
    }
    return null;
}




function IdlEnum(obj)

{
    




    
    this.name = obj.name;

    
    this.values = obj.values;

}


IdlEnum.prototype = Object.create(IdlObject.prototype);



function IdlTypedef(obj)

{
    




    
    this.name = obj.name;

    
    this.values = obj.values;

}


IdlTypedef.prototype = Object.create(IdlObject.prototype);

}());

