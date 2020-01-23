#pragma once

#include <vector>
#include <glm/mat4x4.hpp>

#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Model.h"
#include "Shader.h"

namespace ym
{
	struct DisplayDesc;
	class Renderer
	{
	public:
		Renderer();
		virtual ~Renderer();

		static Renderer* get();

		virtual void resize(unsigned int width, unsigned int height) = 0;

		virtual void init(DisplayDesc& displayDescriptor) = 0;
		virtual void destroy() = 0;

		virtual void beginScene(float r, float g, float b, float a) = 0;
		virtual void endScene() = 0;

		virtual void draw(VertexArray* va, IndexBuffer* ib, Topology topology, Shader* shader) = 0;
		virtual void draw(Model* model, Shader* shader) = 0;

		bool isActive() const;
		void activate();
		void deactivate();

	private:
		bool m_isActive;
	};
}