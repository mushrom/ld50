#pragma once

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>

#include <components/actions/action.hpp>

class Wieldable : public Action {
	// name of the action component type to use when this thing is wielded
	const char *wieldAction;

	public:
		Wieldable(entityManager *manager, entity *ent, const char *action)
			: Action(manager, ent),
			  wieldAction(action)
		{
			manager->registerComponent(ent, this);
		}
		virtual ~Wieldable();

		virtual void action(entityManager *manager, entity *ent) const {
			entity *self = manager->getEntity((component*)this);
			Action *act = getComponent<Action>(manager, self, wieldAction);

			if (act) {
				act->action(manager, ent);

			} else {
				SDL_Log("Wieldable::action(): couldn't find action %s", wieldAction);
			}
		}
};

