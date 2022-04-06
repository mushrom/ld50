#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

#include <memory>
#include <vector>

#include <components/boxSpawner.hpp>
#include <components/inputHandler.hpp>
#include <logic/animationController.hpp>

using namespace grendx;
using namespace grendx::ecs;

class player : public entity, public updatable {
	public:
		player(entityManager *manager, gameMain *game, glm::vec3 position);
		player(entityManager *manager, entity *ent, nlohmann::json properties);
		virtual ~player();

		virtual void update(entityManager *manager, float delta);
		virtual gameObject::ptr getNode(void) { return node; };

		animationController::ptr character;

		// serialization stuff
		constexpr static const char *serializedType = "player";
		static const nlohmann::json defaultProperties(void) {
			return entity::defaultProperties();
		}

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager); 

		float stateRemain = 0.f;
};
