






































var extended_unicode_string = "The Euro Symbol is '\u20ac'";

function MakeTestInterface()
{
    var clazz = Components.classes["Python.TestComponent"];
    var iface = Components.interfaces.nsIPythonTestInterfaceDOMStrings;
    return new clazz(iface);
}    


Array.prototype.compareArrays = function(arr) {
    if (this.length != arr.length) return false;
    for (var i = 0; i < arr.length; i++) {
        if (this[i].compareArrays) { 
            if (!this[i].compareArrays(arr[i])) return false;
            else continue;
        }
        if (this[i] != arr[i]) return false;
    }
    return true;
}

var c = new MakeTestInterface();

if (c.boolean_value != 1)
	throw("boolean_value has wrong initial value");
c.boolean_value = false;
if (c.boolean_value != false)
	throw("boolean_value has wrong new value");




if (c.char_value != 'a')
	throw("char_value has wrong initial value");
c.char_value = 'b';
if (c.char_value != 'b')
	throw("char_value has wrong new value");

if (c.wchar_value != 'b')
	throw("wchar_value has wrong initial value");
c.wchar_value = 'c';
if (c.wchar_value != 'c')
	throw("wchar_value has wrong new value");

if (c.string_value != 'cee')
	throw("string_value has wrong initial value");
c.string_value = 'dee';
if (c.string_value != 'dee')
	throw("string_value has wrong new value");

if (c.wstring_value != 'dee')
	throw("wstring_value has wrong initial value");
c.wstring_value = 'eee';
if (c.wstring_value != 'eee')
	throw("wstring_value has wrong new value");
c.wstring_value = extended_unicode_string;
if (c.wstring_value != extended_unicode_string)
	throw("wstring_value has wrong new value");

if (c.domstring_value != 'dom')
	throw("domstring_value has wrong initial value");
c.domstring_value = 'New value';
if (c.domstring_value != 'New value')
	throw("domstring_value has wrong new value");
c.domstring_value = extended_unicode_string;
if (c.domstring_value != extended_unicode_string)
	throw("domstring_value has wrong new value");

if (c.utf8string_value != 'utf8string')
	throw("utf8string_value has wrong initial value");
c.utf8string_value = 'New value';
if (c.utf8string_value != 'New value')
	throw("utf8string_value has wrong new value");
c.utf8string_value = extended_unicode_string;
if (c.utf8string_value != extended_unicode_string)
	throw("utf8string_value has wrong new value");

var v = new Object();
v.value = "Hello"
var l = new Object();
l.value = v.value.length;
c.DoubleString(l, v);
if ( v.value != "HelloHello")
	throw("Could not double the string!");

var v = new Object();
v.value = "Hello"
var l = new Object();
l.value = v.value.length;
c.DoubleWideString(l, v);
if ( v.value != "HelloHello")
	throw("Could not double the wide string!");


var v = new Array()
v[0] = 1;
v[2] = 2;
v[3] = 3;
var v2 = new Array()
v2[0] = 4;
v2[2] = 5;
v2[3] = 6;
if (c.SumArrays(v.length, v, v2) != 21)
	throw("Could not sum an array of integers!");

var count = new Object();
count.value = 0;
var out = [];
c.DoubleStringArray(count, out);

v = new Array();
var v2 = c.CopyVariant(v);
if (!v.compareArrays(v2))
	throw("Could not copy an empty array of nsIVariant");

v = new Array();
v[0] = 1;
v[1] = "test";
var v2 = c.CopyVariant(v);
if (!v.compareArrays(v2))
	throw("Could not copy an empty array of nsIVariant");

print("OK: javascript successfully tested the Python test component.");
