#include <grend/loadScene.hpp>
#include <grend/ecs/shader.hpp>

#include "player.hpp"
#include "enemy.hpp"

#include <logic/scancodes.hpp>
#include <entities/velocityParticles.hpp>

#include <components/timedLifetime.hpp>
#include <components/playerInfo.hpp>

player::~player() {};

static gameImport::ptr playerModel = nullptr;
static channelBuffers_ptr throw_sfx = nullptr;
static channelBuffers_ptr hit_sfx = nullptr;

/*
static std::array animationPaths {
	Entry {"walking", DEMO_PREFIX "assets/obj/animations/humanoid/run-forward.glb"},
	Entry {"idle",    DEMO_PREFIX "assets/obj/animations/humanoid/idle.glb"},
	Entry {"left",    DEMO_PREFIX "assets/obj/animations/humanoid/run-forward.glb"},
	Entry {"right",   DEMO_PREFIX "assets/obj/animations/humanoid/run-forward.glb"},
	Entry {"back",    DEMO_PREFIX "assets/obj/animations/humanoid/run-forward.glb"},
	Entry {"falling", DEMO_PREFIX "assets/obj/animations/humanoid/falling.glb"},
};
*/

#define ANIM_PREFIX "assets/obj/animations/humanoid/"
static std::array animationPaths {
	Entry {"walking",    DEMO_PREFIX ANIM_PREFIX "run-forward.glb"},
	Entry {"idle",       DEMO_PREFIX ANIM_PREFIX "idle.glb"},
	Entry {"left",       DEMO_PREFIX ANIM_PREFIX "run-forward.glb"},
	Entry {"right",      DEMO_PREFIX ANIM_PREFIX "run-forward.glb"},
	Entry {"back",       DEMO_PREFIX ANIM_PREFIX "run-forward.glb"},
	Entry {"falling",    DEMO_PREFIX ANIM_PREFIX "falling.glb"},
	Entry {"punch-left",  DEMO_PREFIX ANIM_PREFIX "punch-left.glb"},
	Entry {"punch-right", DEMO_PREFIX ANIM_PREFIX "punch-right.glb"},
	Entry {"kick",        DEMO_PREFIX ANIM_PREFIX "kick.glb"},
};

static loadedAnims playerAnims;

player::player(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager)
{
	attach<health>();
	attach<playerInfo>();
	attach<wieldedHandler>();
	attach<movementHandler>();
	attach<projectileCollision>();
	attach<syncRigidBodyPosition>();
	attach<UnlitShader>();
	rigidBody *body = attach<rigidBodySphere>(position, 10, 0.75);

	manager->registerComponent(this, this);
	manager->registerInterface<updatable>(this, this);

	if (!playerModel) {
		playerAnims = loadAnimations(animationPaths);

		// TODO: resource cache
		SDL_Log("Loading player model...");
		if (auto res = loadSceneCompiled(DEMO_PREFIX "assets/obj/trooper.glb")) {
			playerModel = *res;

		} else {
			printError(res);
			return;
		}

		playerModel->setPosition(glm::vec3(0, -0.75, 0));
		playerModel->setRotation(glm::quat(glm::vec3(0, -M_PI/2, 0)));

		if (!throw_sfx || !hit_sfx) {
			throw_sfx = openAudio(DEMO_PREFIX "assets/sfx/throw.wav.ogg");
			hit_sfx   = openAudio(DEMO_PREFIX "assets/sfx/hit.wav.ogg");
		}
	}

	auto temp = std::static_pointer_cast<gameImport>(duplicate(playerModel));
	character = std::make_shared<animationController>(temp);
	setNode("model", node, temp);

	/*
	auto lit = std::make_shared<gameLightSpot>();

	lit->setPosition({0, 0.5, 1});
	lit->setRotation(glm::quat(glm::vec3(0, -M_PI/2, 0)));
	lit->intensity = 125;
	lit->is_static = false;
	lit->casts_shadows = true;
	setNode("spotlight", node, lit);
	*/

	for (const auto& [name, obj] : playerAnims) {
		auto& anim = obj->animations->begin()->second;
		character->bind(name, anim);
	}

	body->registerCollisionQueue(manager->collisions);
	auto [name, _] = *playerModel->animations->begin();

	character->setAnimation(name);
}

nlohmann::json player::serialize(entityManager *manager) {
	return entity::serialize(manager);
}

