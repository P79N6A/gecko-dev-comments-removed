






function test() {
  let { Parser } = Cu.import("resource:///modules/devtools/Parser.jsm", {});

  let source = "let x + 42;";
  let parser = new Parser();
  
  parser.logExceptions = false;
  let parsed = parser.get(source);

  ok(parsed,
    "An object should be returned even though the source had a syntax error.");

  is(parser.errors.length, 1,
    "There should be one error logged when parsing.");
  is(parser.errors[0].name, "SyntaxError",
    "The correct exception was caught.");
  is(parser.errors[0].message, "missing ; before statement",
    "The correct exception was caught.");

  finish();
}
