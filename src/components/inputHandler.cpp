#include "inputHandler.hpp"
#include <logic/gameController.hpp>

inputHandler::~inputHandler() {};
rawEventHandler::~rawEventHandler() {};
inputPoller::~inputPoller() {};
inputHandlerSystem::~inputHandlerSystem() {};
controllable::~controllable() {};
isControlled::~isControlled() {};
movementHandler::~movementHandler() {};
mouseRotationPoller::~mouseRotationPoller() {};
touchMovementHandler::~touchMovementHandler() {};
touchRotationHandler::~touchRotationHandler() {};

void inputHandlerSystem::update(entityManager *manager, float delta) {
//	auto handlers  = manager->getComponents("inputHandler");
	// TODO: maybe have seperate system for pollers
	//auto pollers   = manager->getComponents("inputPoller");
	auto handlers = manager->getComponents<inputHandler>();
	auto pollers  = manager->getComponents<inputPoller>();

	for (auto& ev : *inputs) {
		for (auto& it : handlers) {
			inputHandler *handler = static_cast<inputHandler*>(it);
			entity *ent = manager->getEntity(handler);

			if (handler && ent) {
				handler->handleInput(manager, ent, ev);
			}
		}
	}

	for (auto& it : pollers) {
		inputPoller *poller = dynamic_cast<inputPoller*>(it);
		entity *ent = manager->getEntity(poller);

		if (poller && ent) {
			poller->update(manager, ent);
		}
	}

	inputs->clear();
}

void inputHandlerSystem::handleEvent(entityManager *manager, SDL_Event& ev) {
	//auto rawEvents = manager->getComponents("rawEventHandler");
	auto rawEvents = manager->getComponents<rawEventHandler>();

	for (auto& it : rawEvents) {
		rawEventHandler *handler = static_cast<rawEventHandler*>(it);
		entity *ent = manager->getEntity(handler);

		if (handler && ent) {
			handler->handleEvent(manager, ent, ev);
		}
	}
}

void mouseRotationPoller::update(entityManager *manager, entity *ent) {
	glm::vec2 meh = glm::normalize(glm::vec2(cam->direction().x, cam->direction().z));
	float n = 0;

	if (Controller()) {
		int cx = SDL_GameControllerGetAxis(Controller(), SDL_CONTROLLER_AXIS_RIGHTX);
		int cy = SDL_GameControllerGetAxis(Controller(), SDL_CONTROLLER_AXIS_RIGHTY);

		n = (abs(cx) > 200 || abs(cy) > 200)? atan2(cx/32768.f, cy/32768.f) : 0;

	} else {
		int x, y, win_x, win_y;

		// TODO: this could be passed as a parameter to avoid calling SDL_*
		//       functions redundantly, don't think it'll be a problem though
		SDL_GetMouseState(&x, &y);
		SDL_GetWindowSize(manager->engine->ctx.window, &win_x, &win_y);

		glm::vec2 pos(x * 1.f/win_x, y * 1.f/win_y);
		glm::vec2 center(0.5);
		glm::vec2 diff = pos - center;

		n = atan2(diff.x, diff.y);
	}

	//glm::quat rot(glm::vec3(0, atan2(diff.x, diff.y) - atan2(meh.x, -meh.y), 0));
	//glm::quat rot(glm::vec3(0, n + atan2(diff.x, diff.y) - atan2(meh.x, -meh.y) - M_PI/2.f, 0));
	glm::quat rot(glm::vec3(0, n - atan2(meh.x, -meh.y) - M_PI/2.f, 0));

	TRS transform = ent->node->getTransformTRS();
	transform.rotation = rot;
	ent->node->setTransform(transform);
}

void touchMovementHandler::handleEvent(entityManager *manager,
                                       entity *ent,
                                       SDL_Event& ev)
{
	if (ev.type == SDL_FINGERMOTION || ev.type == SDL_FINGERDOWN) {
		float x = ev.tfinger.x;
		float y = ev.tfinger.y;
		int wx = manager->engine->rend->screen_x;
		int wy = manager->engine->rend->screen_y;

		glm::vec2 touch(wx * x, wy * y);
		glm::vec2 diff = center - touch;
		float     dist = glm::length(diff) / 150.f;

		if (dist < 1.0) {
			glm::vec3 dir = (cam->direction()*diff.y + cam->right()*diff.x) / 15.f;
			SDL_Log("MOVE: %g (%g,%g,%g)", dist, dir.x, dir.y, dir.z);
			touchpos = -diff;
			inputs->push_back({
				.type = inputEvent::types::move,
				.data = dir,
				.active = true,
			});
		}

		SDL_Log("MOVE: got finger touch, %g (%g, %g)", dist, touch.x, touch.y);
	}
}