void player::update(entityManager *manager, float delta) {
	rigidBody *body = getComponent<rigidBody>(manager, this);
	if (!body) return;

	body->phys->setAngularFactor(0.f);

	if (health *hp = get<health>()) {
		if (hp->amount <= 0) {
			manager->remove(this);
		}
	}

	glm::vec3 vel = body->phys->getVelocity();
	if (stateRemain <= 0) {
		if (stateRemain < 0) {
			glm::vec3 ownPos = node->getTransformTRS().position;
			auto& enemies = manager->getComponents<enemy>();

			float range = 0;

			if (character->getCurrentAnimation() == "kick")        range = 5.0;
			if (character->getCurrentAnimation() == "punch-left")  range = 3;
			if (character->getCurrentAnimation() == "punch-right") range = 3;

			for (component *comp : enemies) {
				enemy *foo = static_cast<enemy*>(comp);

				glm::vec3 enPos = foo->node->getTransformTRS().position;
				glm::vec3 diff = enPos - ownPos;
				glm::vec3 dir = glm::normalize(diff);
				glm::vec3 pdir = node->getTransformTRS().rotation * glm::vec3(1, 0, 0);

				if (glm::length(diff) < range && glm::dot(dir, pdir) > 0.71) {
					if (rigidBody *body = foo->get<rigidBody>()) {

						body->phys->setVelocity(dir*10.f + glm::vec3 {0, 2, 0});
						//body->phys->setVelocity(dir*35.f);
					}

					if (health *hp = foo->get<health>()) {
						hp->damage(11);
					}

					glm::vec3 bpos = enPos + glm::vec3(0, 1, 0);
					entity *parts = new velocityParticles(manager, bpos, dir, 10, 0.8);
					manager->add(parts);
					parts->attach<timedLifetime>(4);

					// XXX
					foo->stateRemain = 2.f;
					foo->character->setAnimation("fallen");
					foo->character->setSpeed(1.0);
					foo->character->resetTime();

					auto ch = std::make_shared<stereoAudioChannel>(hit_sfx);
					ch->worldPosition = enPos;
					manager->engine->audio->add(ch);
				}
			}

			stateRemain = 0;
		}

		uint32_t buttons = SDL_GetMouseState(nullptr, nullptr);
		bool left  = toggleScancode(SDL_SCANCODE_Q) || (buttons & SDL_BUTTON_LMASK);
		bool right = toggleScancode(SDL_SCANCODE_E) || (buttons & SDL_BUTTON_RMASK);

		if (left){
			static bool toggle = false;
			toggle = !toggle;

			character->setAnimation(toggle? "punch-left" : "punch-right");
			character->setSpeed(2.0);
			character->resetTime();
			stateRemain = 0.20f;

			auto ch = std::make_shared<stereoAudioChannel>(throw_sfx);
			ch->worldPosition = node->getTransformTRS().position;
			manager->engine->audio->add(ch);

		//} else if (toggleScancode(SDL_SCANCODE_E)) {
		} else if (right) {
			character->setAnimation("kick");
			character->setSpeed(0.6);
			character->resetTime();
			stateRemain = 0.4;

			auto ch = std::make_shared<stereoAudioChannel>(throw_sfx);
			ch->worldPosition = node->getTransformTRS().position;
			manager->engine->audio->add(ch);

		} else if (glm::length(vel) < 2.0) {
			character->setAnimation("idle");
			character->setSpeed(1.0);

		} else {
			glm::mat4 rot = glm::mat4_cast(node->getTransformTRS().rotation);
			glm::vec4 forwardv = rot * glm::vec4( 1, 0, 0, 0);
			glm::vec4 backv    = rot * glm::vec4(-1, 0, 0, 0);
			glm::vec4 leftv    = rot * glm::vec4( 0, 0,-1, 0);
			glm::vec4 rightv   = rot * glm::vec4( 0, 0, 1, 0);

			glm::vec3 normvel = glm::normalize(vel);
			glm::vec3 forward = glm::vec3(forwardv);
			glm::vec3 back = glm::vec3(backv);
			glm::vec3 left = glm::vec3(leftv);
			glm::vec3 right = glm::vec3(rightv);
			glm::vec3 down = glm::vec3(0, -1, 0);

			float max = -1;
			std::string anim = "walking";

			auto test = [&] (const glm::vec3& vec) {
				float dot = glm::dot(vec, normvel);

				if (dot > max) {
					max = dot;
					return true;
				}

				return false;
			};

			if (test(forward)) anim = "walking";
			if (test(back))    anim = "back";
			if (test(left))    anim = "left";
			if (test(right))   anim = "right";
			if (test(down))    anim = "falling";

			character->setAnimation(anim);
			character->setSpeed(glm::length(vel) / 16.0);
		}

	} else {
		stateRemain -= delta;

	}

	character->update(delta);
}
