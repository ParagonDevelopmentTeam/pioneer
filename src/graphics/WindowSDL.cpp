// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "WindowSDL.h"

#include "SDL.h"
#include "SDLWrappers.h"
#include "OS.h"
#include <iostream>

namespace Graphics {

bool WindowSDL::CreateWindowAndContext(const char *name, int w, int h, bool fullscreen, int samples, int depth_bits) {
	Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	if (fullscreen) winFlags |= SDL_WINDOW_FULLSCREEN;

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth_bits);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	int major, minor;
	if (glewIsSupported("GL_VERSION_3_2")) {
		major = 3;
		minor = 2;
	} else if(glewIsSupported("GL_VERSION_3_1")) {
		major = 3;
		minor = 1;
	} else {
		major = 2;
		minor = 1;
	}
	std::cout<<"Creating context for OpenGL "<<major<<"."<<minor<<std::endl;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#ifndef NDEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	// need full 32-bit color
	// (need an alpha channel because of the way progress bars are drawn)
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	m_window = SDL_CreateWindow(name, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, winFlags);
	if (!m_window)
		return false;

	m_glContext = SDL_GL_CreateContext(m_window);
	if (!m_glContext) {
		SDL_DestroyWindow(m_window);
		m_window = 0;
		return false;
	}

	return true;
}

WindowSDL::WindowSDL(const Graphics::Settings &vs, const std::string &name)
{
	bool ok;

	// attempt sequence is:
	// 1- requested mode
	ok = CreateWindowAndContext(name.c_str(), vs.width, vs.height, vs.fullscreen, vs.requestedSamples, 24);

	// 2- requested mode with no anti-aliasing (skipped if no AA was requested anyway)
	//    (skipped if no AA was requested anyway)
	if (!ok && vs.requestedSamples) {
		Output("Failed to set video mode. (%s). Re-trying without multisampling.\n", SDL_GetError());
		ok = CreateWindowAndContext(name.c_str(), vs.width, vs.height, vs.fullscreen, 0, 24);
	}

	// 3- requested mode with 16 bit depth buffer
	if (!ok) {
		Output("Failed to set video mode. (%s). Re-trying with 16-bit depth buffer\n", SDL_GetError());
		ok = CreateWindowAndContext(name.c_str(), vs.width, vs.height, vs.fullscreen, vs.requestedSamples, 16);
	}

	// 4- requested mode with 16-bit depth buffer and no anti-aliasing
	//    (skipped if no AA was requested anyway)
	if (!ok && vs.requestedSamples) {
		Output("Failed to set video mode. (%s). Re-trying with 16-bit depth buffer and no multisampling\n", SDL_GetError());
		ok = CreateWindowAndContext(name.c_str(), vs.width, vs.height, vs.fullscreen, 0, 16);
	}

	// 5- abort!
	if (!ok) {
		Error("Failed to set video mode: %s", SDL_GetError());
	}

	SDLSurfacePtr surface = LoadSurfaceFromFile(vs.iconFile);
	if (surface)
		SDL_SetWindowIcon(m_window, surface.Get());

	SDL_SetWindowTitle(m_window, vs.title);
}

WindowSDL::~WindowSDL()
{
	SDL_GL_DeleteContext(m_glContext);
	SDL_DestroyWindow(m_window);
}

int WindowSDL::GetWidth() const
{
	int w, h;
	SDL_GetWindowSize(m_window, &w, &h);
	return w;
}

int WindowSDL::GetHeight() const
{
	int w, h;
	SDL_GetWindowSize(m_window, &w, &h);
	return h;
}

void WindowSDL::SetGrab(bool grabbed)
{
	SDL_SetWindowGrab(m_window, SDL_bool(grabbed));
	SDL_SetRelativeMouseMode(SDL_bool(grabbed));
}

void WindowSDL::SwapBuffers()
{
	SDL_GL_SwapWindow(m_window);
}

}
