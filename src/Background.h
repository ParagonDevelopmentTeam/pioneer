// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _BACKGROUND_H
#define _BACKGROUND_H

#include "libs.h"
#include "galaxy/SystemPath.h"
#include "graphics/Texture.h"

namespace Graphics {
	class Renderer;
	class StaticMesh;
	class Material;
}

/*
 * Classes to draw background stars and the milky way
 */

namespace Background
{
	class BackgroundElement
	{
	public:
		virtual void SetIntensity(float intensity);

	protected:
		RefCountedPtr<Graphics::Material> m_material;
	};

	class UniverseBox : public BackgroundElement
	{
	public:
		UniverseBox(Graphics::Renderer *r);
		~UniverseBox();

		void Draw(Graphics::Renderer *r);
		void LoadCubeMap(Graphics::Renderer *r, Random* randomizer = nullptr);

		virtual void SetIntensity(float intensity) override;

	private:
		void Init(Graphics::Renderer *);
		Random createRandom(Uint32 seed);
		Random createRandom(const SystemPath& system_path);

		Graphics::StaticMesh *m_model;
		Graphics::Texture* m_cubemap;
		float fIntensity;
	};

	class Starfield : public BackgroundElement
	{
	public:
		//does not Fill the starfield
		Starfield(Graphics::Renderer *r);
		Starfield(Graphics::Renderer *r, Uint32 seed);
		~Starfield();
		void Draw(Graphics::Renderer *r);
		//create or recreate the starfield
		void Fill(Uint32 seed);

	private:
		void Init(Graphics::Renderer *);
		static const int BG_STAR_MAX = 10000;
		Graphics::StaticMesh *m_model;

		//hyperspace animation vertex data
		//allocated when animation starts and thrown away
		//when starfield is destroyed (on exiting hyperspace)
		vector3f *m_hyperVtx;
		Color *m_hyperCol;
	};

	class MilkyWay : public BackgroundElement
	{
	public:
		MilkyWay(Graphics::Renderer*);
		~MilkyWay();
		void Draw(Graphics::Renderer *r);

	private:
		Graphics::StaticMesh *m_model;
	};

	// contains starfield, milkyway, possibly other Background elements
	class Container
	{
	public:
		// default constructor, needs Refresh with proper seed to show starfield
		Container(Graphics::Renderer*);
		Container(Graphics::Renderer*, Uint32 seed);
		void Draw(Graphics::Renderer *r, const matrix4x4d &transform);
		void Refresh(Uint32 seed);

		void SetIntensity(float intensity);

	private:
		MilkyWay m_milkyWay;
		Starfield m_starField;
		UniverseBox m_universeBox;
		bool m_bLoadNewCubemap;
		Uint32 m_uSeed;
	};

} //namespace Background

#endif
