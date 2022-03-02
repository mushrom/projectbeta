#include "targetArea.hpp"
#include <components/health.hpp>
#include <components/sceneTree.hpp>

#include <grend/loadScene.hpp>

using namespace grendx;
using namespace grendx::ecs;
using namespace nlohmann;

targetArea::~targetArea() {};
areaAddScore::~areaAddScore() {};

targetArea::targetArea(entityManager *manager,
                       entity *ent,
                       json properties)
	: entity(manager, properties)
{
	manager->registerComponent(ent, this);

	new areaSphere(manager, this, 5.f);
	attach<sceneTree>(DEMO_PREFIX "assets/obj/hill-sphere.glb");

	/*
	// TODO: resource manager
	static gameObject::ptr areaModel = nullptr;
	if (!areaModel) {
		areaModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/hill-sphere.glb");

		TRS transform = areaModel->getTransformTRS();
		transform.scale = glm::vec3(5.f);
		areaModel->setTransform(transform);
	}

	setNode("model", node, areaModel);
	*/
}

json targetArea::serialize(entityManager *manager) {
	return defaultProperties();
}

areaAddScore::areaAddScore(entityManager *manager,
                           entity *ent,
                           json properties)
	//: areaInside(manager, ent, {"targetArea", "area"})
	: areaInside(manager, ent, {getTypeName<targetArea>(), getTypeName<area>()})
{
	manager->registerComponent(ent, this);
}

void areaAddScore::onEvent(entityManager *manager, entity *ent, entity *other) {
	SDL_Log("Inside target area!");
	health *ownhealth = getComponent<health>(manager, ent);
	//health *ownhealth;

	//castEntityComponent(ownhealth, manager, ent, "health");

	if (ownhealth) {
		// TODO: need to normalize this to 60fps! this runs every frame when,
		//       the player is inside the area, so this function needs 
		//       a delta parameter somewhere
		//
		//       idea: could put current delta in manager, that's a quick
		//             and dirty fix, already storing a reference to the
		//             engine runtime there...
		ownhealth->heal(5.f);
		SDL_Log("Healing!");
	}
}

json areaAddScore::serialize(entityManager *manager) {
	return defaultProperties();
}
