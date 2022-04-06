#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/ecs/ecs.hpp>
#include <grend/ecs/collision.hpp>

#include <components/health.hpp>
#include <entities/enemy.hpp>

using namespace grendx;
using namespace grendx::ecs;

class enemyCollision : public collisionHandler {
	float damage;
	float lastCollision = 0;

	public:
		enemyCollision(entityManager *manager, entity *ent, float _damage = 15.f)
			//: collisionHandler(manager, ent, {"enemy"})
			: collisionHandler(manager, ent, {getTypeName<enemy>()})
		{
			damage = _damage;
			manager->registerComponent(ent, this);
		}

		virtual ~enemyCollision();

		virtual void
		onCollision(entityManager *manager, entity *ent,
		            entity *other, collision& col)
		{
			float ticks = SDL_GetTicks() / 1000.f;

			// only take damage once per second
			if (ticks - lastCollision < 0.25) {
				return;
			}

			lastCollision = ticks;
			health *entHealth = ent->get<health>();

			if (entHealth) {
				float x = entHealth->damage(damage);

				if (x == 0.f) {
					manager->remove(ent);
				}
			}
		};
};
