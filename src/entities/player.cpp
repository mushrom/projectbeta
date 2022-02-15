#include <grend/gameEditor.hpp>
#include "player.hpp"
#include <components/playerInfo.hpp>

player::~player() {};

void animatedCharacter::setAnimation(std::string animation, float weight) {
	auto it = animations->find(animation);
	SDL_Log("setting animation '%s'!", animation.c_str());

	if (it != animations->end()) {
		currentAnimation = it->second;

	} else {
		SDL_Log("WARNING: setting nonexistant animation '%s'!", animation.c_str());
	}
}

// TODO: move to engine code somewhere
void applyAnimation(gameObject::ptr node, animationMap::ptr anim, float time) {
	if (node == nullptr || anim == nullptr) {
		return;
	}

	auto chans = anim->find(node->animChannel);

	if (chans != anim->end()) {
		TRS t = node->getOrigTransform();

		for (auto& ch : chans->second) {
			//SDL_Log("Have animation channel %08x", node->animChannel);
			ch->applyTransform(t, time, anim->endtime);
		}

		node->setTransform(t);
	}

	for (auto& [name, subnode] : node->nodes) {
		std::string asdf = name.substr(name.find(':') + 1);
		/*
		SDL_Log("sub node: %s:%08x -> %08x",
			asdf.c_str(),
			(uint32_t)std::hash<std::string>{}(asdf),
			subnode->animChannel);
			*/
		applyAnimation(subnode, anim, time);
	}
}

animatedCharacter::animatedCharacter(gameImport::ptr objs) {
	animations = objs->animations;
	objects = objs;
}

gameObject::ptr animatedCharacter::getObject(void) {
	return objects;
}

// TODO: might as well have a resource component
static gameImport::ptr playerModel = nullptr;

player::player(entityManager *manager, gameMain *game, glm::vec3 position)
	: entity(manager)
{

	//new boxSpawner(manager, this);
	new wieldedHandler(manager, this);
	new movementHandler(manager, this);
	new projectileCollision(manager, this);
	new syncRigidBodyPosition(manager, this);
	rigidBody *body = new rigidBodySphere(manager, this, position, 10.0, 0.75);

	manager->registerComponent(this, this);
	manager->registerInterface<updatable>(this, this);

	if (!playerModel) {
		// TODO: resource cache
		//playerModel = loadScene(GR_PREFIX "assets/obj/TestGuy/rigged-lowpolyguy.glb");
		SDL_Log("Loading player model...");
		playerModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/buff-dude-testanim.glb");
		//playerModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/trooper.glb");
		//playerModel = loadSceneCompiled(DEMO_PREFIX "assets/obj/ld48/player-cursor.glb");
		//playerModel = loadSceneCompiled("/home/flux/blender/objects/lowpoly-cc0-guy/low-poly-cc0-guy-fixedimport.gltf");

		/*
		TRS transform = playerModel->getTransformTRS();
		//transform.rotation = glm::quat(glm::vec3(0, -M_PI/2, 0));
		//transform.scale = glm::vec3(0.16f);
		//transform.scale = glm::vec3(2.0f);
		transform.position = glm::vec3(0, -0.75, 0);
		playerModel->setTransform(transform);
		*/
		playerModel->setPosition(glm::vec3(0, -0.75, 0));

		assert(playerModel != nullptr);
		SDL_Log("got player model");
	}

	//TRS transform = node->getTransformTRS();
	//transform.position = position;
	//node->setTransform(transform);
	setNode("model", node, playerModel);
	//setNode("light", node, std::make_shared<gameLightPoint>());
	//setNode("light", node, std::make_shared<gameLightPoint>());
	//auto lit = std::make_shared<gameLightPoint>();

	auto lit = std::make_shared<gameLightSpot>();
	lit->setTransform((TRS) {
		.position = glm::vec3(0, 0.5, 1),
		//.rotation = glm::quat(glm::vec3(0, -M_PI/2, 0)),
	});

	lit->intensity = 125;
	lit->is_static = false;
	lit->casts_shadows = true;
	//lit->angle = cos(glm::degrees(35.f));
	//lit->casts_shadows = false;

	auto plit = std::make_shared<gameLightPoint>();
	//plit->diffuse = glm::vec4(0.0, 0.17, 0.46, 1.0);
	plit->diffuse = glm::vec4(1.0);
	plit->setTransform((TRS) { .position = glm::vec3(0, 1.5, 0), });
	plit->intensity = 50;
	plit->radius = 0.75;
	plit->is_static = false;
	plit->casts_shadows = false;

	setNode("spotlight", node, lit);
	//setNode("pointlight", node, plit);
	character = std::make_shared<animatedCharacter>(playerModel);
	//character->setAnimation("idle");

	body->registerCollisionQueue(manager->collisions);
	auto [name, _] = *playerModel->animations->begin();

	character->setAnimation(name);
}

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

	setNode("model", node, playerModel);
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
	character = std::make_shared<animatedCharacter>(playerModel);

	auto [name, _] = *playerModel->animations->begin();
	character->setAnimation(name);
}

void animatedCharacter::update(float delta) {
	float time = SDL_GetTicks() / 1000.f;
	applyAnimation(objects, currentAnimation, time);
}

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
	} else {
		character->setAnimation("walking");
	}
}
