#include <grend/gameMain.hpp>
#include <grend/gameMainDevWindow.hpp>
#include <grend/gameObject.hpp>
//#include <grend/playerView.hpp>
#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>
#include <grend/controllers.hpp>

#include <grend/ecs/ecs.hpp>
#include <grend/ecs/rigidBody.hpp>
#include <grend/ecs/collision.hpp>
#include <grend/ecs/serializer.hpp>

// TODO: move this to the core grend tree
#include <logic/gameController.hpp>

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

// XXX:  toggle using textures/models I have locally, don't want to
//       bloat the assets folder again
#define LOCAL_BUILD 0

using namespace grendx;
using namespace grendx::ecs;

// TODO: should include less stuff
#include <components/inputHandler.hpp>
#include <components/health.hpp>
#include <components/healthbar.hpp>
#include <components/timedLifetime.hpp>
#include <components/team.hpp>
#include <components/itemPickup.hpp>
#include <components/inventory.hpp>
#include <components/playerInfo.hpp>

#include <entities/player.hpp>
#include <entities/enemy.hpp>
#include <entities/projectile.hpp>
#include <entities/enemyCollision.hpp>
#include <entities/items/items.hpp>
#include <entities/flag.hpp>
#include <entities/enemySpawner.hpp>
#include <entities/killedParticles.hpp>
#include <entities/targetArea.hpp>
#include <entities/amulet.hpp>
#include <entities/generator.hpp>

#include <logic/projalphaView.hpp>
#include <logic/wfcGenerator.hpp>
#include <logic/UI.hpp>
#include <logic/levelController.hpp>

#include <nuklear/nuklear.h>

// TODO: move to utilities header
struct grendDirEnt {
	std::string name;
	bool isFile;
};

static std::vector<grendDirEnt> listdir(std::string path) {
	std::vector<grendDirEnt> ret;

// XXX: older debian raspi is built on doesn't include c++17 filesystem functions,
//      so leave the old posix code in for that... aaaaa
#if defined(_WIN32)
	if (fs::exists(currentDir) && fs::is_directory(path)) {
		for (auto& p : fs::directory_iterator(currentDir)) {
			ret.push_back({
				p.path().filename(),
				!fs::is_directory(p.path())
			});
		}

	} else {
		SDL_Log("listdir: Invalid directory %s", path.c_str());
	}

#else
	DIR *dirp;

	if ((dirp = opendir(path.c_str()))) {
		struct dirent *dent;

		while ((dent = readdir(dirp))) {
			ret.push_back({
				std::string(dent->d_name),
				dent->d_type != DT_DIR
			});
		}
	}
#endif

	return ret;
}

void initEntitiesFromNodes(gameObject::ptr node,
                           std::function<void(const std::string&, gameObject::ptr&)> init)
{
	if (node == nullptr) {
		return;
	}

	for (auto& [name, ptr] : node->nodes) {
		init(name, ptr);
	}
}

#include <logic/tests/tests.hpp>

// XXX: global, TODO REMOVE FOR REAL AAAAAAAAA
struct nk_image fooimg;

#if defined(_WIN32)
extern "C" {
//int WinMain(int argc, char *argv[]);
int WinMain(void);
}

