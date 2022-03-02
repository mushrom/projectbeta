#include "flag.hpp"
#include <components/team.hpp>
#include <components/sceneTree.hpp>

#include <grend/loadScene.hpp>

flag::~flag() {};
flagPickup::~flagPickup() {};

flag::flag(entityManager *manager, gameMain *game,
           glm::vec3 position, std::string color)
	: entity(manager)
{
	new team(manager, this, color);
	new areaSphere(manager, this, 2.f);

	manager->registerComponent(this, this);

	/*
	// TODO: resource manager
	static gameObject::ptr flagModel = nullptr;
	if (!flagModel) {
		flagModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/flag.glb");

		TRS transform = flagModel->getTransformTRS();
		transform.scale = glm::vec3(2.0);
		flagModel->setTransform(transform);
	}
	setNode("model", node, flagModel);
	*/

	attach<sceneTree>(DEMO_PREFIX "assets/obj/flag.glb");

	//node->setTransform((TRS) { .position = position, });
	node->setPosition(position);
}

void flag::update(entityManager *manager, float delta) { };

void flagPickup::onEvent(entityManager *manager, entity *ent, entity *other) {
	SDL_Log("Entered flag pickup zone!");
	team *mytem   = getComponent<team>(manager, ent);
	team *theytem = getComponent<team>(manager, other);
	/*
	team *theytem;
	team *mytem;
	castEntityComponent(mytem,   manager, ent,   "team");
	castEntityComponent(theytem, manager, other, "team");
	*/

	if (mytem && theytem && mytem->name != theytem->name) {
		SDL_Log("Different teams, doing the thing!");
		manager->remove(other);
		//new hasFlag(manager, ent, theytem->name);

	} else if (!mytem || !theytem) {
		SDL_Log("Missing team component somewhere!");
	}
}
