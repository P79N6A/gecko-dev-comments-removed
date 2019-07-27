ReflectionTests = {};

ReflectionTests.start = new Date().getTime();























ReflectionTests.resolveUrl = function(url) {
    var el = document.createElement("a");
    el.href = String(url);
    var ret = el.protocol + "//" + el.host + el.pathname + el.search + el.hash;
    if (ret == "//") {
        return "";
    } else {
        return ret;
    }
};




ReflectionTests.urlsExpected = function(urls) {
    var expected = "";
    
    urls = urls + "";
    var split = urls.split(" ");
    for (var j = 0; j < split.length; j++) {
        if (split[j] == "") {
            continue;
        }
        var append = ReflectionTests.resolveUrl(split[j]);
        if (append == "") {
            continue;
        }
        if (expected == "") {
            expected = append;
        } else {
            expected += " " + append;
        }
    }
    return expected;
};






ReflectionTests.parseNonneg = function(input) {
  var value = this.parseInt(input);
  if (value === false || value < 0) {
      return false;
  }
  return value;
};





ReflectionTests.parseInt = function(input) {
    var position = 0;
    var sign = 1;
    
    while (input.length > position && /^[ \t\n\f\r]$/.test(input[position])) {
        position++;
    }
    if (position >= input.length) {
        return false;
    }
    if (input[position] == "-") {
        sign = -1;
        position++;
    } else if (input[position] == "+") {
        position++;
    }
    if (position >= input.length) {
        return false;
    }
    if (!/^[0-9]$/.test(input[position])) {
        return false;
    }
    var value = 0;
    while (input.length > position && /^[0-9]$/.test(input[position])) {
        value *= 10;
        
        value += input.charCodeAt(position) - "0".charCodeAt(0);
        position++;
    }
    if (value === 0) {
        return 0;
    }
    return sign * value;
};


var binaryString = "\x00\x01\x02\x03\x04\x05\x06\x07 "
    + "\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f "
    + "\x10\x11\x12\x13\x14\x15\x16\x17 "
    + "\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f ";
var maxInt = 2147483647;
var minInt = -2147483648;
var maxUnsigned = 4294967295;











