int WinMain(void) try {
	int argc = 1;
	const char *argv[] = {"asdf"};
#else
int main(int argc, char *argv[]) { try {
#endif
	//const char *mapfile = DEMO_PREFIX "assets/maps/level-test.map";
	const char *mapfile = DEMO_PREFIX "assets/maps/arena-test.map";

	if (argc > 1) {
		mapfile = argv[1];
	}

	SDL_Log("entering main()");
	SDL_Log("started SDL context");
	SDL_Log("have game state");

	// include editor in debug builds, use main game view for release
	// XXX: High DPI settings for my current setup
	renderSettings foo;
	foo.scaleX = 0.5;
	foo.scaleY = 0.5;
	foo.windowResX = 2560;
	foo.windowResY = 1440;
	foo.fullscreen = false;
	foo.UIScale = 2.0;

#if defined(GAME_BUILD_DEBUG)
	gameMainDevWindow *game = new gameMainDevWindow(foo);
#else
	gameMain *game = new gameMain(foo);
#endif

	int gl_w, gl_h;
	SDL_GL_GetDrawableSize(game->ctx.window, &gl_w, &gl_h);
	SDL_Log("Drawable: %dx%d", gl_w, gl_h);
	int sdl_w, sdl_h;
	SDL_GetWindowSize(game->ctx.window, &sdl_w, &sdl_h);
	SDL_Log("Window size: %dx%d", gl_w, gl_h);

	float diag, vert, horiz;
	int getdpi = SDL_GetDisplayDPI(0, &diag, &vert, &horiz);

	if (getdpi == 0) {
		SDL_Log("Successful: %g, %g, %g", diag, vert, horiz);
	} else {
		SDL_Log("SDL_GetDisplayDPI() failed");
	}


	initController();
	SDL_Log("loading image...");
	//fooimg = nk_image_load("/home/flux/pics/shit/thinkin.png");
	//fooimg = nk_image_load("/tmp/portagrend/music.png");

	/*
	game->jobs->addAsync([=] {
		auto foo = openSpatialLoop(GR_PREFIX "assets/sfx/Bit Bit Loop.ogg");
		foo->worldPosition = glm::vec3(-10, 0, -5);
		game->audio->add(foo);
		return true;
	});
	*/

#if 1
	/*
	game->jobs->addAsync([=] {
		auto msc = openAudio("/tmp/Kyuss.ogg");

		auto bar = std::make_shared<stereoAudioChannel>(msc);
		glm::vec3 r(rand() / (float)RAND_MAX, 0, rand() / (float)RAND_MAX);
		bar->worldPosition = glm::vec3(4*32.f * r.x, 2.f, 4*32.f * r.z);
		bar->loopMode = audioChannel::mode::Loop;
		game->audio->add(bar);
		return true;
	});
	*/

	game->jobs->addAsync([=] {
		auto hum = openAudio(DEMO_PREFIX "assets/sfx/cave themeb4.ogg");
		auto water = openAudio(DEMO_PREFIX "assets/sfx/atmosbasement.mp3_.ogg");

		for (auto& sfx : {hum, water}) {
			for (int i = 0; i < 5; i++) {
				auto bar = std::make_shared<spatialAudioChannel>(sfx);
				glm::vec3 r(rand() / (float)RAND_MAX, 0, rand() / (float)RAND_MAX);
				bar->worldPosition = glm::vec3(4*32.f * r.x, 2.f, 4*32.f * r.z);
				bar->loopMode = audioChannel::mode::Loop;
				game->audio->add(bar);
			}
		}

		//auto bar = openSpatialLoop(GR_PREFIX "assets/sfx/Meditating Beat.ogg");
		//auto bar = openSpatialLoop(DEMO_PREFIX "assets/sfx/cave themeb4.ogg");
		//bar->worldPosition = glm::vec3(34, 0, 34);
		//game->audio->add(bar);
		return true;
	});
#endif

	projalphaView::ptr view = std::make_shared<projalphaView>(game);
	view->cam->setFar(1000.0);
	view->cam->setFovx(70.0);
	game->setView(view);
	game->rend->lightThreshold = 0.2;

#if defined(GAME_BUILD_DEBUG)
	// XXX: bit of a hacky way to have more interactive map editing...
	//      not the best, but it does the job
	//
	//      could do something like have a transform or sceneNode component,
	//      and have the editor be able to work with those
	game->addEditorCallback(
		[=] (gameObject::ptr node, gameEditor::editAction action) {
			SDL_Log("Got to the editor callback!");
			view->level->reset();
		});

	// TODO: need some way to update world state when editor nodes
	//       are added, could be as simple as a list of callback functions
	//       that get called when adding nodes
	game->input.bind(MODAL_ALL_MODES, [=] (SDL_Event& ev, unsigned flags) {
		if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_p) {
			view->level->reset();
		}
		return MODAL_NO_CHANGE;
	});
#endif

	view->level->addInit([=] () {
		//view->wfcgen->generate(game, {});
	});

	view->level->addInit([=] () {
		entity *playerEnt;
		glm::vec3 pos(-5, 20, -5);

		/*
		auto floor = view->getFloor(game, view->currentFloor);
		if (floor) {
			pos = floor->entrance;
		}
		*/

		playerEnt = new player(game->entities.get(), game, pos);
		game->entities->add(playerEnt);
		//new generatorEventHandler(game->entities.get(), playerEnt);
		new health(game->entities.get(), playerEnt);
		new enemyCollision(game->entities.get(), playerEnt);
		new healthPickupCollision(game->entities.get(), playerEnt);
		new flagPickup(game->entities.get(), playerEnt);
		new team(game->entities.get(), playerEnt, "blue");
		new areaAddScore(game->entities.get(), playerEnt, {});
		new playerInfo(game->entities.get(), playerEnt, {});
		inventory *inv = new inventory(game->entities.get(), playerEnt, {});

		// start with 5 flares
		for (int i = 0; i < 5; i++) {
			entity *flare = new flareItem(game->entities.get(), glm::vec3(0));
			game->entities->add(flare);
			inv->insert(game->entities.get(), flare);
		}

		// 20 bullets
		for (int i = 0; i < 50; i++) {
			entity *bullet = new boxBullet(game->entities.get(), game, glm::vec3(0));
			game->entities->add(bullet);
			inv->insert(game->entities.get(), bullet);
		}

		//new pickupAction(game->entities.get(), playerEnt, {"amuletPickup"});
		//new pickupAction(game->entities.get(), playerEnt, {"pickup"});
		//new autopickupAction(game->entities.get(), playerEnt, {"autopickup"});
		new pickupAction(game->entities.get(), playerEnt, { getTypeName<amuletPickup>() });
		new pickupAction(game->entities.get(), playerEnt, { getTypeName<pickup>() });
		new autopickupAction(game->entities.get(), playerEnt, { getTypeName<autopickup>() });

#if defined(__ANDROID__)
		int wx = game->rend->screen_x;
		int wy = game->rend->screen_y;
		glm::vec2 movepad  ( 2*wx/16.f, 7*wy/9.f);
		glm::vec2 actionpad(14*wx/16.f, 7*wy/9.f);

		new touchMovementHandler(game->entities.get(), playerEnt, cam,
								 view->inputSystem->inputs, movepad, 150.f);
		new touchRotationHandler(game->entities.get(), playerEnt, cam,
								 view->inputSystem->inputs, actionpad, 150.f);

#else
		new mouseRotationPoller(game->entities.get(), playerEnt, view->cam);
#endif
	});

	view->level->addInit([=] () {
		view->currentFloor = -1;
		view->incrementFloor(game, 1);
	});

	view->level->addInit([=] () {
		gameObject::ptr spawners = game->state->rootnode->getNode("spawners");

		initEntitiesFromNodes(spawners,
			[&] (const std::string& name, gameObject::ptr& ptr) {
				std::cerr << "have spawner node " << name << std::endl;

				auto en = new enemySpawner(game->entities.get(), game,
										   ptr->getTransformTRS().position);
				game->entities->add(en);
			});
	});

	view->level->addInit([=] () {
		gameObject::ptr spawners = game->state->rootnode->getNode("generators");

		initEntitiesFromNodes(spawners,
			[&] (const std::string& name, gameObject::ptr& ptr) {
				std::cerr << "have spawner node " << name << std::endl;

				auto gen = new generator(game->entities.get(),
				                        ptr->getTransformTRS().position);
				game->entities->add(gen);
			});
	});

	view->level->addDestructor([=] () {
		// TODO: should just have reset function in entity manager
		for (auto& ent : game->entities->entities) {
			game->entities->remove(ent);
		}

		game->entities->clearFreedEntities();
		view->floors.clear();
	});

	view->level->addObjective("Reach exit",
		[=] () {
			auto players
				= searchEntities(game->entities.get(), {"player", "hasItem:amuletPickup"});
			// TODO: check for goal item (amulet?) and current floor == 0 (exit)
			return view->currentFloor == -1 && players.size() != 0;
		});

	view->level->addLoseCondition(
		[=] () {
		/*
			std::set<entity*> players
				= searchEntities(game->entities.get(), {"player"});
				*/
			std::set<entity*> players
				= searchEntities(game->entities.get(), {getTypeName<player>()});
			std::set<entity*> generators
				= searchEntities(game->entities.get(), {getTypeName<generator>()});

			bool lost = players.size() == 0 || generators.size() == 0;
			return std::pair<bool, std::string>(lost, "lol u died");
		});

	SDL_Log("Got to game->run()! mapfile: %s\n", mapfile);
	//view->load(game, mapfile);
	auto mapdata = loadMapCompiled(game, mapfile);
	game->state->rootnode = mapdata;
	//setNode("asdf", game->state->rootnode, mapdata);
	setNode("entities", game->state->rootnode, game->entities->root);

	std::vector<physicsObject::ptr> mapPhysics;
	game->phys->addStaticModels(nullptr,
								mapdata,
								TRS(),
								mapPhysics);

	if (char *target = getenv("GREND_TEST_TARGET")) {
		SDL_Log("Got a test target!");

		if (strcmp(target, "default") == 0) {
			//view->setMode(projalphaView::modes::Move);
			bool result = tests::defaultTest(game, view);
			SDL_Log("Test '%s' %s.", target, result? "passed" : "failed");

		} else {
			SDL_Log("Unknown test '%s', counting that as an error...", target);
			return 1;
		}

	} else {
		SDL_Log("No test configured, running normally");
		game->run();
	}

	return 0;

} catch (const std::exception& ex) {
	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Exception! %s", ex.what());
	return 1;

} catch (const char* ex) {
	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Exception! %s", ex);
	return 1;
}
}
