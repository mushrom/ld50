#pragma once

#include <grend/ecs/ecs.hpp>
#include <grend/utility.hpp>

using namespace grendx;
using namespace grendx::ecs;

class health : public component {
	public:
		health(entityManager *manager, entity *ent,
		       float _amount = 1.f, float _hp = 100.f)
			: component(manager, ent),
			  amount(_amount), hp(_hp)
		{
			manager->registerComponent(ent, this);
		}

		health(entityManager *manager, entity *ent, nlohmann::json properties)
			: component(manager, ent, properties)
		{
			manager->registerComponent(ent, this);
		};
		virtual ~health();

		virtual float damage(float damage) {
			amount = max(0.0, amount - damage/hp);
			return amount*hp;
		}

		virtual float decrement(float dec) {
			amount = max(0.0, amount - dec);
			return amount;
		}

		virtual float heal(float points) {
			amount = min(1.0, amount + points/hp);
			return amount*hp;
		}

		virtual float increment(float inc) {
			amount = min(1.0, amount + inc);
			return amount;
		}

		float amount;
		float hp;

		// serialization stuff
		constexpr static const char *serializedType = "health";
		static const nlohmann::json defaultProperties(void) {
			return component::defaultProperties();
		}

		virtual const char *typeString(void) const { return serializedType; };
		virtual nlohmann::json serialize(entityManager *manager) {
			return component::defaultProperties();
		};
};
