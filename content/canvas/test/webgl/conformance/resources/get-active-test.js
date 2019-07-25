
























description("Test of getActiveAttrib and getActiveUniform");

var context = create3DContext();
var context2 = create3DContext();
var program = loadStandardProgram(context);
var program2 = loadStandardProgram(context2);

glErrorShouldBe(context, context.NO_ERROR);
shouldBe("context.getActiveUniform(program, 0).name", "'u_modelViewProjMatrix'");
shouldBe("context.getActiveUniform(program, 0).type", "context.FLOAT_MAT4");
shouldBe("context.getActiveUniform(program, 0).size", "1");
shouldBe("context.getActiveUniform(program, 1)", "null");
glErrorShouldBe(context, context.INVALID_VALUE);
shouldBe("context.getActiveUniform(program, -1)", "null");
glErrorShouldBe(context, context.INVALID_VALUE);
shouldBe("context.getActiveUniform(null, 0)", "null");
glErrorShouldBe(context, context.INVALID_VALUE);


var info = [
  context.getActiveAttrib(program, 0),
  context.getActiveAttrib(program, 1)
];

var expected = [
  { name: 'a_normal', type: context.FLOAT_VEC3, size: 1 },
  { name: 'a_vertex', type: context.FLOAT_VEC4, size: 1 }
];

if (info[0].name != expected[0].name) {
  t = info[0];
  info[0] = info[1];
  info[1] = t;
}

for (var ii = 0; ii < info.length; ++ii) {
  shouldBe("info[ii].name", "expected[ii].name");
  shouldBe("info[ii].type", "expected[ii].type");
  shouldBe("info[ii].size", "expected[ii].size");
}
shouldBe("context.getActiveAttrib(program, 2)", "null");
glErrorShouldBe(context, context.INVALID_VALUE);
shouldBe("context.getActiveAttrib(program, -1)", "null");
glErrorShouldBe(context, context.INVALID_VALUE);
shouldBe("context.getActiveAttrib(null, 0)", "null");
glErrorShouldBe(context, context.INVALID_VALUE);

debug("Check trying to get attribs from different context");
shouldBe("context2.getActiveAttrib(program, 0)", "null");
glErrorShouldBe(context2, context2.INVALID_OPERATION);
shouldBe("context2.getActiveUniform(program, 0)", "null");
glErrorShouldBe(context2, context2.INVALID_OPERATION);

debug("Check trying to get attribs from deleted program");
context.deleteProgram(program);
shouldBe("context.getActiveUniform(program, 0)", "null");
glErrorShouldBe(context, context.INVALID_VALUE);
shouldBe("context.getActiveAttrib(program, 0)", "null");
glErrorShouldBe(context, context.INVALID_VALUE);

successfullyParsed = true;