ReflectionTests.typeMap = {
    









    "string": {
        "jsType": "string",
        "defaultVal": "",
        "domTests": ["", " " + binaryString + " foo ", undefined, 7, 1.5, true,
                     false, {"test": 6}, NaN, +Infinity, -Infinity, "\0", null,
                     {"toString":function(){return "test-toString";}},
                     {"valueOf":function(){return "test-valueOf";}, toString:null}
                ]
    },
    









    "url": {
        "jsType": "string",
        "defaultVal": "",
        "domTests": ["", " foo ", "http://site.example/",
                     "//site.example/path???@#l", binaryString, undefined, 7, 1.5, true,
                     false, {"test": 6}, NaN, +Infinity, -Infinity, "\0", null,
                     {"toString":function(){return "test-toString";}},
                     {"valueOf":function(){return "test-valueOf";}, toString:null}],
        "domExpected": ReflectionTests.resolveUrl,
        "idlIdlExpected": ReflectionTests.resolveUrl
    },
    













    "urls": {
        "jsType": "string",
        "defaultVal": "",
        "domTests": ["", " foo   ", "http://site.example/ foo  bar   baz",
                     "//site.example/path???@#l", binaryString, undefined, 7, 1.5, true,
                     false, {"test": 6}, NaN, +Infinity, -Infinity, "\0", null,
                     {"toString":function(){return "test-toString";}},
                     {"valueOf":function(){return "test-valueOf";}, toString:null}],
        "domExpected": ReflectionTests.urlsExpected,
        "idlIdlExpected": ReflectionTests.urlsExpected
    },
    





















































    "enum": {
        "jsType": "string",
        "defaultVal": "",
        "domTests": ["", " " + binaryString + " foo ", undefined, 7, 1.5, true,
                 false, {"test": 6}, NaN, +Infinity, -Infinity, "\0", null,
                 {"toString":function(){return "test-toString";}},
                 {"valueOf":function(){return "test-valueOf";}, toString:null}]
    },
    







    "boolean": {
        "jsType": "boolean",
        "defaultVal": false,
        "domTests": ["", " foo ", undefined, null, 7, 1.5, true, false,
                     {"test": 6}, NaN, +Infinity, -Infinity, "\0",
                     {"toString":function(){return "test-toString";}},
                     {"valueOf":function(){return "test-valueOf";}, toString:null}],
        "domExpected": function(val) {
            return true;
        }
    },
    











    "long": {
        "jsType": "number",
        "defaultVal": 0,
        "domTests": [-36, -1, 0, 1, maxInt, minInt, maxInt + 1, minInt - 1,
                     maxUnsigned, maxUnsigned + 1, "", "-1", "-0", "0", "1",
                     " " + binaryString + " foo ",
                     
                     
                     
                     
                     
                     
                     
                     
                     
                     
                     
                     undefined, 1.5, true, false, {"test": 6}, NaN, +Infinity,
                     -Infinity, "\0",
                     {toString:function() {return 2;}, valueOf: null},
                     {valueOf:function() {return 3;}}],
        "domExpected": function(val) {
            var parsed = ReflectionTests.parseInt(val + "");
            if (parsed === false || parsed > maxInt || parsed < minInt) {
                return null;
            }
            return parsed;
        },
        "idlTests":       [-36, -1, 0, 1, 2147483647, -2147483648],
        "idlDomExpected": [-36, -1, 0, 1, 2147483647, -2147483648]
    },
    













    "limited long": {
        "jsType": "number",
        "defaultVal": -1,
        "domTests": [minInt - 1, minInt, -36, -1, -0, 0, 1, maxInt, maxInt + 1,
                     maxUnsigned, maxUnsigned + 1, "", "-1", "-0", "0", "1",
                     " " + binaryString + " foo ",
                     
                     
                     
                     
                     
                     undefined, 1.5, true, false, {"test": 6}, NaN, +Infinity,
                     -Infinity, "\0",
                     {toString:function() {return 2;}, valueOf: null},
                     {valueOf:function() {return 3;}}],
        "domExpected": function(val) {
            var parsed = ReflectionTests.parseNonneg(val + "");
            if (parsed === false || parsed > maxInt || parsed < minInt) {
                return null;
            }
            return parsed;
        },
        "idlTests":       [minInt, -36,  -1,   0, 1, maxInt],
        "idlDomExpected": [null,   null, null, 0, 1, maxInt]
    },
    











    "unsigned long": {
        "jsType": "number",
        "defaultVal": 0,
        "domTests": [minInt - 1, minInt, -36,  -1,   0, 1, 257, maxInt,
                     maxInt + 1, maxUnsigned, maxUnsigned + 1, "", "-1", "-0", "0", "1",
                     
                     
                     
                     
                     
                     " " + binaryString + " foo ", undefined, 1.5, true, false,
                     {"test": 6}, NaN, +Infinity, -Infinity, "\0",
                     {toString:function() {return 2;}, valueOf: null},
                     {valueOf:function() {return 3;}}],
        "domExpected": function(val) {
            var parsed = ReflectionTests.parseNonneg(val + "");
            
            if (parsed === false || parsed < 0 || parsed > maxInt) {
                return null;
            }
            return parsed;
        },
        "idlTests": [0, 1, 257, 2147483647, "-0"],
        "idlIdlExpected": [0, 1, 257, 2147483647, 0]
    },
    















    "limited unsigned long": {
        "jsType": "number",
        "defaultVal": 1,
        "domTests": [minInt - 1, minInt, -36,  -1,   0,    1, maxInt,
                     maxInt + 1, maxUnsigned, maxUnsigned + 1, "", "-1", "-0", "0", "1",
                     
                     
                     
                     
                     
                     " " + binaryString + " foo ", undefined, 1.5, true, false,
                     {"test": 6}, NaN, +Infinity, -Infinity, "\0",
                     {toString:function() {return 2;}, valueOf: null},
                     {valueOf:function() {return 3;}}],
        "domExpected": function(val) {
            var parsed = ReflectionTests.parseNonneg(val + "");
            
            if (parsed === false || parsed < 1 || parsed > maxInt) {
                return null;
            }
            return parsed;
        },
        "idlTests":       [0,    1, 2147483647],
        "idlDomExpected": [null, 1, 2147483647]
    },
    























    "double": {
        "jsType": "number",
        "defaultVal": 0.0,
        "domTests": [minInt - 1, minInt, -36, -1, 0, 1, maxInt,
            maxInt + 1, maxUnsigned, maxUnsigned + 1, "",
            
            
            
            
            
            " " + binaryString + " foo ", undefined, 1.5, true, false,
            {"test": 6}, NaN, +Infinity, -Infinity, "\0",
            {toString:function() {return 2;}, valueOf: null},
            {valueOf:function() {return 3;}}],
        "domExpected": [minInt - 1, minInt, -36, -1, 0, 1, maxInt,
                        maxInt + 1, maxUnsigned, maxUnsigned + 1, null,
                        
                        
                        
                        
                        
                        
                        
                        null, null, 1.5, null, null,
                        null, null, null, null, null,
                        2, 3],
        
        
        "idlTests":       [ -10000000000,   -1,  -0,   0,   1,   10000000000],
        "idlDomExpected": ["-10000000000", "-1", "0", "0", "1", "10000000000"],
        "idlIdlExpected": [ -10000000000,   -1,  -0,   0,   1,   10000000000]
    }
};

