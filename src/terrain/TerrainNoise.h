// Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _TERRAINNOISE_H
#define _TERRAINNOISE_H

#include "Terrain.h"
#include "perlin.h"

namespace TerrainNoise {

	// octavenoise functions return range [0,1] if roughness = 0.5
	inline double octavenoise(const fracdef_t &def, double roughness, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = def.frequency;
		for (int i=0; i<def.octaves; i++) {
			n += octaveAmplitude * noise(jizm*p);
			octaveAmplitude *= roughness;
			jizm *= def.lacunarity;
		}
		return (n+1.0)*0.5;
	}

	inline double river_octavenoise(const fracdef_t &def, double roughness, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = def.frequency;
		for (int i=0; i<def.octaves; i++) {
			n += octaveAmplitude * fabs(noise(jizm*p));
			octaveAmplitude *= roughness;
			jizm *= def.lacunarity;
		}
		return fabs(n);
	}

	inline double ridged_octavenoise(const fracdef_t &def, double roughness, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = def.frequency;
		for (int i=0; i<def.octaves; i++) {
			n += octaveAmplitude * noise(jizm*p);
			octaveAmplitude *= roughness;
			jizm *= def.lacunarity;
		}
		n = 1.0 - fabs(n);
		n *= n;
		return n;
		//return 1.0 - fabs(n);
	}

	inline double billow_octavenoise(const fracdef_t &def, double roughness, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = def.frequency;
		for (int i=0; i<def.octaves; i++) {
			n += octaveAmplitude * noise(jizm*p);
			octaveAmplitude *= roughness;
			jizm *= def.lacunarity;
		}
		return (2.0 * fabs(n) - 1.0)+1.0;
	}

	inline double voronoiscam_octavenoise(const fracdef_t &def, double roughness, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = def.frequency;
		for (int i=0; i<def.octaves; i++) {
			n += octaveAmplitude * noise(jizm*p);
			octaveAmplitude *= roughness;
			jizm *= def.lacunarity;
		}
		return sqrt(10.0 * fabs(n));
	}

	inline double dunes_octavenoise(const fracdef_t &def, double roughness, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = def.frequency;
		for (int i=0; i<3; i++) {
			n += octaveAmplitude * noise(jizm*p);
			octaveAmplitude *= roughness;
			jizm *= def.lacunarity;
		}
		return 1.0 - fabs(n);
	}

	// XXX merge these with their fracdef versions
	inline double octavenoise(int octaves, double roughness, double lacunarity, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = 1.0;
		while (octaves--) {
			n += octaveAmplitude * noise(jizm*p);
			octaveAmplitude *= roughness;
			jizm *= lacunarity;
		}
		return (n+1.0)*0.5;
	}

	inline double river_octavenoise(int octaves, double roughness, double lacunarity, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = 1.0;
		while (octaves--) {
			n += octaveAmplitude * fabs(noise(jizm*p));
			octaveAmplitude *= roughness;
			jizm *= lacunarity;
		}
		return n;
	}

	inline double ridged_octavenoise(int octaves, double roughness, double lacunarity, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = 1.0;
		while (octaves--) {
			n += octaveAmplitude * noise(jizm*p);
			octaveAmplitude *= roughness;
			jizm *= lacunarity;
		}
		n = 1.0 - fabs(n);
		n *= n;
		return n;
	}

	inline double billow_octavenoise(int octaves, double roughness, double lacunarity, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = 1.0;
		while (octaves--) {
			n += octaveAmplitude * noise(jizm*p);
			octaveAmplitude *= roughness;
			jizm *= lacunarity;
		}
		return (2.0 * fabs(n) - 1.0)+1.0;
	}

	inline double voronoiscam_octavenoise(int octaves, double roughness, double lacunarity, const vector3d &p) {
		double n = 0;
		double octaveAmplitude = roughness;
		double jizm = 1.0;
		while (octaves--) {
			n += octaveAmplitude * noise(jizm*p);
			octaveAmplitude *= roughness;
			jizm *= lacunarity;
		}
		return sqrt(10.0 * fabs(n));
	}

	// not really a noise function but no better place for it
	inline vector3d interpolate_color(double n, const vector3d &start, const vector3d &end) {
		n = Clamp(n, 0.0, 1.0);
		return start*(1.0-n) + end*n;
	}

};

// common colours for earthlike worlds
// XXX better way to do this?

#define terrain_colournoise_rock   octavenoise(GetFracDef(0), 0.65, p)
#define terrain_colournoise_rock2  octavenoise(GetFracDef(1), 0.6, p)*0.6*ridged_octavenoise(GetFracDef(0), 0.55, p)
// #define terrain_colournoise_rock3  0.5*ridged_octavenoise(GetFracDef(0), 0.5, p)*voronoiscam_octavenoise(GetFracDef(0), 0.5, p)*ridged_octavenoise(GetFracDef(1), 0.5, p)
// #define terrain_colournoise_rock4  0.5*ridged_octavenoise(GetFracDef(1), 0.5, p)*octavenoise(GetFracDef(1), 0.5, p)*octavenoise(GetFracDef(5), 0.5, p)
#define terrain_colournoise_mud    0.1*voronoiscam_octavenoise(GetFracDef(1), 0.5, p)*octavenoise(GetFracDef(1), 0.5, p) * GetFracDef(5).amplitude
#define terrain_colournoise_sand   ridged_octavenoise(GetFracDef(0), 0.4, p)*dunes_octavenoise(GetFracDef(2), 0.4, p) + 0.1*dunes_octavenoise(GetFracDef(1), 0.5, p)
#define terrain_colournoise_sand2  dunes_octavenoise(GetFracDef(0), 0.6, p)*octavenoise(GetFracDef(4), 0.6, p)
// #define terrain_colournoise_sand3  dunes_octavenoise(GetFracDef(2), 0.6, p)*dunes_octavenoise(GetFracDef(6), 0.6, p)
#define terrain_colournoise_grass  billow_octavenoise(GetFracDef(1), 0.8, p)
#define terrain_colournoise_grass2 billow_octavenoise(GetFracDef(3), 0.6, p)*voronoiscam_octavenoise(GetFracDef(4), 0.6, p)*river_octavenoise(GetFracDef(5), 0.6, p)
#define terrain_colournoise_forest octavenoise(GetFracDef(1), 0.65, p)*voronoiscam_octavenoise(GetFracDef(2), 0.65, p)
#define terrain_colournoise_water  dunes_octavenoise(GetFracDef(6), 0.6, p)


#endif
