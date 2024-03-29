# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:




















this.G_Protocol4Parser = function G_Protocol4Parser() {
  this.debugZone = "protocol4";

  this.protocol4RegExp_ = new RegExp("([^:]+):\\d+:(.*)$");
  this.newlineRegExp_ = new RegExp("(\\r)?\\n");
}










G_Protocol4Parser.prototype.parse = function(text) {

  var response = {};
  if (!text)
    return response;

  
  var lines = text.split(this.newlineRegExp_);
  for (var i = 0; i < lines.length; i++)
    if (this.protocol4RegExp_.exec(lines[i]))
      response[RegExp.$1] = RegExp.$2;

  return response;
}










G_Protocol4Parser.prototype.serialize = function(map) {
  if (typeof map != "object")
    throw new Error("map must be an object");

  var text = "";
  for (var key in map) {
    if (typeof map[key] != "string")
      throw new Error("Keys and values must be strings");
    
    text += key + ":" + map[key].length + ":" + map[key] + "\n";
  }
  
  return text;
}

#ifdef DEBUG



this.TEST_G_Protocol4Parser = function TEST_G_Protocol4Parser() {
  if (G_GDEBUG) {
    var z = "protocol4 UNITTEST";
    G_debugService.enableZone(z);

    G_Debug(z, "Starting");

    var p = new G_Protocol4Parser();
    
    let isEmpty = function (map) {
      for (var key in map) 
        return false;
      return true;
    };

    G_Assert(z, isEmpty(p.parse(null)), "Parsing null broken");
    G_Assert(z, isEmpty(p.parse("")), "Parsing nothing broken");

    var t = "foo:3:bar";
    G_Assert(z, p.parse(t)["foo"] === "bar", "Parsing one line broken");

    t = "foo:3:bar\n";
    G_Assert(z, p.parse(t)["foo"] === "bar", "Parsing line with lf broken");

    t = "foo:3:bar\r\n";
    G_Assert(z, p.parse(t)["foo"] === "bar", "Parsing with crlf broken");


    t = "foo:3:bar\nbar:3:baz\r\nbom:3:yaz\n";
    G_Assert(z, p.parse(t)["foo"] === "bar", "First in multiline");
    G_Assert(z, p.parse(t)["bar"] === "baz", "Second in multiline");
    G_Assert(z, p.parse(t)["bom"] === "yaz", "Third in multiline");
    G_Assert(z, p.parse(t)[""] === undefined, "Nonexistent in multiline");
    
    

    var original = { 
      "1": "1", 
      "2": "2", 
      "foobar": "baz",
      "hello there": "how are you?" ,
    };
    var deserialized = p.parse(p.serialize(original));
    for (var key in original) 
      G_Assert(z, original[key] === deserialized[key], 
               "Trouble (de)serializing " + key);

    G_Debug(z, "PASSED");  
  }
}
#endif
