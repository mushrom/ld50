#include <entities/amulet.hpp>
#include <components/sceneTree.hpp>
#include <grend/loadScene.hpp>

amuletPickup::~amuletPickup() {};

amuletPickup::amuletPickup(entityManager *manager,
                           gameMain *game,
                           glm::vec3 position)
	: entity(manager)
{
	new areaSphere(manager, this, 2.f);
	manager->registerComponent(this, this);

	/*
	// TODO: resource manager
	static gameObject::ptr amuletModel = nullptr;
	if (!amuletModel) {
		amuletModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/amulet.glb");

		TRS transform = amuletModel->getTransformTRS();
		transform.scale = glm::vec3(2.0);
		amuletModel->setTransform(transform);
	}

	setNode("model", node, amuletModel);
	*/

	attach<sceneTree>(DEMO_PREFIX "assets/obj/amulet.glb");
	node->setPosition(position);
}

void amuletPickup::update(entityManager *manager, float delta) {};
