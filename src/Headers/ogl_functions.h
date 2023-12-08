#pragma once

#ifdef __SWITCH__
#include <GL/glew.h>
#else
#include <SDL_opengl.h>
#endif

extern PFNGLACTIVETEXTUREARBPROC			procptr_glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC		procptr_glClientActiveTextureARB;

#ifndef __SWITCH__
#define glActiveTextureARB					procptr_glActiveTextureARB
#define glClientActiveTextureARB			procptr_glClientActiveTextureARB
#endif

void OGL_InitFunctions(void);
