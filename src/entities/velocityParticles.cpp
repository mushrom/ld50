#include "velocityParticles.hpp"

#include <grend/ecs/shader.hpp>
#include <components/timedLifetime.hpp>

#include <stdlib.h>
static inline float randfloat(void) {
	return float(rand()) / RAND_MAX;
}

static inline glm::vec3 randvec3(void) {
	return glm::vec3 {
		randfloat(),
		randfloat(),
		randfloat()
	};
}


velocityParticles::velocityParticles(entityManager *manager,
                                     glm::vec3 pos,
                                     glm::vec3 dir,
                                     float speed,
                                     float randweight)
	: entity(manager)
{
	static gameObject::ptr model = nullptr;

	manager->registerComponent(this, this);
	manager->registerInterface<updatable>(this, this);

	attach<PBRShader>();

	asdf = pos;
	direction = dir;

	parts = std::make_shared<gameBillboardParticles>(128);
	parts->activeInstances = 128;
	parts->radius = 50.f;
	parts->update();

	for (unsigned i = 0; i < 128; i++) {
		velocities[i] = (direction + randvec3()*randweight) * speed;
		positions[i]  = pos;
	}

	if (!model) {
		auto data = loadSceneCompiled(DEMO_PREFIX "assets/obj/emissive-plane.glb");
		if (data)
			model = *data;
	}

	setNode("model", parts, model);
	setNode("parts", node, parts);
}

velocityParticles::~velocityParticles() {}

void velocityParticles::update(entityManager *manager, float delta) {
	for (unsigned i = 0; i < 128; i++) {
		positions[i] += velocities[i]*delta;
		positions[i].y = max(0.2f, positions[i].y);
		velocities[i].y -= delta*30;
		time += delta;

		if (positions[i].y < 0.3) {
			velocities[i]   *= 0.8;
		}

		parts->positions[i] = glm::vec4(positions[i], 0.5);
	}

	parts->update();
}
