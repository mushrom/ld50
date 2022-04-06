#include <grend/geometryGeneration.hpp>
#include <grend/loadScene.hpp>

#include <components/health.hpp>
#include <components/healthbar.hpp>
#include <components/sceneTree.hpp>
#include <grend/ecs/shader.hpp>

#include <entities/projectile.hpp>
#include <entities/player.hpp>
#include <entities/generator.hpp>
#include "enemy.hpp"

static channelBuffers_ptr sfx = nullptr;
static channelBuffers_ptr throw_sfx = nullptr;
static channelBuffers_ptr hit_sfx = nullptr;
static uint32_t counter = 0;

// TODO: seriously for real just do a resource manager already
static std::map<std::string, gameObject::ptr> enemyModels;

enemy::~enemy() {};
noodler::~noodler() {};
bat::~bat() {};

#define ANIM_PREFIX "assets/obj/animations/humanoid/"

static std::array animationPaths {
	Entry {"walking",    DEMO_PREFIX ANIM_PREFIX "run-forward.glb"},
	Entry {"idle",       DEMO_PREFIX ANIM_PREFIX "idle.glb"},
	Entry {"left",       DEMO_PREFIX ANIM_PREFIX "run-forward.glb"},
	Entry {"right",      DEMO_PREFIX ANIM_PREFIX "run-forward.glb"},
	Entry {"back",       DEMO_PREFIX ANIM_PREFIX "run-forward.glb"},
	Entry {"falling",    DEMO_PREFIX ANIM_PREFIX "falling.glb"},
	Entry {"fallen",     DEMO_PREFIX ANIM_PREFIX "fallen.glb"},
	Entry {"punch-left", DEMO_PREFIX ANIM_PREFIX "punch-right.glb"}
};

static loadedAnims playerAnims;

enemy::enemy(entityManager *manager,
		gameMain *game,
		glm::vec3 position,
		std::string modelPath,
		float radius,
		float height,
		float mass)
	: entity(manager)
{
	node->setPosition(position);

	attach<health>();
	attach<worldHealthbar>();
	attach<projectileCollision>();
	attach<syncRigidBodyXZVelocity>();
	attach<PBRShader>();
	rigidBody *body = attach<rigidBodyCapsule>(position, mass, radius, height);

	manager->registerComponent(this, this);
	manager->registerInterface<updatable>(this, this);

	gameObject::ptr model = nullptr;
	auto it = enemyModels.find(modelPath);

	// TODO:
	if (it == enemyModels.end()) {
		playerAnims = loadAnimations(animationPaths);
		if (auto data = loadSceneCompiled(modelPath)) {
			model = *data;
			model->setRotation(glm::quat(glm::vec3(0, M_PI, 0)));
			model->setPosition(glm::vec3(0, -height/2, 0));
			enemyModels.insert({modelPath, model});
		} else printError(data);

	} else {
		model = it->second;
	}

	if (!throw_sfx || !hit_sfx) {
		throw_sfx = openAudio(DEMO_PREFIX "assets/sfx/throw.wav.ogg");
		hit_sfx   = openAudio(DEMO_PREFIX "assets/sfx/hit.wav.ogg");
	}

	/*
	if (!sfx) {
		sfx = openAudio(DEMO_PREFIX "assets/sfx/mnstr7.ogg");
	}
	*/

	gameImport::ptr temp = std::static_pointer_cast<gameImport>(duplicate(model));
	character = std::make_shared<animationController>(temp);

	for (auto& [name, obj] : playerAnims) {
		auto& anim = obj->animations->begin()->second;
		character->bind(name, anim);
	}

	setNode("model", node, temp);
	body->registerCollisionQueue(manager->collisions);
	body->phys->setAngularFactor(0.0);

	xxxid = counter++;
}

