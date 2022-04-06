#pragma once

#include <grend/gameObject.hpp>
#include <grend/ecs/ecs.hpp>

using namespace grendx;
using namespace grendx::ecs;

class velocityParticles : public entity, public updatable {
	public:
		velocityParticles(entityManager *manager,
		                  glm::vec3 pos,
		                  glm::vec3 dir,
		                  float speed,
		                  float randweight = 0.1);
		virtual ~velocityParticles();
		virtual void update(entityManager *manager, float delta);

		gameBillboardParticles::ptr parts;
		glm::vec3 positions[128];
		glm::vec3 velocities[128];
		float time = 0.0;

		glm::vec3 asdf;
		glm::vec3 direction;
};
