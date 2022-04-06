#pragma once

#include <grend/gameObject.hpp>
#include <grend/loadScene.hpp>
#include <grend/ecs/ecs.hpp>

using namespace grendx;
using namespace grendx::ecs;

#include <map>
inline std::map<std::string, gameImport::ptr> sceneCache;

class sceneTree : public component {
	public:
		sceneTree(entityManager *manager, entity *ent, const std::string& path)
			: component(manager, ent)
		{
			manager->registerComponent(ent, this);

			auto it = sceneCache.find(path);

			if (it != sceneCache.end()) {
				// XXX
				// entire thing here is a big hack, actually
				setNode("sceneTree", ent->node, it->second);

			} else {
				if (auto res = loadSceneCompiled(path)) {
					sceneCache[path] = *res;
					setNode("sceneTree", ent->node, *res);
				} else printError(res);
			}
		}
};
