#include <grend/gameMain.hpp>
#include <grend/gameMainDevWindow.hpp>
#include <grend/gameObject.hpp>
//#include <grend/playerView.hpp>
#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>
#include <grend/loadScene.hpp>
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
#include <entities/blockade.hpp>

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

template <typename... T>
std::vector<const char *> getTypeNames() {
	return { getTypeName<T>()... };
}

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

template <typename T>
void addEntityNodes(levelController *controller, gameMain *game) {
	controller->addInit([=] () {
		const std::string& entname = demangle(getTypeName<T>());
		gameObject::ptr node = game->state->rootnode->getNode(entname);
		SDL_Log("Testing this '%s':%p", entname.c_str(), node.get());

		initEntitiesFromNodes(node,
			[&] (const std::string& name, gameObject::ptr& ptr) {
				auto en = new T(game->entities.get(), ptr->getTransformTRS().position);
				game->entities->add(en);

				updateEntityTransforms(game->entities.get(), en, ptr->getTransformTRS());
				en->node->setTransform(ptr->getTransformTRS());
			}
		);
	});
}

void addLevelInitializers(gameMain *game, projalphaView::ptr view);
int runTests(gameMain *game, projalphaView::ptr view, const char *target);

#include <logic/tests/tests.hpp>

// TODO: code is garbage
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
	//const char *mapfile = DEMO_PREFIX "assets/maps/arena-test.map";
	const char *mapfile = DEMO_PREFIX "assets/maps/save.map";

	if (argc > 1) {
		mapfile = argv[1];
	}

	SDL_Log("entering main()");
	SDL_Log("started SDL context");
	SDL_Log("have game state");

	// include editor in debug builds, use main game view for release
	// XXX: High DPI settings for my current setup
	renderSettings foo = (renderSettings){
		.scaleX = 1.0,
		.scaleY = 1.0,
		.targetResX = 1280,
		.targetResY = 720,
		.fullscreen = false,
		.windowResX = 1280,
		.windowResY = 720,
		.UIScale = 1.0,
	};

#if defined(GAME_BUILD_DEBUG)
	gameMainDevWindow *game = new gameMainDevWindow(foo);
#else
	gameMain *game = new gameMain("game", foo);
#endif

	/*
	// TODO: try to autodetect whether this is a high DPI display,
	//       and set scaling accordingly
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
	*/

	initController();

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
#endif

	addLevelInitializers(game, view);

	SDL_Log("Got to game->run()! mapfile: %s\n", mapfile);

	static std::vector<physicsObject::ptr> mapPhysics;
	if (auto res = loadMapCompiled(mapfile)) {
		auto mapdata = *res;
		game->state->rootnode = mapdata;
		//setNode("entities", game->state->rootnode, game->entities->root);

		game->phys->addStaticModels(nullptr, mapdata, TRS(), mapPhysics);
	} else {
		printError(res);
		return 2;
	}

	if (const char *target = getenv("GREND_TEST_TARGET")) {
		return runTests(game, view, target);

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

void addLevelInitializers(gameMain *game, projalphaView::ptr view) {
	view->level->addInit([=] () {
		entity *playerEnt;
		glm::vec3 pos(-5, 20, -5);

		playerEnt = new player(game->entities.get(), game, pos);
		game->entities->add(playerEnt);

		playerEnt->attach<health>();
		playerEnt->attach<enemyCollision>();
		playerEnt->attach<healthPickupCollision>();
		playerEnt->attach<playerInfo>();
		inventory *inv = playerEnt->attach<inventory>();

		playerEnt->attach<pickupAction>( getTypeNames<amuletPickup>() );
		playerEnt->attach<pickupAction>( getTypeNames<pickup>() );
		playerEnt->attach<autopickupAction>( getTypeNames<autopickup>() );

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
		playerEnt->attach<mouseRotationPoller>(view->cam);
		//new mouseRotationPoller(game->entities.get(), playerEnt, view->cam);
#endif
	});

	addEntityNodes<enemySpawner>(view->level.get(), game);
	addEntityNodes<generator>(view->level.get(), game);
	addEntityNodes<blockade>(view->level.get(), game);

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
			std::set<entity*> players
				= searchEntities(game->entities.get(), {getTypeName<player>()});
			std::set<entity*> generators
				= searchEntities(game->entities.get(), {getTypeName<generator>()});

			bool lost = players.size() == 0 || generators.size() == 0;
			return std::pair<bool, std::string>(lost, "lol u died");
		});

}

int runTests(gameMain *game, projalphaView::ptr view, const char *target) {
	SDL_Log("Got a test target!");

	if (strcmp(target, "default") == 0) {
		//view->setMode(projalphaView::modes::Move);
		bool result = tests::defaultTest(game, view);
		SDL_Log("Test '%s' %s.", target, result? "passed" : "failed");

	} else {
		SDL_Log("Unknown test '%s', counting that as an error...", target);
		return 1;
	}
}
