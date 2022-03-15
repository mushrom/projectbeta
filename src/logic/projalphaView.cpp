#include <grend/gameMain.hpp>
#include <grend/gameMainDevWindow.hpp>
#include <grend/gameObject.hpp>
//#include <grend/playerView.hpp>
#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>
#include <grend/loadScene.hpp>
#include <grend/controllers.hpp>

#include <grend/ecs/ecs.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

#include <memory>
#include <map>
#include <vector>
#include <set>

using namespace grendx;
using namespace grendx::ecs;

#include <logic/projalphaView.hpp>
#include <logic/wfcGenerator.hpp>
#include <logic/UI.hpp>
#include <logic/levelController.hpp>

#include <entities/player.hpp>
#include <components/shader.hpp>

#include <nuklear/nuklear.h>

void projalphaView::logic(gameMain *game, float delta) {
	// XXX: handle input from end of frame at render() to logic() here
	//      on the next frame... should probably override handleInput()
	nk_input_end(nk_ctx);

	if (input.mode == modes::MainMenu
		|| input.mode == modes::Settings
		|| input.mode == modes::NewGame
		|| input.mode == modes::Intro
		|| input.mode == modes::Pause
		|| input.mode == modes::Won
		)
	{
		// XXX:
		return;
	}

	for (auto& p : floors) {
		p->processMessages();
	}

	/*
	// big XXX
	if (currentMap != loadedMap) {
		loadedMap = currentMap;
		load(game, currentMap);
	}
	*/

/*
	// TODO: is this still even being used
	if (input.mode == modes::Loading) {
		gameObject::ptr wfcroot = wfcgen->getNode()->getNode("nodes");
		//if (!game->state->rootnode->hasNode("wfc")) {
		if (!wfcroot || !wfcroot->hasNode("leaves")) {
			return;

		} else {
			input.setMode(modes::Move);
		}
	}
	*/

	if (input.mode == modes::Loading) {
		input.setMode(modes::Intro);
	}

	static glm::vec3 lastvel = glm::vec3(0);
	static gameObject::ptr retval;

	if (cam->velocity() != lastvel) {
		lastvel = cam->velocity();
	}

	game->phys->stepSimulation(delta);
	game->phys->filterCollisions();;

	//entity *playerEnt = findFirst(game->entities.get(), {"player"});
	//entity *playerEnt = findFirst(game->entities.get(), {getTypeName<player>()});
	entity *playerEnt = findFirst<player>(game->entities.get());
	

	if (playerEnt) {
		TRS transform = playerEnt->getNode()->getTransformTRS();
		cam->slide(transform.position - zoom*cam->direction(), 16.f, delta);
		//wfcgen->setPosition(game, transform.position);
	}

	game->entities->update(delta);

	if (level->won()) {
		SDL_Log("winner winner, a dinner of chicken!");
		level->reset();
		input.setMode(modes::Won);
	} 

	auto lost = level->lost();
	if (lost.first) {
		SDL_Log("lol u died: %s", lost.second.c_str());
		input.setMode(modes::MainMenu);
	}
}

struct flagHash {
	std::size_t operator()(const renderFlags& r) const noexcept {
		std::size_t ret = 737;

		unsigned k = (r.cull_faces << 6)
		           | (r.sort       << 5)
		           | (r.stencil    << 4)
		           | (r.depthTest  << 3)
		           | (r.depthMask  << 2)
		           | (r.syncshader << 1)
		           | (r.shadowmap  << 0);

		ret = ret*33 + (uintptr_t)r.mainShader.get();
		ret = ret*33 + (uintptr_t)r.skinnedShader.get();
		ret = ret*33 + (uintptr_t)r.instancedShader.get();
		ret = ret*33 + (uintptr_t)r.billboardShader.get();
		ret = ret*33 + k;

		return ret;
	}
};

// TODO: this should instantiate a render queue for each unique shader,
//       collect light nodes, build light/shadow/reflection maps etc...
//       need to break up the renderWorld() function into a bunch of functions
//       to do this, flush() should be moved out of render queue and instead
//       be a seperate function that also takes light list, etc parameters
//
//       etc.
//renderQueue drawEntities(gameMain *game) {
void drawEntities(gameMain *game, renderQueue& ret, camera::ptr cam) {
	entityManager *entities = game->entities.get();
	//renderQueue ret;

	auto drawable = searchEntities(entities, {getTypeName<abstractShader>()});
	auto entroot = game->state->rootnode->getNode("entities");

	std::map<std::size_t, renderQueue> queues;
	std::map<std::size_t, renderFlags> shadermap;

	for (entity *shader : drawable) {
		auto flags = shader->get<abstractShader>();
		size_t h = flagHash{}(flags->getShader());

		if (shadermap.find(h) == shadermap.end()) {
			shadermap[h] = flags->getShader();
		}

		entity *ent = entities->getEntity(shader);
		queues[h].add(ent->node);

		//entity *ent = entities->getEntity(shader);
		//ret.add(ent->node);
		//std::string name = "entity" + std::to_string((uintptr_t)ent);
		//setNode(name, entroot, ent->node);
	}

	// XXX: less than ideal
	renderQueue hax = ret;

	for (auto& [id, que] : queues) {
		hax.add(que);

		cullQueue(que, cam,
		          game->rend->framebuffer->width,
		          game->rend->framebuffer->height,
		          game->rend->lightThreshold);
		sortQueue(que, cam);
	}

	updateLights(game->rend, hax);
	updateReflections(game->rend, hax);
	buildTilemap(hax.lights, cam, game->rend);
	updateReflectionProbe(game->rend, hax, cam);

	game->rend->framebuffer->bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	DO_ERROR_CHECK();


	for (auto& [id, flags] : shadermap) {
		flush(queues[id], cam, game->rend->framebuffer, game->rend, flags);
	}
}

