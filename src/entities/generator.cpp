#include <grend/geometryGeneration.hpp>
#include <grend/loadScene.hpp>
#include <grend/ecs/shader.hpp>

#include <components/health.hpp>
#include <components/healthbar.hpp>
#include <components/sceneTree.hpp>
#include <entities/enemyCollision.hpp>
#include <entities/projectile.hpp>
#include <entities/player.hpp>

#include "generator.hpp"

generator::generator(entityManager *manager, const glm::vec3& position)
	: entity(manager)
{
	static gameObject::ptr enemyModel = nullptr;

	node->setPosition(position);
	node->setScale(glm::vec3 {0.5});

	manager->registerComponent(this, this);
	attach<health>();
	attach<worldHealthbar>();
	attach<syncRigidBodyXZVelocity>();
	attach<enemyCollision>();
	attach<PBRShader>();
	attach<sceneTree>(DEMO_PREFIX "assets/obj/lightpole.glb");
	auto body = attach<rigidBodySphere>(position, 1.0, 1.0);

	//setNode("model", node, enemyModel);
	body->registerCollisionQueue(manager->collisions);
	body->phys->setAngularFactor(0.0);
}

generator::~generator() {

}
