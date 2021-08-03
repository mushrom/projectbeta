#include <grend/ecs/ecs.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <memory>
#include <chrono>
#include <map>
#include <vector>
#include <set>
#include <functional>

#include <initializer_list>

using namespace grendx;
using namespace grendx::ecs;

#include <entities/player.hpp>
#include <entities/enemy.hpp>
#include <entities/projectile.hpp>
#include <entities/enemyCollision.hpp>
#include <entities/amulet.hpp>
#include <entities/items/items.hpp>

#include <logic/projalphaView.hpp>
#include <logic/wfcGenerator.hpp>
#include <logic/UI.hpp>
#include <logic/levelController.hpp>

#include <nuklear/nuklear.h>

projalphaView::floorStates::floorStates(gameMain *game,
                                        projalphaView *view,
                                        std::string z,
                                        wfcSpec::ptr spec)
	: zone(z)
	  //mapQueue(view->cam)
{
	generator = std::make_shared<wfcGenerator>(game, spec),
	generator->generate(game, {});

	SDL_Log("Generated");
	generator->setPosition(game, glm::vec3(0));
	SDL_Log("Set position");

	//setNode("foo", game->state->rootnode, generator->getNode());
	//mapQueue.add(generator->getNode());
	//SDL_Log("Adding to queue: %lu meshes\n", mapQueue.meshes.size());

	gameObject::ptr wfcroot = generator->getNode()->getNode("nodes");
	if (wfcroot && wfcroot->hasNode("leaves")) {
		gameObject::ptr leafroot = wfcroot->getNode("leaves");
		glm::vec3 amuletPos;

		for (const auto& [name, ptr] : leafroot->nodes) {
			auto en = new enemy(game->entities.get(),
					game,
					ptr->getTransformTRS().position + glm::vec3(4, 2, 0));
			amuletPos = ptr->getTransformTRS().position + glm::vec3(2);

			game->entities->add(en);
			levelEntities.push_back(en);

			int mod = rand() % 3;

			if (mod == 0) {
				auto hen = new healthPickup(game->entities.get(),
				                            ptr->getTransformTRS().position + glm::vec3(0, 0.75, 0));
				
				game->entities->add(hen);
				levelEntities.push_back(hen);

			} else if (mod == 1) {
				// chest
				auto cen = new chestItem(game->entities.get(),
				                         ptr->getTransformTRS().position + glm::vec3(0, 0.75, 0));

				game->entities->add(cen);
				levelEntities.push_back(cen);

			} else {
				auto xen = new coinPickup(game->entities.get(),
				                          ptr->getTransformTRS().position + glm::vec3(0, 0, 0));
				
				game->entities->add(xen);
				levelEntities.push_back(xen);
			}
		}

		/*
		if (currentFloor == 5) {
			auto en = new amuletPickup(game->entities.get(), game, amuletPos);
			game->entities->add(en);
		}
		*/

		if (rand() % 10 == 0) {
			auto en = new amuletPickup(game->entities.get(), game, amuletPos);
			game->entities->add(en);
		}
	}
	
	SDL_Log("Generated entities");

	auto entranceNode = wfcroot->getNode("entry");
	auto exitNode     = wfcroot->getNode("exit");

	if (entranceNode) entrance = entranceNode->getTransformTRS().position;
	if (exitNode)     exit     = exitNode->getTransformTRS().position;

	SDL_Log("Have entrance: (%g, %g, %g)", entrance.x, entrance.y, entrance.z);
	SDL_Log("Have exit: (%g, %g, %g)", exit.x, exit.y, exit.z);
}

projalphaView::floorStates* projalphaView::getFloor(gameMain *game, int n) {
	if (n < 0) return nullptr;

	if (n < (int)floors.size()) {
		return &floors[n];
	}

	// otherwise, have to generate a new floor
	// TODO: what happens if there's a jump larger than one level, not
	//       just pushing to the back?

	floorStates foo(game, this, "catacombs", spec);
	//SDL_Log("Generated floor, map queue has %lu meshes", foo.mapQueue.meshes.size());

	floors.push_back(foo);
	return &floors.back();
}

void projalphaView::incrementFloor(gameMain *game, int amount) {
	int nextFloor = currentFloor + amount;
	auto cur  = getFloor(game, currentFloor);
	auto next = getFloor(game, nextFloor);

	if (cur) {
		/* deactivate stuff */;
		cur->generator->mapobjs.clear();

		// XXX
		// TODO: need to remove items from level entities when they're picked up...
		//       and deleted, need to re-add them to the current level when they're
		//       dropped from inventory, hmmmmm... not sure how to approach this
		for (auto& e : cur->levelEntities) {
			if (game->entities->valid(e)) {
				game->entities->deactivate(e);
			}
		}
	}

	if (next) {
		/* activate stuff */
		next->generator->mapobjs.clear();
		next->generator->setPosition(game, glm::vec3(0));
		mapQueue.clear();
		mapQueue.add(next->generator->getNode());
		SDL_Log("Built queue: %lu meshes\n", mapQueue.meshes.size());

		for (auto& e : next->levelEntities) {
			if (game->entities->valid(e)) {
				game->entities->activate(e);
			}
		}

		entity *playerEnt = findFirst(game->entities.get(), {"player"});
		if (playerEnt) {
			TRS t;
			t.position = (amount > 0)? next->entrance : next->exit;
			// XXX: avoid falling below staircases
			t.position += glm::vec3(0, 2, 0);
			updateEntityTransforms(game->entities.get(), playerEnt, t);
			SDL_Log("Setting player transform");
		}
	};

	currentFloor = nextFloor;
}

bool projalphaView::nearNode(gameMain *game, const std::string& name, float thresh)
{
	auto floor = getFloor(game, currentFloor);
	// XXX: don't like this one bit
	if (!floor) return false;

	gameObject::ptr wfcroot = floor->generator->getNode()->getNode("nodes");
	//gameObject::ptr wfcroot = wfcgen->getNode()->getNode("nodes");
	entity *playerEnt = findFirst(game->entities.get(), {"player"});

	if (wfcroot->hasNode(name) && playerEnt) {
		TRS nodeTrans = wfcroot->getNode(name)->getTransformTRS();
		TRS playerTrans = playerEnt->getNode()->getTransformTRS();
		glm::vec3 pos = nodeTrans.position;
		return glm::distance(pos, playerTrans.position) < thresh;
	}

	return false;
}
