






































































































































"use strict";
(function(){
var interfaces = {};



window.IdlArray = function()

{
    this.members = {};
    this.objects = {};
    
    
    
    
    this.partials = [];
    this.implements = {};
}


IdlArray.prototype.add_idls = function(raw_idls)

{
    this.internal_add_idls(WebIDLParser.parse(raw_idls));
};


IdlArray.prototype.add_untested_idls = function(raw_idls)

{
    var parsed_idls = WebIDLParser.parse(raw_idls);
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
}


IdlArray.prototype.internal_add_idls = function(parsed_idls)

{
    parsed_idls.forEach(function(parsed_idl)
    {
        if (parsed_idl.type == "partialinterface")
        {
            this.partials.push(parsed_idl);
            return;
        }

        if (parsed_idl.type == "implements")
        {
            if (!(parsed_idl.target in this.implements))
            {
                this.implements[parsed_idl.target] = [];
            }
            this.implements[parsed_idl.target].push(parsed_idl.implements);
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
            this.members[parsed_idl.name] = new IdlInterface(parsed_idl);
            break;

        case "exception":
            this.members[parsed_idl.name] = new IdlException(parsed_idl);
            break;

        case "dictionary":
            
            
            this.members[parsed_idl.name] = new IdlDictionary(parsed_idl);
            break;

        case "typedef":
            
            break;

        case "enum":
            
            break;

        default:
            throw parsed_idl.name + ": " + parsed_idl.type + " not yet supported";
        }
    }.bind(this));
}


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
}


IdlArray.prototype.prevent_multiple_testing = function(name)

{
    this.members[name].prevent_multiple_testing = true;
}


IdlArray.prototype.recursively_get_implements = function(interface_name)

{
    var ret = this.implements[interface_name];
    if (ret === undefined)
    {
        return [];
    }
    for (var i = 0; i < this.implements[interface_name].length; i++)
    {
        ret = ret.concat(this.recursively_get_implements(ret[i]));
        if (ret.indexOf(ret[i]) != ret.lastIndexOf(ret[i]))
        {
            throw "Circular implements statements involving " + ret[i];
        }
    }
    return ret;
}


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

    for (var lhs in this.implements)
    {
        this.recursively_get_implements(lhs).forEach(function(rhs)
        {
            if (!(lhs in this.members)
            || !(this.members[lhs] instanceof IdlInterface)
            || !(rhs in this.members)
            || !(this.members[rhs] instanceof IdlInterface))
            {
                throw lhs + " implements " + rhs + ", but one is undefined or not an interface";
            }
            this.members[rhs].members.forEach(function(member)
            {
                this.members[lhs].members.push(new IdlInterfaceMember(member));
            }.bind(this));
        }.bind(this));
    }
    this.implements = {};

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
            assert_equals(typeof value, "number");
            assert_true(0 <= value, "unsigned long long is negative");
            return;

        case "float":
        case "double":
            
            assert_equals(typeof value, "number");
            return;

        case "DOMString":
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
        && type in window)
        {
            assert_true(value instanceof window[type], "not instanceof " + type);
        }
    }
    else if (this.members[type] instanceof IdlDictionary)
    {
        
    }
    else
    {
        throw "Type " + type + " isn't an interface or dictionary";
    }
};



function IdlObject() {}
IdlObject.prototype.has_extended_attribute = function(name)

{
    return this.extAttrs.some(function(o)
    {
        return o.name == name;
    });
};


IdlObject.prototype.test = function() {};



function IdlDictionary(obj)

{
    this.name = obj.name;
    this.members = obj.members ? obj.members : [];
    this.inheritance = obj.inheritance ? obj.inheritance: [];
}


IdlDictionary.prototype = Object.create(IdlObject.prototype);


function IdlException(obj)

{
    this.name = obj.name;
    this.array = obj.array;
    this.untested = obj.untested;
    this.extAttrs = obj.extAttrs ? obj.extAttrs : [];
    this.members = obj.members ? obj.members.map(function(m){return new IdlInterfaceMember(m)}) : [];
    this.inheritance = obj.inheritance ? obj.inheritance : [];
}


