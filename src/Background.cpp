// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Background.h"
#include "Frame.h"
#include "FileSystem.h"
#include "Game.h"
#include "perlin.h"
#include "Pi.h"
#include "Player.h"
#include "Space.h"
#include "galaxy/StarSystem.h"
#include "graphics/Graphics.h"
#include "graphics/Material.h"
#include "graphics/Renderer.h"
#include "graphics/VertexArray.h"
#include "graphics/TextureBuilder.h"
#include "StringF.h"
#include "graphics/gl3/EffectMaterial.h"
#include "graphics/gl3/Effect.h"

#include <SDL_stdinc.h>
#include <sstream>
#include <iostream>

using namespace Graphics;

namespace
{
	static std::unique_ptr<Graphics::Texture> s_defaultCubeMap;

	static Uint32 GetNumSkyboxes()
	{
		char filename[1024];
		snprintf(filename, sizeof(filename), "textures/cube");
		std::vector<FileSystem::FileInfo> fileList;
		FileSystem::gameDataFiles.ReadDirectory(filename, fileList);

		const char *itemMask = "ub";

		Uint32 num_matching = 0;
		for (std::vector<FileSystem::FileInfo>::const_iterator it = fileList.begin(), itEnd = fileList.end(); it!=itEnd; ++it) {
			if (starts_with((*it).GetName(), itemMask)) {
				++num_matching;
			}
		}
		return num_matching;
	}
};

