





























description("Tests generation of synthetic and real GL errors");

var context = create3DContext();
var program = loadStandardProgram(context);








shouldBe("context.getError()", "0");

debug("Testing getActiveAttrib");

shouldBeNull("context.getActiveAttrib(null, 2)");
glErrorShouldBe(context, context.INVALID_OPERATION);

glErrorShouldBe(context, context.NO_ERROR);

shouldBeNull("context.getActiveAttrib(program, 2)");
glErrorShouldBe(context, context.INVALID_VALUE);

glErrorShouldBe(context, context.NO_ERROR);

debug("Testing getActiveUniform");

shouldBeNull("context.getActiveUniform(null, 0)");
glErrorShouldBe(context, context.INVALID_OPERATION);

glErrorShouldBe(context, context.NO_ERROR);

shouldBeNull("context.getActiveUniform(program, 50)");
glErrorShouldBe(context, context.INVALID_VALUE);

glErrorShouldBe(context, context.NO_ERROR);

debug("Testing attempts to manipulate the default framebuffer");
shouldBeUndefined("context.bindFramebuffer(context.FRAMEBUFFER, 0)");
glErrorShouldBe(context, context.NO_ERROR);
shouldBeUndefined("context.framebufferRenderbuffer(context.FRAMEBUFFER, context.DEPTH_ATTACHMENT, context.RENDERBUFFER, 0)");

glErrorShouldBe(context, context.INVALID_OPERATION);

glErrorShouldBe(context, context.NO_ERROR);
shouldBeUndefined("context.framebufferTexture2D(context.FRAMEBUFFER, context.COLOR_ATTACHMENT0, context.TEXTURE_2D, 0, 0)");

glErrorShouldBe(context, context.INVALID_OPERATION);

glErrorShouldBe(context, context.NO_ERROR);

successfullyParsed = true;
