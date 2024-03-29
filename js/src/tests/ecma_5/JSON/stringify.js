function assertStringify(v, expect)
{
  assertEq(JSON.stringify(v), expect);
}

assertStringify({}, "{}");
assertStringify([], "[]");
assertStringify({"foo":"bar"}, '{"foo":"bar"}');
assertStringify({"null":null}, '{"null":null}');
assertStringify({"five":5}, '{"five":5}');
assertStringify({"five":5, "six":6}, '{"five":5,"six":6}');
assertStringify({"x":{"y":"z"}}, '{"x":{"y":"z"}}');
assertStringify({"w":{"x":{"y":"z"}}}, '{"w":{"x":{"y":"z"}}}');
assertStringify([1,2,3], '[1,2,3]');
assertStringify({"w":{"x":{"y":[1,2,3]}}}, '{"w":{"x":{"y":[1,2,3]}}}');
assertStringify({"false":false}, '{"false":false}');
assertStringify({"true":true}, '{"true":true}');
assertStringify({"child has two members": {"this":"one", 2:"and this one"}},
                '{"child has two members":{"2":"and this one","this":"one"}}');
assertStringify({"x":{"a":"b","c":{"y":"z"},"f":"g"}},
                '{"x":{"a":"b","c":{"y":"z"},"f":"g"}}');
assertStringify({"x":[1,{"y":"z"},3]}, '{"x":[1,{"y":"z"},3]}');
assertStringify([new String("hmm")], '["hmm"]');
assertStringify([new Boolean(true)], '[true]');
assertStringify([new Number(42)], '[42]');
assertStringify([new Date(Date.UTC(1978, 8, 13, 12, 24, 34, 23))],
                '["1978-09-13T12:24:34.023Z"]');
assertStringify([1,,3], '[1,null,3]');
assertStringify({"mm\"mm":"hmm"}, '{"mm\\\"mm":"hmm"}');
assertStringify({"mm\"mm\"mm":"hmm"}, '{"mm\\\"mm\\\"mm":"hmm"}');
assertStringify({'"':"hmm"}, '{"\\\"":"hmm"}');
assertStringify({'\\':"hmm"}, '{"\\\\":"hmm"}');
assertStringify({'mmm\\mmm':"hmm"}, '{"mmm\\\\mmm":"hmm"}');
assertStringify({'mmm\\mmm\\mmm':"hmm"}, '{"mmm\\\\mmm\\\\mmm":"hmm"}');
assertStringify({"mm\u000bmm":"hmm"}, '{"mm\\u000bmm":"hmm"}');
assertStringify({"mm\u0000mm":"hmm"}, '{"mm\\u0000mm":"hmm"}');

var x = {"free":"variable"};
assertStringify(x, '{"free":"variable"}');
assertStringify({"y":x}, '{"y":{"free":"variable"}}');


assertStringify({ a: [1,2,3] }, '{"a":[1,2,3]}');

assertStringify({"y": { foo: function(hmm) { return hmm; } } }, '{"y":{}}');


var hmm = { toJSON: function() { return {"foo":"bar"} } };
assertStringify({"hmm":hmm}, '{"hmm":{"foo":"bar"}}');
assertStringify(hmm, '{"foo":"bar"}'); 


var Y = function() {
  this.not = "there?";
  this.d = "e";
};
Y.prototype = {
  not: "there?",
  toJSON: function() { return {"foo":"bar"}}
};
var y = new Y();
assertStringify(y.toJSON(), '{"foo":"bar"}');
assertStringify(y, '{"foo":"bar"}');


assertStringify({"hmm": { toJSON: function() { return; } } }, '{}');


var x = new Array();
x[0] = 1;
x.foo = "bar";
assertStringify(x, '[1]');


var X = function() { this.a = "b" };
X.prototype = { c: "d" };
assertStringify(new X(), '{"a":"b"}');



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