for (var type in ReflectionTests.typeMap) {
    var props = ReflectionTests.typeMap[type];
    var cast = window[props.jsType[0].toUpperCase() + props.jsType.slice(1)];
    if (props.domExpected === undefined) {
        props.domExpected = props.domTests.map(cast);
    } else if (typeof props.domExpected == "function") {
        props.domExpected = props.domTests.map(props.domExpected);
    }
    if (props.idlTests === undefined) {
        props.idlTests = props.domTests;
    }
    if (props.idlDomExpected === undefined) {
        props.idlDomExpected = props.idlTests.map(cast);
    } else if (typeof props.idlDomExpected == "function") {
        props.idlDomExpected = props.idlTests.map(props.idlDomExpected);
    }
    if (props.idlIdlExpected === undefined) {
        props.idlIdlExpected = props.idlDomExpected;
    } else if (typeof props.idlIdlExpected == "function") {
        props.idlIdlExpected = props.idlTests.map(props.idlIdlExpected);
    }
}













ReflectionTests.reflects = function(data, idlName, idlObj, domName, domObj) {
    
    if (typeof data == "string") {
        data = {type: data};
    }
    if (domName === undefined) {
        domName = idlName;
    }
    if (typeof idlObj == "string") {
        idlObj = document.createElement(idlObj);
    }
    if (domObj === undefined) {
        domObj = idlObj;
    }

    
    
    
    ReflectionHarness.currentTestInfo = {data: data, idlName: idlName, idlObj: idlObj, domName: domName, domObj: domObj};

    ReflectionHarness.testWrapper(function() {
        ReflectionTests.doReflects(data, idlName, idlObj, domName, domObj);
    });
};