#if 0
// commenting this out for now, TODO: redo serialization stuff
// TODO: sync this constructor with the above
enemy::enemy(entityManager *manager, entity *ent, nlohmann::json properties)
	: entity(manager, properties)
{
	static gameObject::ptr enemyModel = nullptr;

	new health(manager, this);
	new worldHealthbar(manager, this);
	new projectileCollision(manager, this);
	new syncRigidBodyXZVelocity(manager, this);
	auto body = new rigidBodyCapsule(manager, this, node->getTransformTRS().position, 1.0, 1.0, 2.0);

	manager->registerComponent(this, this);

	// TODO:
	if (!enemyModel) {
		//enemyModel = loadSceneAsyncCompiled(manager->engine, DEMO_PREFIX "assets/obj/test-enemy.glb");
		//auto [data, _] = loadSceneAsyncCompiled(manager->engine, DEMO_PREFIX "assets/obj/noodler.glb");
		auto data = loadSceneCompiled(DEMO_PREFIX "assets/obj/noodler.glb");
		enemyModel = data;
		sfx = openAudio(DEMO_PREFIX "assets/sfx/mnstr7.ogg");

		animfile  = loadSceneCompiled("/tmp/advdroid/run.glb");
		idlefile  = loadSceneCompiled("/tmp/advdroid/idle.glb");
		leftfile  = loadSceneCompiled("/tmp/advdroid/left.glb");
		rightfile = loadSceneCompiled("/tmp/advdroid/right.glb");
		backfile  = loadSceneCompiled("/tmp/advdroid/back.glb");
		//fallfile  = loadSceneCompiled("/tmp/falling.glb");


		TRS transform = enemyModel->getTransformTRS();
		transform.scale = glm::vec3(0.25);
		enemyModel->setTransform(transform);
	}

	auto& foo   = animfile->animations->begin()->second;
	auto& id    = idlefile->animations->begin()->second;
	auto& left  = leftfile->animations->begin()->second;
	auto& right = rightfile->animations->begin()->second;
	auto& back  = backfile->animations->begin()->second;
	//auto& fall  = fallfile->animations->begin()->second;

	auto temp = enemyModel;
	/*
	gameImport::ptr temp = std::static_pointer_cast<gameImport>(duplicate(enemyModel));
	character = std::make_shared<animationController>(temp);
	character->bind("walking", foo);
	character->bind("idle",    id);
	character->bind("left",    left);
	character->bind("right",   right);
	character->bind("back",    back);
	//character->bind("falling", fall);
	*/

	//setNode("model", node, enemyModel);
	setNode("model", node, temp);
	body->registerCollisionQueue(manager->collisions);
	body->phys->setAngularFactor(0.0);
}
#endif

#include <logic/projalphaView.hpp>
void enemy::update(entityManager *manager, float delta) {
	// TODO: const refs
	glm::vec3 playerPos;
	glm::vec3 ownPos = node->getTransformTRS().position;
	entity *playerEnt = findNearest<player>(manager, ownPos);

	if (playerEnt) {
		playerPos = playerEnt->getNode()->getTransformTRS().position;
	}

	// TODO: should this be a component, a generic chase implementation?
	//       wrapping things in generic "behavior" components could be pretty handy...
	rigidBody *body = getComponent<rigidBody>(manager, this);
	if (!body) return;

	glm::vec3 diff = playerPos - ownPos;

	glm::vec3 vel = body->phys->getVelocity();
	float speed = glm::length(vel);
	float horizSpeed = glm::length(vel * glm::vec3(1, 0, 1));

	if (horizSpeed > 20) {
		character->setAnimation("fallen");
		character->setSpeed(1.0);
		stateRemain = 1.5f;
	}

	if (stateRemain <= 0) {
		{
			glm::vec3 flatvel = glm::normalize(glm::vec3(diff.x, 0, diff.z));
			body->phys->setAcceleration(10.f*flatvel);
		}

		if (stateRemain < 0) {
			if (character->getCurrentAnimation() == "punch-left"
			    && glm::length(diff) < 4)
			{
				glm::vec3 pdir = node->getTransformTRS().rotation * glm::vec3(1, 0, 0);
				glm::vec3 dir = glm::normalize(diff);

				if (glm::dot(dir, pdir) > 0.6) {
					if (health *php = playerEnt->get<health>()) {
						php->damage(5);

						/*
						auto ch = std::make_shared<stereoAudioChannel>(hit_sfx);
						ch->worldPosition = playerPos;
						manager->engine->audio->add(ch);
						*/
					}
				}

				/*
				   if (rigidBody *body = playerEnt->get<rigidBody>()) {
				//glm::vec3 temp = playerEnt->node->getTransformTRS().rotation * glm::vec3(-1, 0, 0);
				body->phys->setVelocity(glm::normalize(diff)*30.f);
				}
				*/

			} else if (character->getCurrentAnimation() == "fallen") {
				if (health *hp = get<health>()) {
					if (hp->amount <= 0) {
						manager->remove(this);
					}
				}
			}

			stateRemain = 0;
		}

		if (speed < 2.0) {
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

			if (glm::length(diff) < 3) {
				anim = "punch-left";
				character->setSpeed(1.0);
				character->resetTime();
				stateRemain = 0.5f;
				/*
				auto ch = std::make_shared<stereoAudioChannel>(throw_sfx);
				ch->worldPosition = node->getTransformTRS().position;
				manager->engine->audio->add(ch);
				*/

			} else {
				if (test(forward)) anim = "walking";
				if (test(back))    anim = "back";
				if (test(left))    anim = "left";
				if (test(right))   anim = "right";
				if (test(down))    anim = "falling";
				character->setSpeed(glm::length(vel) / 8.0);
			}

			character->setAnimation(anim);
		}

	} else {
		stateRemain -= delta;
	}

	character->update(delta);
}

nlohmann::json enemy::serialize(entityManager *manager) {
	return entity::serialize(manager);
}