void touchRotationHandler::handleEvent(entityManager *manager,
                                       entity *ent,
                                       SDL_Event& ev)
{
	if (ev.type == SDL_FINGERMOTION || ev.type == SDL_FINGERDOWN) {
		static uint32_t last_action = 0;

		float x = ev.tfinger.x;
		float y = ev.tfinger.y;
		int wx = manager->engine->rend->screen_x;
		int wy = manager->engine->rend->screen_y;

		glm::vec2 touch(wx * x, wy * y);
		glm::vec2 diff = center - touch;
		float     dist = glm::length(diff) / 150.f;
		uint32_t  ticks = SDL_GetTicks();

		if (dist < 1.0) {
			glm::vec3 dir = (cam->direction()*diff.y + cam->right()*diff.x) / 15.f;
			SDL_Log("ACTION: %g (%g,%g,%g)", dist, dir.x, dir.y, dir.z);
			touchpos = -diff;

			glm::quat rot(glm::vec3(0, atan2(touchpos.x, touchpos.y), 0));
			TRS transform = ent->node->getTransformTRS();
			transform.rotation = rot;
			ent->node->setTransform(transform);

			if (ticks - last_action > 500) {
				last_action = ticks;
				inputs->push_back({
					.type = inputEvent::types::primaryAction,
					.data = glm::normalize(dir),
					.active = true,
				});
			}
		}

		SDL_Log("ACTION: got finger touch, %g (%g, %g)", dist, touch.x, touch.y);
	}
}

#include <logic/scancodes.hpp>

bindFunc camMovement2D(inputQueue q, camera::ptr cam, float accel) {
	auto curdir = std::make_shared<glm::vec3>(0);
	auto pressed = std::make_shared<std::map<int, glm::vec3>>();

	return [=] (SDL_Event& ev, unsigned flags) {
		glm::vec3 dir, right, up;
		bool any = false;

		dir   = glm::normalize(cam->direction() * glm::vec3(1, 0, 1));
		right = glm::normalize(cam->right() * glm::vec3(1, 0, 1));
		up    = glm::vec3(0, 1, 0);

		glm::vec3 evdir(0);
		float asdf[4];

		asdf[0] = scancodePressed(SDL_SCANCODE_W);
		asdf[1] = scancodePressed(SDL_SCANCODE_S);
		asdf[2] = scancodePressed(SDL_SCANCODE_A);
		asdf[3] = scancodePressed(SDL_SCANCODE_D);

		evdir =    dir*asdf[0]
		      +   -dir*asdf[1]
		      +  right*asdf[2]
		      + -right*asdf[3];

		if (evdir != glm::vec3 {0}) {
			evdir = glm::normalize(evdir);
		}
		/*
		if (toggleScancode(SDL_SCANCODE_W)) evdir +=  dir;
		if (toggleScancode(SDL_SCANCODE_S)) evdir += -dir;
		if (toggleScancode(SDL_SCANCODE_A)) evdir +=  right;
		if (toggleScancode(SDL_SCANCODE_D)) evdir += -right;
		*/

		if (*curdir != evdir) {
			*curdir = evdir;
			any = true;
		}

#if 0
		if (ev.type == SDL_KEYDOWN && !pressed->count(ev.key.keysym.sym)) {
			/*
			glm::vec3 evdir(0);

			switch (ev.key.keysym.sym) {
				case SDLK_w:     evdir =  dir; break;
				case SDLK_s:     evdir = -dir; break;
				case SDLK_a:     evdir =  right; break;
				case SDLK_d:     evdir = -right; break;
				//case SDLK_q:     evdir =  up; break;
				//case SDLK_e:     evdir = -up; break;
				//case SDLK_SPACE: evdir =  up; break;
				default: break;
			};

			pressed->insert({ev.key.keysym.sym, evdir});
			*curdir += evdir;
			any = true;
			*/

		} else if (ev.type == SDL_KEYUP && pressed->count(ev.key.keysym.sym)) {
			/*
			auto it = pressed->find(ev.key.keysym.sym);
			auto& [_, evdir] = *it;

			pressed->erase(ev.key.keysym.sym);
			*curdir -= evdir;
			any = true;
			*/

			/*
		} else if (ev.type == SDL_CONTROLLERBUTTONDOWN) {
			glm::vec3 evdir(0);

			switch (ev.cbutton.button) {
				case SDL_CONTROLLER_BUTTON_DPAD_UP:
					evdir =  dir;
					break;
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
					evdir = -dir;
					break;
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
					evdir =  right;
					break;
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
					evdir = -right;
					break;
				default:
					break;
			}

			pressed->insert({ev.cbutton.button, evdir});
			*curdir += evdir;
			any = true;

		} else if (ev.type == SDL_CONTROLLERBUTTONUP) {
			auto it = pressed->find(ev.cbutton.button);
			auto& [_, evdir] = *it;

			pressed->erase(ev.cbutton.button);
			*curdir -= evdir;
			any = true;
		*/
		}

		else
#endif
		if (ev.type == SDL_CONTROLLERAXISMOTION) {
			if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
			}

			if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
			}

			int cx = SDL_GameControllerGetAxis(Controller(), SDL_CONTROLLER_AXIS_LEFTX);
			int cy = SDL_GameControllerGetAxis(Controller(), SDL_CONTROLLER_AXIS_LEFTY);
			//uint32_t  ticks = SDL_GetTicks();
			//

			float x = cx / 32768.f;
			float y = cy / 32768.f;
			glm::vec2 diff(x, y);

			glm::vec2 meh = glm::normalize(glm::vec2(cam->direction().x, cam->direction().z));
			glm::quat rot(glm::vec3(0, atan2(diff.x, diff.y) - atan2(meh.x, -meh.y) - M_PI/2.f, 0));

			/*
			if (ticks - last_action > 500) {
				last_action = ticks;
				inputs->push_back({
					.type = inputEvent::types::primaryAction,
					.data = glm::normalize(dir),
					.active = true,
				});
			}
			*/

			if (abs(cx) > 1000 || abs(cy) > 1000) {
				*curdir = glm::mat3_cast(rot) * glm::vec3(1, 0, 0) * glm::length(diff);
			} else {
				*curdir = glm::vec3(0);
			}

			any = true;
		}

		//cam->setVelocity(*curdir);
		if (any) {
			q->push_back({
				.type = inputEvent::types::move,
				.data = *curdir,
				.active = ev.type == SDL_KEYDOWN,
			});
		}

		return MODAL_NO_CHANGE;
	};
}

