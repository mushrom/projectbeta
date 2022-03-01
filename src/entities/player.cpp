#include <grend/gameEditor.hpp>
#include "player.hpp"
#include <components/playerInfo.hpp>

player::~player() {};

static gameImport::ptr playerModel = nullptr;
static std::array animationPaths {
	Entry {"walking", DEMO_PREFIX "assets/obj/animations/humanoid/run-forward.glb"},
	Entry {"idle",    DEMO_PREFIX "assets/obj/animations/humanoid/idle.glb"},
	Entry {"left",    DEMO_PREFIX "assets/obj/animations/humanoid/run-left.glb"},
	Entry {"right",   DEMO_PREFIX "assets/obj/animations/humanoid/run-right.glb"},
	Entry {"back",    DEMO_PREFIX "assets/obj/animations/humanoid/run-backward.glb"},
	Entry {"falling", DEMO_PREFIX "assets/obj/animations/humanoid/falling.glb"},
};

static loadedAnims playerAnims;

/*
// testing stuff, leaving here for now
animfile  = loadSceneCompiled(DEMO_PREFIX "assets/obj/animations/humanoid/Humanoid@RunForwardUnarmed.glb");
idlefile  = loadSceneCompiled(DEMO_PREFIX "assets/obj/animations/humanoid/Humanoid@IdleCombat.glb");
leftfile  = loadSceneCompiled(DEMO_PREFIX "assets/obj/animations/humanoid/Humanoid@RunLeftUnarmed.glb");
rightfile = loadSceneCompiled(DEMO_PREFIX "assets/obj/animations/humanoid/Humanoid@RunRightUnarmed.glb");
backfile  = loadSceneCompiled(DEMO_PREFIX "assets/obj/animations/humanoid/Humanoid@RunBackwardsUnarmed.glb");
fallfile  = loadSceneCompiled(DEMO_PREFIX "assets/obj/animations/humanoid/Humanoid@FallingUnarmed.glb");
*/

player::player(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager)
{
	attach<wieldedHandler>();
	attach<movementHandler>();
	attach<projectileCollision>();
	attach<syncRigidBodyPosition>();
	rigidBody *body = attach<rigidBodySphere>(position, 10, 0.75);

	/*
	//new boxSpawner(manager, this);
	new wieldedHandler(manager, this);
	new movementHandler(manager, this);
	new projectileCollision(manager, this);
	new syncRigidBodyPosition(manager, this);
	rigidBody *body = new rigidBodySphere(manager, this, position, 10.0, 0.75);
	*/

	manager->registerComponent(this, this);
	manager->registerInterface<updatable>(this, this);

	if (!playerModel) {
		playerAnims = loadAnimations(animationPaths);

		// TODO: resource cache
		//playerModel = loadScene(GR_PREFIX "assets/obj/TestGuy/rigged-lowpolyguy.glb");
		SDL_Log("Loading player model...");
		//playerModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/buff-dude-testanim.glb");
		playerModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/trooper.glb");
		//playerModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/ld48/player-cursor.glb");
		//playerModel = loadSceneCompiled("/home/flux/blender/objects/lowpoly-cc0-guy/low-poly-cc0-guy-fixedimport.gltf");

		playerModel->setPosition(glm::vec3(0, -0.75, 0));

		assert(playerModel != nullptr);
		SDL_Log("got player model");
	}

	auto temp = std::static_pointer_cast<gameImport>(duplicate(playerModel));
	character = std::make_shared<animationController>(temp);
	setNode("model", node, temp);

	auto lit = std::make_shared<gameLightSpot>();

	lit->setPosition({0, 0.5, 1});
	lit->intensity = 125;
	lit->is_static = false;
	lit->casts_shadows = true;
	setNode("spotlight", node, lit);

	for (const auto& [name, obj] : playerAnims) {
		auto& anim = obj->animations->begin()->second;
		character->bind(name, anim);
	}

	body->registerCollisionQueue(manager->collisions);
	auto [name, _] = *playerModel->animations->begin();

	character->setAnimation(name);
}

#if 0
// commenting this out for now, might have gotten ahead of myself with serialization
player::player(entityManager *manager,
               entity *ent,
               nlohmann::json properties)
	: entity(manager, properties)
{
	manager->registerComponent(this, this);

	if (!playerModel) {
		// TODO: resource cache
		//playerModel = loadScene(GR_PREFIX "assets/obj/TestGuy/rigged-lowpolyguy.glb");
		SDL_Log("Loading player model...");
		playerModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/buff-dude-testanim.glb");

		TRS transform = playerModel->getTransformTRS();
		transform.rotation = glm::quat(glm::vec3(0, -M_PI/2, 0));
		//transform.scale = glm::vec3(0.16f);
		transform.position = glm::vec3(0, -0.5, 0);
		playerModel->setTransform(transform);

		//bindCookedMeshes();
		assert(playerModel != nullptr);
		SDL_Log("got player model");
	}

	auto temp = std::static_pointer_cast<gameImport>(duplicate(playerModel));
	setNode("model", node, temp);
	//setNode("model", node, playerModel);
	//setNode("light", node, std::make_shared<gameLightPoint>());
	//setNode("light", node, std::make_shared<gameLightPoint>());
	//auto lit = std::make_shared<gameLightPoint>();
	auto lit = std::make_shared<gameLightSpot>();
	lit->setTransform((TRS) {
		.position = glm::vec3(0, 0, 1),
		.rotation = glm::quat(glm::vec3(0, -M_PI/2, 0)),
	});

	lit->intensity = 200;
	lit->is_static = false;
	lit->casts_shadows = true;
	setNode("light", node, lit);
	character = std::make_shared<animationController>(temp);

	auto [name, _] = *playerModel->animations->begin();
	character->setAnimation(name);
}
#endif

nlohmann::json player::serialize(entityManager *manager) {
	return entity::serialize(manager);
}

void player::update(entityManager *manager, float delta) {
	rigidBody *body = getComponent<rigidBody>(manager, this);
	if (!body) return;

	body->phys->setAngularFactor(0.f);
	character->update(delta);

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
		character->setSpeed(glm::length(vel) / 16.0);
	}
}