namespace Background
{

#pragma pack(push, 4)
struct MilkyWayVert {
	vector4f pos;
	Color4ub col;
};
#pragma pack(pop)

void BackgroundElement::SetIntensity(float intensity)
{
	m_material->emissive = Color(intensity*255);
}

Texture* UniverseBox::s_cubeMap = nullptr;
Texture* UniverseBox::s_emptyCube = nullptr;

void UniverseBox::InitEmptyCubemap(Renderer* renderer)
{
	TextureBuilder texture_builder = TextureBuilder::Cube("textures/cube/ub.dds");
	UniverseBox::s_emptyCube = texture_builder.CreateTexture(renderer);
}

UniverseBox::UniverseBox(Graphics::Renderer *renderer)
{
	m_renderer = renderer;
	if(s_emptyCube == nullptr) {
		UniverseBox::InitEmptyCubemap(m_renderer);
	}
	s_cubeMap = s_emptyCube;
	Init();
}

UniverseBox::~UniverseBox()
{
}

void UniverseBox::Init()
{
	// Load cubemap
	TextureBuilder texture_builder = TextureBuilder::Cube("textures/cube/ub0.dds");
	m_cubemap.reset(texture_builder.CreateTexture(m_renderer));

	// Skybox geometry
	const float vp = 1000.0f;
	std::vector<vector4f> skybox_verts = {
		// Top +Y
		vector4f(-vp, vp, vp, 1.0f),
		vector4f(-vp, vp, -vp, 1.0f),
		vector4f(vp, vp, vp, 1.0f),
		vector4f(vp, vp, vp, 1.0f),
		vector4f(-vp, vp, -vp, 1.0f),
		vector4f(vp, vp, -vp, 1.0f),
		// Bottom -Y
		vector4f(-vp, -vp, -vp, 1.0f),
		vector4f(-vp, -vp, vp, 1.0f),
		vector4f(vp, -vp, -vp, 1.0f),
		vector4f(vp, -vp, -vp, 1.0f),
		vector4f(-vp, -vp, vp, 1.0f),
		vector4f(vp, -vp, vp, 1.0f),
		// Front -Z
		vector4f(-vp, vp, -vp, 1.0f),
		vector4f(-vp, -vp, -vp, 1.0f),
		vector4f(vp, vp, -vp, 1.0f),
		vector4f(vp, vp, -vp, 1.0f),
		vector4f(-vp, -vp, -vp, 1.0f),
		vector4f(vp, -vp, -vp, 1.0f),
		// Back +Z
		vector4f(vp, vp, vp, 1.0f),
		vector4f(vp, -vp, vp, 1.0f),
		vector4f(-vp, vp, vp, 1.0f),
		vector4f(-vp, vp, vp, 1.0f),
		vector4f(vp, -vp, vp, 1.0f),
		vector4f(-vp, -vp, vp, 1.0f),
		// Right +X
		vector4f(vp, vp, -vp, 1.0f),
		vector4f(vp, -vp, -vp, 1.0f),
		vector4f(vp, vp, vp, 1.0f),
		vector4f(vp, vp, vp, 1.0f),
		vector4f(vp, -vp, -vp, 1.0f),
		vector4f(vp, -vp, vp, 1.0f),
		// Left -X
		vector4f(-vp, vp, vp, 1.0f),
		vector4f(-vp, -vp, vp, 1.0f),
		vector4f(-vp, vp, -vp, 1.0f),
		vector4f(-vp, vp, -vp, 1.0f),
		vector4f(-vp, -vp, vp, 1.0f),
		vector4f(-vp, -vp, -vp, 1.0f),
	};

	if(Graphics::Hardware::GL3()) {
		Graphics::GL3::EffectDescriptor sb_desc;
		sb_desc.uniforms.push_back("su_ModelViewProjectionMatrix");
		sb_desc.uniforms.push_back("u_viewPosition");
		sb_desc.uniforms.push_back("u_skyboxIntensity");
		sb_desc.uniforms.push_back("texture0");
		sb_desc.vertex_shader = "gl3/skybox.vert";
		sb_desc.fragment_shader = "gl3/skybox.frag";
		m_material.Reset(new Graphics::GL3::EffectMaterial(m_renderer, sb_desc));
	} else {
		Graphics::MaterialDescriptor desc;
		desc.effect = EFFECT_SKYBOX;
		m_material.Reset(m_renderer->CreateMaterial(desc));
	}
	m_material->texture0 = m_cubemap.get();

	Graphics::VertexBufferDesc vbd;
	vbd.attrib[0].semantic = Graphics::ATTRIB_POSITION;
	vbd.attrib[0].format   = Graphics::ATTRIB_FORMAT_FLOAT4;
	vbd.numVertices = skybox_verts.size();
	vbd.usage = Graphics::BUFFER_USAGE_STATIC;

	m_vertexBuffer.reset(m_renderer->CreateVertexBuffer(vbd));

	Uint8* vtxPtr = m_vertexBuffer->Map<Uint8>(Graphics::BUFFER_MAP_WRITE);
	assert(m_vertexBuffer->GetDesc().stride == sizeof(vector4f));
	memcpy(vtxPtr, &skybox_verts[0], sizeof(vector4f) * skybox_verts.size());
	m_vertexBuffer->Unmap();

	SetIntensity(1.0f);
	
	Graphics::RenderStateDesc rsd;
	rsd.blendMode = Graphics::BlendMode::BLEND_SOLID;
	rsd.depthTest = false;
	rsd.depthWrite = false;
	m_cubeRS = m_renderer->CreateRenderState(rsd);

	m_viewPositionId = m_material->GetEffect()->GetUniformID("u_viewPosition");
	m_skyboxIntensityId = m_material->GetEffect()->GetUniformID("u_skyboxIntensity");

	fSkyboxFactor = 0.8f;

	m_material->GetEffect()->SetProgram();
	m_material->GetEffect()->GetUniform(m_viewPositionId).Set(vector4f(0.0f, 0.0f, 0.0f, 0.0f));

	UniverseBox::s_cubeMap = m_cubemap.get();
}

void UniverseBox::Draw()
{
	if(m_cubemap) {
		m_material->GetEffect()->SetProgram();
		m_material->GetEffect()->GetUniform(m_skyboxIntensityId).Set(fIntensity * fSkyboxFactor);
		m_renderer->DrawBuffer(m_vertexBuffer.get(), m_cubeRS, m_material.Get());
	}
}

void UniverseBox::LoadCubeMap(Random* randomizer)
{
	// Clean old texture
	m_cubemap.reset();
	UniverseBox::s_cubeMap = s_emptyCube;
	
	if(randomizer) {
		int new_ubox_index = randomizer->Int32(0, 9);
		if(new_ubox_index >= 0) {
			// Load new one
			std::ostringstream os;
			os << "textures/cube/ub" << (new_ubox_index) << ".dds";
			TextureBuilder texture_builder = TextureBuilder::Cube(os.str().c_str());
			m_cubemap.reset(texture_builder.CreateTexture(m_renderer));
			UniverseBox::s_cubeMap = m_cubemap.get();
			m_material->texture0 = m_cubemap.get();
		}
	}
}

void UniverseBox::SetIntensity(float intensity)
{
	fIntensity = intensity;
	m_material->specialParameter0 = &fIntensity;
}

Starfield::Starfield(Graphics::Renderer *renderer, Random &rand)
{
	m_renderer = renderer;
	Init();
	Fill(rand);
}

void Starfield::Init()
{
	if (Graphics::Hardware::GL3()) {
		Graphics::GL3::EffectDescriptor sf_desc;
		sf_desc.uniforms.push_back("su_ModelViewProjectionMatrix");
		sf_desc.uniforms.push_back("material.emission");
		sf_desc.vertex_shader = "gl3/starfield.vert";
		sf_desc.fragment_shader = "gl3/starfield.frag";
		m_material.Reset(new Graphics::GL3::EffectMaterial(m_renderer, sf_desc));
	} else {
		Graphics::MaterialDescriptor desc;
		desc.effect = Graphics::EFFECT_STARFIELD;
		desc.vertexColors = true;
		m_material.Reset(m_renderer->CreateMaterial(desc));
	}
	m_material->emissive = Color::WHITE;

}

void Starfield::Fill(Random &rand)
{
	Graphics::VertexBufferDesc vbd;
	vbd.attrib[0].semantic = Graphics::ATTRIB_POSITION;
	vbd.attrib[0].format = Graphics::ATTRIB_FORMAT_FLOAT4;
	vbd.attrib[1].semantic = Graphics::ATTRIB_DIFFUSE;
	vbd.attrib[1].format = Graphics::ATTRIB_FORMAT_UBYTE4;
	vbd.usage = Graphics::BUFFER_USAGE_STATIC;
	vbd.numVertices = BG_STAR_MAX;
	m_vertexBuffer.reset(m_renderer->CreateVertexBuffer(vbd));
	vbd.numVertices = BG_STAR_MAX * 3;
	vbd.usage = Graphics::BUFFER_USAGE_DYNAMIC;
	m_hyperVB.reset(m_renderer->CreateVertexBuffer(vbd));

	assert(sizeof(StarVert) == 20);
	assert(m_vertexBuffer->GetDesc().stride == sizeof(StarVert));
	auto vtxPtr = m_vertexBuffer->Map<StarVert>(Graphics::BUFFER_MAP_WRITE);
	auto hyperPtr = m_hyperVB->Map<StarVert>(Graphics::BUFFER_MAP_WRITE);
	//fill the array
	for (int i=0; i<BG_STAR_MAX; i++) {
		Uint8 col = rand.Double(0.2,0.7) * 255.0;

		// this is proper random distribution on a sphere's surface
		const float theta = float(rand.Double(0.0, 2.0*M_PI));
		const float u = float(rand.Double(-1.0, 1.0));

		vtxPtr->pos = vector4f(
			1000.0f * sqrt(1.0f - u*u) * cos(theta),
			1000.0f * u,
			1000.0f * sqrt(1.0f - u*u) * sin(theta),
			1.0f);
		vtxPtr->col = Color(col, col, col,	255);

		//need to keep data around for HS anim - this is stupid
		hyperPtr[BG_STAR_MAX * 2 + i].pos = vtxPtr->pos;
		hyperPtr[BG_STAR_MAX * 2 + i].col = vtxPtr->col;

		vtxPtr++;
	}
	m_vertexBuffer->Unmap();
	m_hyperVB->Unmap();
}

void Starfield::Draw(Graphics::RenderState *rs)
{
	// XXX would be nice to get rid of the Pi:: stuff here
	if (!Pi::game || Pi::player->GetFlightState() != Ship::HYPERSPACE) {
		m_renderer->DrawBuffer(m_vertexBuffer.get(), rs, m_material.Get(), Graphics::POINTS);
	} else {
		// roughly, the multiplier gets smaller as the duration gets larger.
		// the time-looking bits in this are completely arbitrary - I figured
		// it out by tweaking the numbers until it looked sort of right
		double mult = 0.0015 / (Pi::player->GetHyperspaceDuration() / (60.0*60.0*24.0*7.0));
		double hyperspaceProgress = Pi::game->GetHyperspaceProgress();

		vector3d pz3 = Pi::player->GetOrient().VectorZ();
		vector4f pz(pz3.x*hyperspaceProgress*mult, pz3.y*hyperspaceProgress*mult, pz3.z*hyperspaceProgress*mult, 1.0);
		
		auto hyperPtr = m_hyperVB->Map<StarVert>(Graphics::BUFFER_MAP_WRITE);
		for (int i=0; i<BG_STAR_MAX; i++) {
			vector4f v = hyperPtr[BG_STAR_MAX * 2 + i].pos + pz;
			const Color &c = hyperPtr[BG_STAR_MAX * 2 + i].col;
			
			hyperPtr[i*2].pos = hyperPtr[BG_STAR_MAX * 2 + i].pos + v;
			hyperPtr[i*2].col = c;

			hyperPtr[i*2+1].pos = v;
			hyperPtr[i*2+1].col = c;
		}
		m_hyperVB->Unmap();

		m_renderer->DrawLinesBuffer(BG_STAR_MAX * 2, m_hyperVB.get(), rs);
	}
}

MilkyWay::MilkyWay(Graphics::Renderer *renderer)
{
	m_renderer = renderer;

	//build milky way model in two strips (about 256 verts)
	std::unique_ptr<Graphics::VertexArray> bottom(new VertexArray(ATTRIB_POSITION | ATTRIB_DIFFUSE));
	std::unique_ptr<Graphics::VertexArray> top(new VertexArray(ATTRIB_POSITION | ATTRIB_DIFFUSE));

	const Color dark(0);
	const Color bright(13, 13, 13, 13);

	//bottom
	float theta;
	for (theta=0.0; theta < 2.f*float(M_PI); theta+=0.1f) {
		bottom->Add(
				vector3f(100.0f*sin(theta), float(-40.0 - 30.0*noise(sin(theta),1.0,cos(theta))), 100.0f*cos(theta)),
				dark);
		bottom->Add(
			vector3f(100.0f*sin(theta), float(5.0*noise(sin(theta),0.0,cos(theta))), 100.0f*cos(theta)),
			bright);
	}
	theta = 2.f*float(M_PI);
	bottom->Add(
		vector3f(100.0f*sin(theta), float(-40.0 - 30.0*noise(sin(theta),1.0,cos(theta))), 100.0f*cos(theta)),
		dark);
	bottom->Add(
		vector3f(100.0f*sin(theta), float(5.0*noise(sin(theta),0.0,cos(theta))), 100.0f*cos(theta)),
		bright);
	//top
	for (theta=0; theta < 2.f*float(M_PI); theta+=0.1f) {
		top->Add(
			vector3f(100.0f*sin(theta), float(5.0*noise(sin(theta),0.0,cos(theta))), 100.0f*cos(theta)),
			bright);
		top->Add(
			vector3f(100.0f*sin(theta), float(40.0 + 30.0*noise(sin(theta),-1.0,cos(theta))), 100.0f*cos(theta)),
			dark);
	}
	theta = 2.f*float(M_PI);
	top->Add(
		vector3f(100.0f*sin(theta), float(5.0*noise(sin(theta),0.0,cos(theta))), 100.0f*cos(theta)),
		bright);
	top->Add(
		vector3f(100.0f*sin(theta), float(40.0 + 30.0*noise(sin(theta),-1.0,cos(theta))), 100.0f*cos(theta)),
		dark);

	if(Graphics::Hardware::GL3()) {
		Graphics::GL3::EffectDescriptor sf_desc;
		sf_desc.uniforms.push_back("su_ModelViewProjectionMatrix");
		sf_desc.uniforms.push_back("material.emission");
		sf_desc.vertex_shader = "gl3/starfield.vert";
		sf_desc.fragment_shader = "gl3/starfield.frag";
		m_material.Reset(new Graphics::GL3::EffectMaterial(renderer, sf_desc));
	} else {
		Graphics::MaterialDescriptor desc;
		desc.effect = Graphics::EFFECT_STARFIELD;
		desc.vertexColors = true;
		m_material.Reset(m_renderer->CreateMaterial(desc));
	}
	m_material->emissive = Color::WHITE;

	Graphics::VertexBufferDesc vbd;
	vbd.attrib[0].semantic = Graphics::ATTRIB_POSITION;
	vbd.attrib[0].format = Graphics::ATTRIB_FORMAT_FLOAT4;
	vbd.attrib[1].semantic = Graphics::ATTRIB_DIFFUSE;
	vbd.attrib[1].format = Graphics::ATTRIB_FORMAT_UBYTE4;
	vbd.numVertices = bottom->GetNumVerts() + top->GetNumVerts();
	vbd.usage = Graphics::BUFFER_USAGE_STATIC;

	//two strips in one buffer, but seems to work ok without degenerate triangles
	m_vertexBuffer.reset(renderer->CreateVertexBuffer(vbd));
	assert(m_vertexBuffer->GetDesc().stride == sizeof(MilkyWayVert));
	auto vtxPtr = m_vertexBuffer->Map<MilkyWayVert>(Graphics::BUFFER_MAP_WRITE);
	for (Uint32 i = 0; i < top->GetNumVerts(); i++) {
		vtxPtr->pos = top->position[i];
		vtxPtr->col = top->diffuse[i];
		vtxPtr++;
	}
	for (Uint32 i = 0; i < bottom->GetNumVerts(); i++) {
		vtxPtr->pos = bottom->position[i];
		vtxPtr->col = bottom->diffuse[i];
		vtxPtr++;
	}
	m_vertexBuffer->Unmap();
}

void MilkyWay::Draw(Graphics::RenderState *rs)
{
	assert(m_vertexBuffer);
	assert(m_material);
	m_renderer->DrawBuffer(m_vertexBuffer.get(), rs, m_material.Get(), Graphics::TRIANGLE_STRIP);
}

Container::Container(Graphics::Renderer *renderer, Random &rand)
: m_renderer(renderer)
, m_bLoadNewCubemap(true)
, m_uSeed(0)
{
	m_milkyWay.reset(new MilkyWay(renderer));
	m_starField.reset(new Starfield(renderer, rand));
	m_universeBox.reset(new UniverseBox(renderer));
	Graphics::RenderStateDesc rsd;
	rsd.depthTest  = false;
	rsd.depthWrite = false;
	m_renderState = renderer->CreateRenderState(rsd);
	Refresh(rand.Int32());
}

Container::Container(Graphics::Renderer *renderer, Uint32 seed)
: m_renderer(renderer)
, m_bLoadNewCubemap(true)
, m_uSeed(seed)
{
	m_milkyWay.reset(new MilkyWay(renderer));
	Random rand(seed);
	m_starField.reset(new Starfield(renderer, rand));
	m_universeBox.reset(new UniverseBox(renderer));
	Graphics::RenderStateDesc rsd;
	rsd.depthTest = false;
	rsd.depthWrite = false;
	m_renderState = renderer->CreateRenderState(rsd);
	Refresh(seed);
};

void Container::Refresh(Uint32 seed)
{
	// always redo starfield, milkyway stays normal for now
	Random rand(seed);
	m_starField->Fill(rand);
	if(m_uSeed != seed) {
		m_bLoadNewCubemap = true;
	}
	m_uSeed = seed;
}

void Container::Draw(const matrix4x4d &transform)
{
	PROFILE_SCOPED()
	m_renderer->SetTransform(transform);

	if(m_bLoadNewCubemap) {
		m_bLoadNewCubemap = false;
		if(Pi::player == nullptr || Pi::player->GetFlightState() != Ship::HYPERSPACE) {
			if(Pi::player && Pi::game->GetSpace()->GetStarSystem()) {
				Uint32 seeds [5];
				const SystemPath& system_path = Pi::game->GetSpace()->GetStarSystem()->GetPath();
				seeds[0] = system_path.systemIndex + 41;
				seeds[1] = system_path.sectorX;
				seeds[2] = system_path.sectorY;
				seeds[3] = system_path.sectorZ;
				seeds[4] = UNIVERSE_SEED;
				Random rand(seeds, 5);
				m_universeBox->LoadCubeMap(&rand);
			} else {
				Random rand(m_uSeed);
				m_universeBox->LoadCubeMap(&rand);
			}
		} else {
			m_universeBox->LoadCubeMap();
		}
	}

	m_renderer->SetTransform(transform);
	m_universeBox->Draw();
	//m_milkyWay->Draw(m_renderState);
	// squeeze the starfield a bit to get more density near horizon
	matrix4x4d starTrans = transform * matrix4x4d::ScaleMatrix(1.0, 0.4, 1.0);
	m_renderer->SetTransform(starTrans);
	m_starField->Draw(m_renderState);
}

void Container::SetIntensity(float intensity)
{
	PROFILE_SCOPED()
	m_universeBox->SetIntensity(intensity);
	m_starField->SetIntensity(intensity);
	m_milkyWay->SetIntensity(intensity);
}

void Container::SetDrawFlags(const Uint32 flags)
{
	//m_drawFlags = flags;
}

} //namespace Background
