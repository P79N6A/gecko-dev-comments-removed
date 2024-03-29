







function test() {
  let { Parser, ParserHelpers, SyntaxTreeVisitor } =
    Cu.import("resource:///modules/devtools/Parser.jsm", {});

  function verify(source, predicate, [sline, scol], [eline, ecol]) {
    let ast = Parser.reflectionAPI.parse(source);
    let node = SyntaxTreeVisitor.filter(ast, predicate).pop();
    let loc = ParserHelpers.getNodeLocation(node);

    is(loc.start.toSource(), { line: sline, column: scol }.toSource(),
      "The start location was correct for the identifier in: '" + source + "'.");
    is(loc.end.toSource(), { line: eline, column: ecol }.toSource(),
      "The end location was correct for the identifier in: '" + source + "'.");
  }

  

  
  verify("function foo(){}", e => e.name == "foo", [1, 9], [1, 12]);
  verify("\nfunction\nfoo\n(\n)\n{\n}\n", e => e.name == "foo", [3, 0], [3, 3]);

  verify("({bar:function foo(){}})", e => e.name == "foo", [1, 15], [1, 18]);
  verify("(\n{\nbar\n:\nfunction\nfoo\n(\n)\n{\n}\n}\n)", e => e.name == "foo", [6, 0], [6, 3]);

  
  verify("({bar:function foo(){}})", e => e.name == "bar", [1, 2], [1, 5]);
  verify("(\n{\nbar\n:\nfunction\nfoo\n(\n)\n{\n}\n}\n)", e => e.name == "bar", [3, 0], [3, 3]);

  

  
  verify("foo.bar", e => e.name == "bar", [1, 4], [1, 7]);
  verify("\nfoo\n.\nbar\n", e => e.name == "bar", [4, 0], [4, 3]);

  
  verify("foo.bar", e => e.name == "foo", [1, 0], [1, 3]);
  verify("\nfoo\n.\nbar\n", e => e.name == "foo", [2, 0], [2, 3]);

  

  
  verify("let foo = bar", e => e.name == "foo", [1, 4], [1, 7]);
  verify("\nlet\nfoo\n=\nbar\n", e => e.name == "foo", [3, 0], [3, 3]);

  
  verify("let foo = bar", e => e.name == "bar", [1, 10], [1, 13]);
  verify("\nlet\nfoo\n=\nbar\n", e => e.name == "bar", [5, 0], [5, 3]);

  
  verify("foo = bar", e => e.name == "foo", [1, 0], [1, 3]);
  verify("\nfoo\n=\nbar\n", e => e.name == "foo", [2, 0], [2, 3]);
  verify("foo = bar", e => e.name == "bar", [1, 6], [1, 9]);
  verify("\nfoo\n=\nbar\n", e => e.name == "bar", [4, 0], [4, 3]);

  

  verify("foo: bar", e => e.name == "foo", [1, 0], [1, 3]);
  verify("\nfoo\n:\nbar\n", e => e.name == "foo", [2, 0], [2, 3]);

  verify("foo: for(;;) break foo", e => e.name == "foo", [1, 19], [1, 22]);
  verify("\nfoo\n:\nfor(\n;\n;\n)\nbreak\nfoo\n", e => e.name == "foo", [9, 0], [9, 3]);

  verify("foo: bar", e => e.name == "foo", [1, 0], [1, 3]);
  verify("\nfoo\n:\nbar\n", e => e.name == "foo", [2, 0], [2, 3]);

  verify("foo: for(;;) continue foo", e => e.name == "foo", [1, 22], [1, 25]);
  verify("\nfoo\n:\nfor(\n;\n;\n)\ncontinue\nfoo\n", e => e.name == "foo", [9, 0], [9, 3]);

  finish();
}
