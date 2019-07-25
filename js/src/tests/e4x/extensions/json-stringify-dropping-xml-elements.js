

assertEq(JSON.stringify({foo: 123, bar: <x><y></y></x>, baz: 123}),
         '{"foo":123,"baz":123}');

assertEq(JSON.stringify([123, <x><y></y></x>, 456]),
         '[123,null,456]');



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