ReflectionTests.doReflects = function(data, idlName, idlObj, domName, domObj) {
    
    if (this.typeMap[data.type] === undefined) {
        if (unimplemented.indexOf(data.type) == -1) {
            unimplemented.push(data.type);
        }
        return;
    }

    var typeInfo = this.typeMap[data.type];

    
    
    if (!ReflectionHarness.test(typeof idlObj[idlName], typeInfo.jsType, "typeof IDL attribute")) {
        return;
    }

    
    var defaultVal = data.defaultVal;
    if (defaultVal === undefined) {
        defaultVal = typeInfo.defaultVal;
    }
    if (defaultVal !== null) {
        ReflectionHarness.test(idlObj[idlName], defaultVal, "IDL get with DOM attribute unset");
    }

    var domTests = typeInfo.domTests.slice(0);
    var domExpected = typeInfo.domExpected.map(function(val) { return val === null ? defaultVal : val; });
    var idlTests = typeInfo.idlTests.slice(0);
    var idlDomExpected = typeInfo.idlDomExpected.slice(0);
    var idlIdlExpected = typeInfo.idlIdlExpected.slice(0);
    switch (data.type) {
        
        case "boolean":
        domTests.push(domName);
        domExpected.push(true);
        break;

        case "enum":
        
        if (typeof data.invalidVal == "undefined") {
            data.invalidVal = defaultVal;
        }
        if (typeof data.nonCanon == "undefined") {
            data.nonCanon = {};
        }
        for (var i = 0; i < data.keywords.length; i++) {
            if (data.keywords[i] != "") {
                domTests.push(data.keywords[i], "x" + data.keywords[i], data.keywords[i] + "\0");
                idlTests.push(data.keywords[i], "x" + data.keywords[i], data.keywords[i] + "\0");
            }

            if (data.keywords[i].length > 1) {
                domTests.push(data.keywords[i].slice(1));
                idlTests.push(data.keywords[i].slice(1));
            }

            if (data.keywords[i] != data.keywords[i].toLowerCase()) {
                domTests.push(data.keywords[i].toLowerCase());
                idlTests.push(data.keywords[i].toLowerCase());
            }
            if (data.keywords[i] != data.keywords[i].toUpperCase()) {
                domTests.push(data.keywords[i].toUpperCase());
                idlTests.push(data.keywords[i].toUpperCase());
            }
        }

        
        
        idlDomExpected = idlTests.slice(0);

        
        domExpected = [];
        idlIdlExpected = [];
        for (var i = 0; i < domTests.length; i++) {
            domExpected.push(this.enumExpected(data.keywords, data.nonCanon, data.invalidVal, domTests[i]));
        }
        for (var i = 0; i < idlTests.length; i++) {
            idlIdlExpected.push(this.enumExpected(data.keywords, data.nonCanon, data.invalidVal, idlTests[i]));
        }
        break;

        case "string":
        if ("treatNullAsEmptyString" in data) {
            for (var i = 0; i < idlTests.length; i++) {
                if (idlTests[i] === null) {
                    idlDomExpected[i] = idlIdlExpected[i] = "";
                }
            }
        }
        break;
    }
    if (domObj.tagName.toLowerCase() == "canvas" && (domName == "width" || domName == "height")) {
        
        
        
        
        domTests = domTests.filter(function(element, index, array) { return domExpected[index] < 1000; });
        domExpected = domExpected.filter(function(element, index, array) { return element < 1000; });
        idlTests = idlTests.filter(function(element, index, array) { return idlIdlExpected[index] < 1000; });
        idlDomExpected = idlDomExpected.filter(function(element, index, array) { return idlIdlExpected[index] < 1000; });
        idlIdlExpected = idlIdlExpected.filter(function(element, index, array) { return idlIdlExpected[index] < 1000; });
    }

    for (var i = 0; i < domTests.length; i++) {
        if (domExpected[i] === null) {
            
            
            
            
            continue;
        }
        try {
            domObj.setAttribute(domName, domTests[i]);
            
            
            
            if (domTests[i] !== null) {
                ReflectionHarness.test(domObj.getAttribute(domName), domTests[i] + "", "setAttribute() to " + ReflectionHarness.stringRep(domTests[i]) + " followed by getAttribute()");
            }
            ReflectionHarness.test(idlObj[idlName], domExpected[i], "setAttribute() to " + ReflectionHarness.stringRep(domTests[i]) + " followed by IDL get");
            if (ReflectionHarness.catchUnexpectedExceptions) {
                ReflectionHarness.success();
            }
        } catch (err) {
            if (ReflectionHarness.catchUnexpectedExceptions) {
                ReflectionHarness.failure("Exception thrown during tests with setAttribute() to " + ReflectionHarness.stringRep(domTests[i]));
            } else {
                throw err;
            }
        }
    }

    for (var i = 0; i < idlTests.length; i++) {
        if (idlDomExpected[i] === null && data.type != "enum") {
            ReflectionHarness.testException("INDEX_SIZE_ERR", function() {
                idlObj[idlName] = idlTests[i];
            }, "IDL set to " + ReflectionHarness.stringRep(idlTests[i]) + " must throw INDEX_SIZE_ERR");
        } else {
            ReflectionHarness.run(function() {
                idlObj[idlName] = idlTests[i];
                if (data.type == "boolean") {
                    
                    ReflectionHarness.test(domObj.hasAttribute(domName), Boolean(idlTests[i]), "IDL set to " + ReflectionHarness.stringRep(idlTests[i]) + " followed by hasAttribute()");
                } else if (idlDomExpected[i] !== null) {
                    ReflectionHarness.test(domObj.getAttribute(domName), idlDomExpected[i] + "", "IDL set to " + ReflectionHarness.stringRep(idlTests[i]) + " followed by getAttribute()");
                }
                if (idlIdlExpected[i] !== null) {
                    ReflectionHarness.test(idlObj[idlName], idlIdlExpected[i], "IDL set to " + ReflectionHarness.stringRep(idlTests[i]) + " followed by IDL get");
                }
                if (ReflectionHarness.catchUnexpectedExceptions) {
                    ReflectionHarness.success();
                }
            }, "IDL set to " + ReflectionHarness.stringRep(idlTests[i]) + " should not throw");
        }
    }
};








