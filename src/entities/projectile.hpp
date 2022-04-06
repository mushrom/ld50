#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>

#include <components/health.hpp>

using namespace grendx;
using namespace grendx::ecs;

class projectile : public entity {
	public:
		projectile(entityManager *manager, gameMain *game, glm::vec3 position);

		virtual ~projectile();
		virtual void update(entityManager *manager, float delta);

		// TODO:
		float impactDamage = 25.f;
};

class projectileCollision : public collisionHandler {
	public:
		projectileCollision(entityManager *manager, entity *ent)
			//: collisionHandler(manager, ent, {"projectile"})
			: collisionHandler(manager, ent, {getTypeName<projectile>()})
		{
			manager->registerComponent(ent, this);
		}

		projectileCollision(entityManager *manager,
		                    entity *ent,
		                    nlohmann::json properties);

		virtual ~projectileCollision();

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col)
		{
			std::cerr << "projectile collision!" << std::endl;
			health *entHealth = getComponent<health>(manager, ent);
			projectile *proj  = getComponent<projectile>(manager, other);

			//castEntityComponent(entHealth, manager, ent, "health");
			//castEntityComponent(proj, manager, other, "projectile");

			if (entHealth && proj) {
				float x = entHealth->damage(proj->impactDamage);
				std::cerr << "current health: " << x << std::endl;

				if (x == 0.f) {
					manager->remove(ent);
				}
			}
		};

		// serialization stuff
		constexpr static const char *serializedType = "projectileCollision";

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager); 
};

class projectileDestruct : public collisionHandler {
	public:
		projectileDestruct(entityManager *manager, entity *ent)
			//: collisionHandler(manager, ent, {"projectileCollision"})
			: collisionHandler(manager, ent, {getTypeName<projectileCollision>()})
		{
			manager->registerComponent(ent, this);
		}

		virtual ~projectileDestruct();

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col) {
			std::cerr << "projectile destruct!" << std::endl;
			manager->remove(ent);
		};
};

