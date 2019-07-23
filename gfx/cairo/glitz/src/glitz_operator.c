


























#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "glitzint.h"

void
glitz_set_operator (glitz_gl_proc_address_list_t *gl,
		    glitz_operator_t		 op)
{
    switch (op) {
    case GLITZ_OPERATOR_CLEAR:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ZERO, GLITZ_GL_ZERO);
	break;
    case GLITZ_OPERATOR_SRC:
	gl->disable (GLITZ_GL_BLEND);
	break;
    case GLITZ_OPERATOR_DST:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ZERO, GLITZ_GL_ONE);
	break;
    case GLITZ_OPERATOR_OVER:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ONE, GLITZ_GL_ONE_MINUS_SRC_ALPHA);
	break;
    case GLITZ_OPERATOR_OVER_REVERSE:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ONE_MINUS_DST_ALPHA, GLITZ_GL_ONE);
	break;
    case GLITZ_OPERATOR_IN:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_DST_ALPHA, GLITZ_GL_ZERO);
	break;
    case GLITZ_OPERATOR_IN_REVERSE:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ZERO, GLITZ_GL_SRC_ALPHA);
	break;
    case GLITZ_OPERATOR_OUT:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ONE_MINUS_DST_ALPHA, GLITZ_GL_ZERO);
	break;
    case GLITZ_OPERATOR_OUT_REVERSE:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ZERO, GLITZ_GL_ONE_MINUS_SRC_ALPHA);
	break;
    case GLITZ_OPERATOR_ATOP:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_DST_ALPHA, GLITZ_GL_ONE_MINUS_SRC_ALPHA);
	break;
    case GLITZ_OPERATOR_ATOP_REVERSE:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ONE_MINUS_DST_ALPHA, GLITZ_GL_SRC_ALPHA);
	break;
    case GLITZ_OPERATOR_XOR:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ONE_MINUS_DST_ALPHA,
			GLITZ_GL_ONE_MINUS_SRC_ALPHA);
	break;
    case GLITZ_OPERATOR_ADD:
	gl->enable (GLITZ_GL_BLEND);
	gl->blend_func (GLITZ_GL_ONE, GLITZ_GL_ONE);
	break;
    }
}
