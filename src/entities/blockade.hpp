#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/modalSDLInput.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/collision.hpp>

using namespace grendx;
using namespace grendx::ecs;

class blockade : public entity {
	public:
		blockade(entityManager *manager, const glm::vec3& position);
		virtual ~blockade();
};
