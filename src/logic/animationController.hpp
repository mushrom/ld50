#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>
#include <grend/loadScene.hpp>

using namespace grendx;

class animationController {
	public:
		typedef std::shared_ptr<animationController> ptr;
		typedef std::weak_ptr<animationController>   weakptr;

		animationController(gameImport::ptr objs);
		void setAnimation(std::string animation, float weight = 1.0);
		gameImport::ptr getObject(void);
		void update(float delta);
		const std::string& getCurrentAnimation(void) {
			static std::string empty = "";
			return currentAnimationName? *currentAnimationName : empty;
		};

		// bind an animation to a name
		// the animation can be from another animation collection
		void bind(std::string name, animationMap::ptr anim);
		void setSpeed(float speed);
		void resetTime(void) { animTime = 0; };

	private:
		float animTime = 0;
		float animSpeed = 1.0;

		animationCollection::ptr animations;
		animationMap::ptr currentAnimation;
		gameImport::ptr objects;
		const std::string *currentAnimationName = nullptr;
};

void applyAnimation(gameObject::ptr node, animationMap::ptr anim, float time);

using Entry = std::pair<std::string, std::string>;
using loadedAnims = std::map<std::string, gameImport::ptr>;

template <size_t N>
loadedAnims loadAnimations(std::array<Entry, N> paths) {
	loadedAnims ret;

	for (const auto& [name, path] : paths) {
		if (auto res = loadSceneCompiled(path)) {
			ret[name] = *res;
		} else printError(res);
	}

	return ret;
}