bindFunc inputMapper(inputQueue q, camera::ptr cam) {
	return [=] (SDL_Event& ev, unsigned flags) {
		/*
		if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
			switch (ev.key.keysym.sym) {
				case SDLK_w:
				case SDLK_s:
				case SDLK_a:
				case SDLK_d:
				case SDLK_q:
				case SDLK_e:
				case SDLK_SPACE:
					// XXX: just sync movement with camera, will want
					//      to change this
					q->push_back({
						.type = inputEvent::types::move,
						.data = cam->velocity(),
						.active = ev.type == SDL_KEYDOWN,
					});
					break;
			}
		}
		*/

		if (ev.type == SDL_MOUSEBUTTONDOWN || ev.type == SDL_MOUSEBUTTONUP) {
			// down = active (clicked), up = inactive
			bool active = ev.type == SDL_MOUSEBUTTONDOWN;

			switch (ev.button.button) {
				case SDL_BUTTON_LEFT:
					q->push_back({
						.type = inputEvent::types::primaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				case SDL_BUTTON_RIGHT:
					q->push_back({
						.type = inputEvent::types::secondaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				case SDL_BUTTON_MIDDLE:
					q->push_back({
						.type = inputEvent::types::tertiaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;
			}
		}

		if (ev.type == SDL_CONTROLLERBUTTONDOWN/* || ev.type == SDL_CONTROLLERBUTTONUP*/) {
			bool active = ev.type == SDL_CONTROLLERBUTTONDOWN;

			switch (ev.cbutton.button) {
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: 
					q->push_back({
						.type = inputEvent::types::primaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: 
					q->push_back({
						.type = inputEvent::types::secondaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				case SDL_CONTROLLER_BUTTON_X: 
					q->push_back({
						.type = inputEvent::types::tertiaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				case SDL_CONTROLLER_BUTTON_Y: 
					q->push_back({
						.type = inputEvent::types::primaryAction,
						.data = cam->direction(),
						.active = active,
					});
					break;

				default: break;
			}
		}

		return MODAL_NO_CHANGE;
	};
}