ReflectionTests.enumExpected = function(keywords, nonCanon, invalidVal, contentVal) {
    var ret = invalidVal;
    for (var i = 0; i < keywords.length; i++) {
        if (String(contentVal).toLowerCase() == keywords[i].toLowerCase()) {
            ret = keywords[i];
            break;
        }
    }
    if (typeof nonCanon[ret] != "undefined") {
        return nonCanon[ret];
    }
    return ret;
};















var unimplemented = [];
for (var element in elements) {
    ReflectionTests.reflects("string", "title", element);
    ReflectionTests.reflects("string", "lang", element);
    ReflectionTests.reflects({type: "enum", keywords: ["ltr", "rtl", "auto"]}, "dir", element);
    ReflectionTests.reflects("string", "className", element, "class");
    ReflectionTests.reflects("tokenlist", "classList", element, "class");
    ReflectionTests.reflects("boolean", "hidden", element);
    ReflectionTests.reflects("string", "accessKey", element);
    
    
    ReflectionTests.reflects({type: "long", defaultVal: null}, "tabIndex", element);
    
    

    for (var idlAttrName in elements[element]) {
        var type = elements[element][idlAttrName];
        ReflectionTests.reflects(type, idlAttrName, element,
            typeof type == "object" && "domAttrName" in type ? type.domAttrName : idlAttrName);
    }
}

for (var i = 0; i < extraTests.length; i++) {
    extraTests[i]();
}

var time = document.getElementById("time");
if (time) {
    time.innerHTML = (new Date().getTime() - ReflectionTests.start)/1000;
}

if (unimplemented.length) {
    var p = document.createElement("p");
    p.textContent = "(Note: missing tests for types " + unimplemented.join(", ") + ".)";
    document.body.appendChild(p);
}