IdlException.prototype = Object.create(IdlObject.prototype);
IdlException.prototype.test = function()

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
}


IdlException.prototype.test_self = function()

{
    test(function()
    {
        
        
        
        
        
        
        
        
        assert_own_property(window, this.name,
                            "window does not have own property " + format_value(this.name));
        var desc = Object.getOwnPropertyDescriptor(window, this.name);
        assert_false("get" in desc, "window's property " + format_value(this.name) + " has getter");
        assert_false("set" in desc, "window's property " + format_value(this.name) + " has setter");
        assert_true(desc.writable, "window's property " + format_value(this.name) + " is not writable");
        assert_false(desc.enumerable, "window's property " + format_value(this.name) + " is enumerable");
        assert_true(desc.configurable, "window's property " + format_value(this.name) + " is not configurable");

        
        
        
        
        
        
        
        
        assert_equals(Object.getPrototypeOf(window[this.name]), Function.prototype,
                      "prototype of window's property " + format_value(this.name) + " is not Function.prototype");
        
        
        
        
        
        
        
        
        
        
        
        
        assert_class_string(window[this.name], "Function",
                            "class string of " + this.name);

        
        
    }.bind(this), this.name + " exception: existence and properties of exception interface object");

    test(function()
    {
        assert_own_property(window, this.name,
                            "window does not have own property " + format_value(this.name));

        
        
        
        
        
        assert_own_property(window[this.name], "prototype",
                            'exception "' + this.name + '" does not have own property "prototype"');
        var desc = Object.getOwnPropertyDescriptor(window[this.name], "prototype");
        assert_false("get" in desc, this.name + ".prototype has getter");
        assert_false("set" in desc, this.name + ".prototype has setter");
        assert_false(desc.writable, this.name + ".prototype is writable");
        assert_false(desc.enumerable, this.name + ".prototype is enumerable");
        assert_false(desc.configurable, this.name + ".prototype is configurable");

        
        
        
        
        
        
        
        
        
        
        
        var inherit_exception = this.inheritance.length ? this.inheritance[0] : "Error";
        assert_own_property(window, inherit_exception,
                            'should inherit from ' + inherit_exception + ', but window has no such property');
        assert_own_property(window[inherit_exception], "prototype",
                            'should inherit from ' + inherit_exception + ', but that object has no "prototype" property');
        assert_equals(Object.getPrototypeOf(window[this.name].prototype),
                      window[inherit_exception].prototype,
                      'prototype of ' + this.name + '.prototype is not ' + inherit_exception + '.prototype');

        
        
        
        
        
        assert_class_string(window[this.name].prototype, this.name + "Prototype",
                            "class string of " + this.name + ".prototype");
        assert_equals(String(window[this.name].prototype), "[object " + this.name + "Prototype]",
                      "String(" + this.name + ".prototype)");
    }.bind(this), this.name + " exception: existence and properties of exception interface prototype object");

    test(function()
    {
        assert_own_property(window, this.name,
                            "window does not have own property " + format_value(this.name));
        assert_own_property(window[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');

        
        
        
        
        assert_own_property(window[this.name].prototype, "name",
                'prototype object does not have own property "name"');
        var desc = Object.getOwnPropertyDescriptor(window[this.name].prototype, "name");
        assert_false("get" in desc, this.name + ".prototype.name has getter");
        assert_false("set" in desc, this.name + ".prototype.name has setter");
        assert_true(desc.writable, this.name + ".prototype.name is not writable");
        assert_false(desc.enumerable, this.name + ".prototype.name is enumerable");
        assert_true(desc.configurable, this.name + ".prototype.name is not configurable");
        assert_equals(desc.value, this.name, this.name + ".prototype.name has incorrect value");
    }.bind(this), this.name + " exception: existence and properties of exception interface prototype object's \"name\" property");

    test(function()
    {
        assert_own_property(window, this.name,
                            "window does not have own property " + format_value(this.name));
        assert_own_property(window[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');

        
        
        
        
        
        
        assert_own_property(window[this.name].prototype, "constructor",
                            this.name + '.prototype does not have own property "constructor"');
        var desc = Object.getOwnPropertyDescriptor(window[this.name].prototype, "constructor");
        assert_false("get" in desc, this.name + ".prototype.constructor has getter");
        assert_false("set" in desc, this.name + ".prototype.constructor has setter");
        assert_true(desc.writable, this.name + ".prototype.constructor is not writable");
        assert_false(desc.enumerable, this.name + ".prototype.constructor is enumerable");
        assert_true(desc.configurable, this.name + ".prototype.constructor in not configurable");
        assert_equals(window[this.name].prototype.constructor, window[this.name],
                      this.name + '.prototype.constructor is not the same object as ' + this.name);
    }.bind(this), this.name + " exception: existence and properties of exception interface prototype object's \"constructor\" property");
}


IdlException.prototype.test_members = function()

{
    for (var i = 0; i < this.members.length; i++)
    {
        var member = this.members[i];
        if (member.untested)
        {
            continue;
        }
        if (member.type == "const" && member.name != "prototype")
        {
            test(function()
            {
                assert_own_property(window, this.name,
                                    "window does not have own property " + format_value(this.name));

                
                
                
                
                assert_own_property(window[this.name], member.name);
                
                
                
                assert_equals(window[this.name][member.name], eval(member.value),
                              "property has wrong value");
                
                
                var desc = Object.getOwnPropertyDescriptor(window[this.name], member.name);
                assert_false("get" in desc, "property has getter");
                assert_false("set" in desc, "property has setter");
                assert_false(desc.writable, "property is writable");
                assert_true(desc.enumerable, "property is not enumerable");
                assert_false(desc.configurable, "property is configurable");
            }.bind(this), this.name + " exception: constant " + member.name + " on exception interface object");
            
            
            test(function()
            {
                assert_own_property(window, this.name,
                                    "window does not have own property " + format_value(this.name));
                assert_own_property(window[this.name], "prototype",
                                    'exception "' + this.name + '" does not have own property "prototype"');

                assert_own_property(window[this.name].prototype, member.name);
                assert_equals(window[this.name].prototype[member.name], eval(member.value),
                              "property has wrong value");
                var desc = Object.getOwnPropertyDescriptor(window[this.name].prototype, member.name);
                assert_false("get" in desc, "property has getter");
                assert_false("set" in desc, "property has setter");
                assert_false(desc.writable, "property is writable");
                assert_true(desc.enumerable, "property is not enumerable");
                assert_false(desc.configurable, "property is configurable");
            }.bind(this), this.name + " exception: constant " + member.name + " on exception interface prototype object");
        }
        else if (member.type == "field")
        {
            test(function()
            {
                assert_own_property(window, this.name,
                                    "window does not have own property " + format_value(this.name));
                assert_own_property(window[this.name], "prototype",
                                    'exception "' + this.name + '" does not have own property "prototype"');

                
                
                
                
                
                assert_own_property(window[this.name].prototype, member.name);
                
                
                
                var desc = Object.getOwnPropertyDescriptor(window[this.name].prototype, member.name);
                assert_false("value" in desc, "property descriptor has value but is supposed to be accessor");
                assert_false("writable" in desc, 'property descriptor has "writable" field but is supposed to be accessor');
                
                
                assert_true(desc.enumerable, "property is not enumerable");
                assert_true(desc.configurable, "property is not configurable");
                
                
                assert_equals(typeof desc.get, "function", "typeof getter");
                
                
                
                
                assert_equals(desc.get.length, 0, "getter length");
                
                
                
                
                
                assert_throws(new TypeError(), function()
                {
                    window[this.name].prototype[member.name];
                }.bind(this), "getting property on prototype object must throw TypeError");
                assert_throws(new TypeError(), function()
                {
                    desc.get.call({});
                }.bind(this), "calling getter on wrong object type must throw TypeError");
            }.bind(this), this.name + " exception: field " + member.name + " on exception interface prototype object");
        }
    }
}


IdlException.prototype.test_object = function(desc)

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

    test(function()
    {
        assert_equals(exception, null, "Unexpected exception when evaluating object");
        assert_equals(typeof obj, "object", "wrong typeof object");

        
        
        
        
        if (!this.has_extended_attribute("NoInterfaceObject")
        && (typeof obj != "object" || obj instanceof Object))
        {
            assert_own_property(window, this.name,
                                "window does not have own property " + format_value(this.name));
            assert_own_property(window[this.name], "prototype",
                                'exception "' + this.name + '" does not have own property "prototype"');

            
            
            
            
            assert_equals(Object.getPrototypeOf(obj),
                          window[this.name].prototype,
                          desc + "'s prototype is not " + this.name + ".prototype");
        }

        
        
        assert_class_string(obj, this.name, "class string of " + desc);
        
        
    }.bind(this), this.name + " must be represented by " + desc);

    for (var i = 0; i < this.members.length; i++)
    {
        var member = this.members[i];
        test(function()
        {
            assert_equals(exception, null, "Unexpected exception when evaluating object");
            assert_equals(typeof obj, "object", "wrong typeof object");
            assert_inherits(obj, member.name);
            if (member.type == "const")
            {
                assert_equals(obj[member.name], eval(member.value));
            }
            if (member.type == "field")
            {
                this.array.assert_type_is(obj[member.name], member.idlType);
            }
        }.bind(this), this.name + " exception: " + desc + ' must inherit property "' + member.name + '" with the proper type');
    }
}



function IdlInterface(obj)

{
    this.name = obj.name;
    this.array = obj.array;
    this.untested = obj.untested;
    this.extAttrs = obj.extAttrs ? obj.extAttrs : [];
    this.members = obj.members ? obj.members.map(function(m){return new IdlInterfaceMember(m)}) : [];
    this.inheritance = obj.inheritance ? obj.inheritance : [];
    interfaces[this.name] = this;
}


IdlInterface.prototype = Object.create(IdlObject.prototype);
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
}


IdlInterface.prototype.test_self = function()

{
    test(function()
    {
        
        
        
        
        
        
        
        
        
        assert_own_property(window, this.name,
                            "window does not have own property " + format_value(this.name));
        var desc = Object.getOwnPropertyDescriptor(window, this.name);
        assert_false("get" in desc, "window's property " + format_value(this.name) + " has getter");
        assert_false("set" in desc, "window's property " + format_value(this.name) + " has setter");
        assert_true(desc.writable, "window's property " + format_value(this.name) + " is not writable");
        assert_false(desc.enumerable, "window's property " + format_value(this.name) + " is enumerable");
        assert_true(desc.configurable, "window's property " + format_value(this.name) + " is not configurable");

        
        
        
        
        
        
        
        assert_equals(Object.getPrototypeOf(window[this.name]), Function.prototype,
                      "prototype of window's property " + format_value(this.name) + " is not Function.prototype");
        
        
        
        
        
        
        
        
        
        
        
        
        
        assert_class_string(window[this.name], "Function", "class string of " + this.name);

        if (!this.has_extended_attribute("Constructor"))
        {
            
            
            
            
            
            assert_throws(new TypeError(), function()
            {
                window[this.name]();
            }.bind(this), "interface object didn't throw TypeError when called as a function");
            assert_throws(new TypeError(), function()
            {
                new window[this.name]();
            }.bind(this), "interface object didn't throw TypeError when called as a constructor");
        }
    }.bind(this), this.name + " interface: existence and properties of interface object");

    if (this.has_extended_attribute("Constructor"))
    {
        test(function()
        {
            assert_own_property(window, this.name,
                                "window does not have own property " + format_value(this.name));

            
            
            
            
            
            
            
            
            
            
            var expected_length = this.extAttrs
                .filter(function(attr) { return attr.name == "Constructor" })
                .map(function(attr) {
                    return attr.arguments ? attr.arguments.filter(
                        function(arg) {
                            return !arg.optional;
                        }).length : 0
                })
                .reduce(function(m, n) { return Math.min(m, n) });
            assert_own_property(window[this.name], "length");
            assert_equals(window[this.name].length, expected_length, "wrong value for " + this.name + ".length");
            var desc = Object.getOwnPropertyDescriptor(window[this.name], "length");
            assert_false("get" in desc, this.name + ".length has getter");
            assert_false("set" in desc, this.name + ".length has setter");
            assert_false(desc.writable, this.name + ".length is writable");
            assert_false(desc.enumerable, this.name + ".length is enumerable");
            assert_false(desc.configurable, this.name + ".length is configurable");
        }.bind(this), this.name + " interface constructor");
    }

    

    test(function()
    {
        assert_own_property(window, this.name,
                            "window does not have own property " + format_value(this.name));

        
        
        
        
        
        
        assert_own_property(window[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');
        var desc = Object.getOwnPropertyDescriptor(window[this.name], "prototype");
        assert_false("get" in desc, this.name + ".prototype has getter");
        assert_false("set" in desc, this.name + ".prototype has setter");
        assert_false(desc.writable, this.name + ".prototype is writable");
        assert_false(desc.enumerable, this.name + ".prototype is enumerable");
        assert_false(desc.configurable, this.name + ".prototype is configurable");

        
        
        
        
        
        
        
        
        
        
        var inherit_interface = (function()
        {
            for (var i = 0; i < this.inheritance.length; ++i)
            {
                if (!interfaces[this.inheritance[i]].has_extended_attribute("NoInterfaceObject"))
                {
                    return this.inheritance[i];
                }
            }
            if (this.has_extended_attribute("ArrayClass"))
            {
                return "Array";
            }
            return "Object";
        }).bind(this)();
        assert_own_property(window, inherit_interface,
                            'should inherit from ' + inherit_interface + ', but window has no such property');
        assert_own_property(window[inherit_interface], "prototype",
                            'should inherit from ' + inherit_interface + ', but that object has no "prototype" property');
        assert_equals(Object.getPrototypeOf(window[this.name].prototype),
                      window[inherit_interface].prototype,
                      'prototype of ' + this.name + '.prototype is not ' + inherit_interface + '.prototype');

        
        
        
        
        
        assert_class_string(window[this.name].prototype, this.name + "Prototype",
                            "class string of " + this.name + ".prototype");
        assert_equals(String(window[this.name].prototype), "[object " + this.name + "Prototype]",
                      "String(" + this.name + ".prototype)");
    }.bind(this), this.name + " interface: existence and properties of interface prototype object");

    test(function()
    {
        assert_own_property(window, this.name,
                            "window does not have own property " + format_value(this.name));
        assert_own_property(window[this.name], "prototype",
                            'interface "' + this.name + '" does not have own property "prototype"');

        
        
        
        
        
        assert_own_property(window[this.name].prototype, "constructor",
                            this.name + '.prototype does not have own property "constructor"');
        var desc = Object.getOwnPropertyDescriptor(window[this.name].prototype, "constructor");
        assert_false("get" in desc, this.name + ".prototype.constructor has getter");
        assert_false("set" in desc, this.name + ".prototype.constructor has setter");
        assert_true(desc.writable, this.name + ".prototype.constructor is not writable");
        assert_false(desc.enumerable, this.name + ".prototype.constructor is enumerable");
        assert_true(desc.configurable, this.name + ".prototype.constructor in not configurable");
        assert_equals(window[this.name].prototype.constructor, window[this.name],
                      this.name + '.prototype.constructor is not the same object as ' + this.name);
    }.bind(this), this.name + ' interface: existence and properties of interface prototype object\'s "constructor" property');
}


IdlInterface.prototype.test_members = function()

{
    for (var i = 0; i < this.members.length; i++)
    {
        var member = this.members[i];
        if (member.untested)
        {
            continue;
        }
        if (member.type == "const")
        {
            test(function()
            {
                assert_own_property(window, this.name,
                                    "window does not have own property " + format_value(this.name));

                
                
                
                assert_own_property(window[this.name], member.name);
                
                
                
                assert_equals(window[this.name][member.name], eval(member.value),
                              "property has wrong value");
                
                
                var desc = Object.getOwnPropertyDescriptor(window[this.name], member.name);
                assert_false("get" in desc, "property has getter");
                assert_false("set" in desc, "property has setter");
                assert_false(desc.writable, "property is writable");
                assert_true(desc.enumerable, "property is not enumerable");
                assert_false(desc.configurable, "property is configurable");
            }.bind(this), this.name + " interface: constant " + member.name + " on interface object");
            
            
            test(function()
            {
                assert_own_property(window, this.name,
                                    "window does not have own property " + format_value(this.name));
                assert_own_property(window[this.name], "prototype",
                                    'interface "' + this.name + '" does not have own property "prototype"');

                assert_own_property(window[this.name].prototype, member.name);
                assert_equals(window[this.name].prototype[member.name], eval(member.value),
                              "property has wrong value");
                var desc = Object.getOwnPropertyDescriptor(window[this.name], member.name);
                assert_false("get" in desc, "property has getter");
                assert_false("set" in desc, "property has setter");
                assert_false(desc.writable, "property is writable");
                assert_true(desc.enumerable, "property is not enumerable");
                assert_false(desc.configurable, "property is configurable");
            }.bind(this), this.name + " interface: constant " + member.name + " on interface prototype object");
        }
        else if (member.type == "attribute")
        {
            if (member.has_extended_attribute("Unforgeable"))
            {
                
                continue;
            }
            test(function()
            {
                assert_own_property(window, this.name,
                                    "window does not have own property " + format_value(this.name));
                assert_own_property(window[this.name], "prototype",
                                    'interface "' + this.name + '" does not have own property "prototype"');

                
                assert_throws(new TypeError(), function() {
                    window[this.name].prototype[member.name];
                }.bind(this), "getting property on prototype object must throw TypeError");

                do_interface_attribute_asserts(window[this.name].prototype, member);
            }.bind(this), this.name + " interface: attribute " + member.name);
        }
        else if (member.type == "operation")
        {
            
            
            if (!member.name)
            {
                
                continue;
            }
            test(function()
            {
                assert_own_property(window, this.name,
                                    "window does not have own property " + format_value(this.name));
                assert_own_property(window[this.name], "prototype",
                                    'interface "' + this.name + '" does not have own property "prototype"');

                
                
                
                
                
                
                
                
                
                assert_own_property(window[this.name].prototype, member.name,
                    "interface prototype object missing non-static operation");

                var desc = Object.getOwnPropertyDescriptor(window[this.name].prototype, member.name);
                
                
                assert_false("get" in desc, "property has getter");
                assert_false("set" in desc, "property has setter");
                assert_true(desc.writable, "property is not writable");
                assert_true(desc.enumerable, "property is not enumerable");
                assert_true(desc.configurable, "property is not configurable");
                
                
                assert_equals(typeof window[this.name].prototype[member.name], "function",
                              "property must be a function");
                
                
                
                
                
                
                assert_equals(window[this.name].prototype[member.name].length,
                    member.arguments.filter(function(arg) {
                        return !arg.optional;
                    }).length,
                    "property has wrong .length");

                
                var args = member.arguments.map(function(arg) {
                    return create_suitable_object(arg.type);
                });

                
                
                
                
                
                
                
                assert_throws(new TypeError(), function() {
                    window[this.name].prototype[member.name].apply(null, args);
                }, "calling operation with this = null didn't throw TypeError");

                
                
                
                
                assert_throws(new TypeError(), function() {
                    window[this.name].prototype[member.name].apply({}, args);
                }, "calling operation with this = {} didn't throw TypeError");
            }.bind(this), this.name + " interface: operation " + member.name +
            "(" + member.arguments.map(function(m) { return m.type.idlType; }) +
            ")");
        }
        
    }
}


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
        current_interface = this.array.members[current_interface.inheritance[0]];
    }
}


IdlInterface.prototype.test_primary_interface_of = function(desc, obj, exception, expected_typeof)

{
    
    
    
    
    if (!this.has_extended_attribute("NoInterfaceObject")
    && (typeof obj != expected_typeof || obj instanceof Object))
    {
        test(function()
        {
            assert_equals(exception, null, "Unexpected exception when evaluating object");
            assert_equals(typeof obj, expected_typeof, "wrong typeof object");
            assert_own_property(window, this.name,
                                "window does not have own property " + format_value(this.name));
            assert_own_property(window[this.name], "prototype",
                                'interface "' + this.name + '" does not have own property "prototype"');

            
            
            
            assert_equals(Object.getPrototypeOf(obj),
                          window[this.name].prototype,
                          desc + "'s prototype is not " + this.name + ".prototype");
        }.bind(this), this.name + " must be primary interface of " + desc);
    }

    
    
    
    test(function()
    {
        assert_equals(exception, null, "Unexpected exception when evaluating object");
        assert_equals(typeof obj, expected_typeof, "wrong typeof object");
        assert_class_string(obj, this.name, "class string of " + desc);
        if (!this.members.some(function(member) { return member.stringifier || member.type == "stringifier"}))
        {
            assert_equals(String(obj), "[object " + this.name + "]", "String(" + desc + ")");
        }
    }.bind(this), "Stringification of " + desc);
}


IdlInterface.prototype.test_interface_of = function(desc, obj, exception, expected_typeof)

{
    
    this.already_tested = true;

    for (var i = 0; i < this.members.length; i++)
    {
        var member = this.members[i];
        if (member.has_extended_attribute("Unforgeable"))
        {
            test(function()
            {
                assert_equals(exception, null, "Unexpected exception when evaluating object");
                assert_equals(typeof obj, expected_typeof, "wrong typeof object");
                do_interface_attribute_asserts(obj, member);
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
                assert_inherits(obj, member.name);
                if (member.type == "const")
                {
                    assert_equals(obj[member.name], eval(member.value));
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
            }.bind(this), this.name + " interface: " + desc + ' must inherit property "' + member.name + '" with the proper type (' + i + ')');
        }
        
        
        
        if (member.type == "operation" && member.name && member.arguments.length)
        {
            test(function()
            {
                assert_equals(exception, null, "Unexpected exception when evaluating object");
                assert_equals(typeof obj, expected_typeof, "wrong typeof object");
                assert_inherits(obj, member.name);
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

                    args.push(create_suitable_object(member.arguments[i].type));
                }
            }.bind(this), this.name + " interface: calling " + member.name +
            "(" + member.arguments.map(function(m) { return m.type.idlType; }) +
            ") on " + desc + " with too few arguments must throw TypeError");
        }
    }
}


function do_interface_attribute_asserts(obj, member)

{
    
    
    
    
    
    
    
    assert_own_property(obj, member.name);

    
    
    
    
    
    
    
    var desc = Object.getOwnPropertyDescriptor(obj, member.name);
    assert_false("value" in desc, 'property descriptor has value but is supposed to be accessor');
    assert_false("writable" in desc, 'property descriptor has "writable" field but is supposed to be accessor');
    assert_true(desc.enumerable, "property is not enumerable");
    if (member.has_extended_attribute("Unforgeable"))
    {
        assert_false(desc.configurable, "[Unforgeable] property must not be configurable");
    }
    else
    {
        assert_true(desc.configurable, "property must be configurable");
    }

    
    
    
    
    
    assert_equals(typeof desc.get, "function", "getter must be Function");
    assert_equals(desc.get.length, 0, "getter length must be 0");
    
    assert_throws(new TypeError(), function()
    {
        desc.get.call({});
    }.bind(this), "calling getter on wrong object type must throw TypeError");

    
    
    
    
    
    
    
    
    
    
    
    if (member.readonly
    && !member.has_extended_attribute("PutForwards")
    && !member.has_extended_attribute("Replaceable"))
    {
        assert_equals(desc.set, undefined, "setter must be undefined for readonly attributes");
    }
    else
    {
        assert_equals(typeof desc.set, "function", "setter must be function for PutForwards, Replaceable, or non-readonly attributes");
        assert_equals(desc.set.length, 1, "setter length must be 1");
    }
}



function IdlInterfaceMember(obj)

{
    for (var k in obj)
    {
        if (k == "extAttrs")
        {
            this.extAttrs = obj.extAttrs ? obj.extAttrs : [];
        }
        else
        {
            this[k] = obj[k];
        }
    }
    if (!("extAttrs" in this))
    {
        this.extAttrs = [];
    }
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
            return 7;

        case "DOMString":
            return "foo";

        case "object":
            return {a: "b"};

        case "Node":
            return document.createTextNode("abc");
    }
    return null;
}

})();

