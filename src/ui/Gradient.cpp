// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Gradient.h"
#include "Context.h"
#include "graphics/Renderer.h"
#include "graphics/VertexArray.h"
#include "graphics/Material.h"
#include "MainMaterial.h"

namespace UI {

Gradient::Gradient(Context *context, const Color &beginColor, const Color &endColor, Direction direction) :
	Single(context), m_beginColor(beginColor), m_endColor(endColor), m_direction(direction)
{
	Graphics::MaterialDescriptor desc;
	desc.vertexColors = true;
	if(Graphics::Hardware::GL3()) {
		m_material.reset(new MainMaterial(GetContext()->GetRenderer(), desc));
	} else {
		m_material.reset(GetContext()->GetRenderer()->CreateMaterial(desc));
	}
}

void Gradient::Draw()
{
	const Point &offset = GetActiveOffset();
	const Point &area = GetActiveArea();

	const float x = offset.x;
	const float y = offset.y;
	const float sx = area.x;
	const float sy = area.y;

	Graphics::VertexArray va(Graphics::ATTRIB_POSITION | Graphics::ATTRIB_DIFFUSE);
	va.Add(vector3f(x,    y,    0.0f), m_beginColor);
	va.Add(vector3f(x,    y+sy, 0.0f), m_direction == HORIZONTAL ? m_beginColor : m_endColor);
	va.Add(vector3f(x+sx, y,    0.0f), m_direction == HORIZONTAL ? m_endColor : m_beginColor);
	va.Add(vector3f(x+sx, y+sy, 0.0f), m_endColor);

	Graphics::Renderer *r = GetContext()->GetRenderer();
	auto renderState = GetContext()->GetSkin().GetAlphaBlendState();
	r->DrawTriangles(&va, renderState, m_material.get(), Graphics::TRIANGLE_STRIP);

	Container::Draw();
}

}
