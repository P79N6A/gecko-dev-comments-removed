































#include "cairoint.h"
#include "cairo-gl-private.h"

static GLint
_cairo_gl_compile_glsl(GLenum type, GLint *shader_out, const char *source)
{
    GLint ok;
    GLint shader;

    shader = glCreateShaderObjectARB (type);
    glShaderSourceARB (shader, 1, (const GLchar **)&source, NULL);
    glCompileShaderARB (shader);
    glGetObjectParameterivARB (shader, GL_OBJECT_COMPILE_STATUS_ARB, &ok);
    if (!ok) {
	GLchar *info;
	GLint size;

	glGetObjectParameterivARB (shader, GL_OBJECT_INFO_LOG_LENGTH_ARB,
				   &size);
	info = malloc (size);

	if (info)
	    glGetInfoLogARB (shader, size, NULL, info);
	fprintf (stderr, "Failed to compile %s: %s\n",
		 type == GL_FRAGMENT_SHADER ? "FS" : "VS",
		 info);
	fprintf (stderr, "Shader source:\n%s", source);
	fprintf (stderr, "GLSL compile failure\n");

	glDeleteObjectARB (shader);

	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    *shader_out = shader;

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_gl_load_glsl (GLint *shader_out,
		     const char *vs_source, const char *fs_source)
{
    GLint ok;
    GLint shader, vs, fs;
    cairo_status_t status;

    shader = glCreateProgramObjectARB ();

    status = _cairo_gl_compile_glsl (GL_VERTEX_SHADER_ARB, &vs, vs_source);
    if (_cairo_status_is_error (status))
	goto fail;
    status = _cairo_gl_compile_glsl (GL_FRAGMENT_SHADER_ARB, &fs, fs_source);
    if (_cairo_status_is_error (status))
	goto fail;

    glAttachObjectARB (shader, vs);
    glAttachObjectARB (shader, fs);
    glLinkProgram (shader);
    glGetObjectParameterivARB (shader, GL_OBJECT_LINK_STATUS_ARB, &ok);
    if (!ok) {
	GLchar *info;
	GLint size;

	glGetObjectParameterivARB (shader, GL_OBJECT_INFO_LOG_LENGTH_ARB,
				   &size);
	info = malloc (size);

	if (info)
	    glGetInfoLogARB (shader, size, NULL, info);
	fprintf (stderr, "Failed to link: %s\n", info);
	free (info);
	status = CAIRO_INT_STATUS_UNSUPPORTED;

	return CAIRO_INT_STATUS_UNSUPPORTED;
    }

    *shader_out = shader;

    return CAIRO_STATUS_SUCCESS;

fail:
    glDeleteObjectARB (shader);
    return status;
}
