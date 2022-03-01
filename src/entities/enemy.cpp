#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>

#include <components/health.hpp>
#include <components/healthbar.hpp>
#include <entities/projectile.hpp>
#include <entities/player.hpp>
#include <entities/generator.hpp>
#include "enemy.hpp"

static channelBuffers_ptr sfx = nullptr;
static uint32_t counter = 0;

// TODO: seriously for real just do a resource manager already
static std::map<std::string, gameObject::ptr> enemyModels;

enemy::~enemy() {};
noodler::~noodler() {};
bat::~bat() {};

static std::array animationPaths {
	Entry {"idle",    DEMO_PREFIX "assets/obj/animations/advdroid/idle.glb"},
	Entry {"walking", DEMO_PREFIX "assets/obj/animations/advdroid/run-forward.glb"},
	Entry {"back",    DEMO_PREFIX "assets/obj/animations/advdroid/run-backward.glb"},
	Entry {"left",    DEMO_PREFIX "assets/obj/animations/advdroid/run-left.glb"},
	Entry {"right",   DEMO_PREFIX "assets/obj/animations/advdroid/run-right.glb"},
	Entry {"falling", DEMO_PREFIX "assets/obj/animations/advdroid/falling.glb"},
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
	rigidBody *body = attach<rigidBodyCapsule>(position, mass, radius, height);

	manager->registerComponent(this, this);
	manager->registerInterface<updatable>(this, this);

	gameObject::ptr model = nullptr;
	auto it = enemyModels.find(modelPath);

	// TODO:
	if (it == enemyModels.end()) {
		playerAnims = loadAnimations(animationPaths);
		auto data = loadSceneCompiled(modelPath);

		model = data;
		enemyModels.insert({modelPath, data});

	} else {
		model = it->second;
	}

	if (!sfx) {
		sfx = openAudio(DEMO_PREFIX "assets/sfx/mnstr7.ogg");
	}

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
	glm::vec3 playerPos;

	entity *playerEnt =
		findNearest<generator>(manager, node->getTransformTRS().position);

	if (playerEnt) {
		playerPos = playerEnt->getNode()->getTransformTRS().position;
	}

	// TODO: should this be a component, a generic chase implementation?
	//       wrapping things in generic "behavior" components could be pretty handy...
	rigidBody *body = getComponent<rigidBody>(manager, this);
	glm::vec3 diff = playerPos - node->getTransformTRS().position;
	{
		glm::vec3 vel =  glm::normalize(glm::vec3(diff.x, 0, diff.z));

		if (body) {
			body->phys->setAcceleration(10.f*vel);
		}
	}

	glm::vec3 vel = body->phys->getVelocity();
	if (glm::length(vel) < 2.0) {
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

		auto test = [&] (glm::vec3& vec) {
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
		character->setSpeed(glm::length(vel) / 8.0);
	}

	character->update(delta);

#if 0
	//rigidBody *body = castEntityComponent<rigidBody*>(manager, this, "rigidBody");
	//health *hp = castEntityComponent<health*>(manager, this, "health");

	rigidBody *body = getComponent<rigidBody>(manager, this);
	health *hp = getComponent<health>(manager, this);

	if (!body || !hp) {
		SDL_Log("No body/health!");
		return;
	}

	// BIG XXX
	auto v = std::dynamic_pointer_cast<projalphaView>(manager->engine->view);

	if (!v) {
		//std::cerr << "No view!" << std::endl;
		SDL_Log("enemy::update(): No view!");
		return;
	}

	// STILL XXX
	auto wfcgen = v->getGenerator();
	if (!wfcgen) {
		return;
	}

	gameObject::ptr wfcroot = wfcgen->getNode()->getNode("nodes");

	if (!wfcroot) {
		return;
	}

	if (hp->amount < 0.5) {
		// flee
		glm::vec3 dir = wfcgen->pathfindAway(selfPos, playerPos);
		body->phys->setAcceleration(10.f*dir);

	} else if (hp->amount < 1.0 || glm::distance(selfPos, playerPos) < 12.f) {
		glm::vec3 dir = wfcgen->pathfindDirection(selfPos, playerPos);
		body->phys->setAcceleration(10.f*dir);
	}

	uint32_t k = SDL_GetTicks();
	if (k - lastSound > 3000 + 50*xxxid) {
		auto ch = std::make_shared<spatialAudioChannel>(sfx);
		ch->worldPosition = node->getTransformTRS().position;
		manager->engine->audio->add(ch);
		lastSound = k;
	}
#endif
}

nlohmann::json enemy::serialize(entityManager *manager) {
	return entity::serialize(manager);
}