void projalphaView::render(gameMain *game) {
	int winsize_x, winsize_y;
	SDL_GetWindowSize(game->ctx.window, &winsize_x, &winsize_y);
	renderFlags flags = game-> rend->getLightingFlags();

	post->setUniform("cameraPos",     cam->position());
	post->setUniform("cameraForward", cam->direction());
	post->setUniform("cameraRight",   cam->right());
	post->setUniform("cameraUp",      cam->up());

	float tim = SDL_GetTicks() * 1000.f;
	post->setUniform("exposure", game->rend->exposure);
	post->setUniform("time_ms",  tim);
	post->setUniform("lightThreshold", game->rend->lightThreshold);

	post->setUniform("shadowmap_atlas", TEXU_SHADOWS);

	post->setUniformBlock("lights", game->rend->lightBuffer, UBO_LIGHT_INFO);
#if !defined(USE_SINGLE_UBO)
	post->setUniformBlock("point_light_tiles", game->rend->pointTiles,
	                      UBO_POINT_LIGHT_TILES);
	post->setUniformBlock("spot_light_tiles", game->rend->spotTiles,
	                      UBO_SPOT_LIGHT_TILES);
#endif

	if (input.mode == modes::Loading) {
		// render loading screen
		// TODO:
		Framebuffer().bind();
		setDefaultGlFlags();
		disable(GL_DEPTH_TEST);
		disable(GL_SCISSOR_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	} else {
		renderQueue foo = mapQueue;
		//renderQueue asdf = drawEntities(game);
		//foo.add(asdf);

		renderWorld(game, cam, foo, flags);
		drawEntities(game, foo, cam);

		post->draw(game->rend->framebuffer);
		renderHealthbars(game->entities.get(), nk_ctx, cam);

		if (debugTiles) {
			drawTileDebug(game);
		}

		switch (input.mode) {
			case modes::MainMenu:  drawMainMenu(game, winsize_x, winsize_y); break;
			case modes::Settings:  drawSettings(game, winsize_x, winsize_y); break;
			case modes::NewGame:   drawNewGameMenu(game, winsize_x, winsize_y); break;
			case modes::Intro:     drawIntroWindow(game, winsize_x, winsize_y); break;
			case modes::Pause:     drawPauseMenu(game, winsize_x, winsize_y); break;
			case modes::Won:       drawWinScreen(game, winsize_x, winsize_y); break;
			case modes::Inventory: drawInventory(game, winsize_x, winsize_y); break;
			default: break;
		}

		drawNavPrompts(game, winsize_x, winsize_y);
	}

	nk_sdl_render(NK_ANTI_ALIASING_ON, 512*1024, 128*1024);
	// XXX: handle input from end of frame here to beginning of logic()
	//      on the next frame... should probably override handleInput()
	nk_input_begin(nk_ctx);
}

// TODO: idk if this is going to be needed, at least not in
//       this project...
//       could consider removing it
void projalphaView::load(gameMain *game, std::string map) {
	return;
	// avoid reloading if the target map is already loaded
	if (true || map != currentMap) {
		TRS staticPosition; // default
		gameObject::ptr newroot
			//= game->state->rootnode
			= std::make_shared<gameObject>();

		currentMap = map;
		//game->state->rootnode = loadMapCompiled(game, map);
		game->jobs->addAsync([=, this] () {
			//auto [node, models] = loadMapData(game, map);
			if (auto res = loadMapData(game, map)) {
				auto mapdata = *res;
				auto node = mapdata.first;
				auto models = mapdata.second;

				game->jobs->addDeferred([=, this] () {
					// TODO: some sort of world entity
					//mapPhysics.clear();
					//mapQueue.clear();

	/*
					game->phys->addStaticModels(nullptr,
												node,
												staticPosition,
												mapPhysics);

					compileModels(models);
					*/

					level->reset();
					//game->state->rootnode = node;
					setNode("asyncLoaded", node, std::make_shared<gameObject>());
					setNode("entities", node, game->entities->root);
					setNode("maproot", game->state->rootnode, node);
					//setNode("wfc", node, wfcgen->getNode());
					//mapQueue.add(wfcgen->getNode());

					return true;
				});

				return true;

			} else printError(res);
		});
	}
}


