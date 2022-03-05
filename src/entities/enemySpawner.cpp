#include <grend/loadScene.hpp>

#include <components/health.hpp>
#include <components/healthbar.hpp>
#include <components/team.hpp>
#include <components/shader.hpp>

#include <entities/projectile.hpp>
#include <entities/enemy.hpp>
#include "enemySpawner.hpp"

enemySpawner::~enemySpawner() {};

enemySpawner::enemySpawner(entityManager *manager, glm::vec3 position)
	: entity(manager)
{
	static gameObject::ptr spawnerModel = nullptr;

	new health(manager, this, 1.f, 1000.f);
	new worldHealthbar(manager, this);
	new projectileCollision(manager, this);
	attach<PBRShader>();
	auto body = new rigidBodySphere(manager, this, position, 0.0, 1.0);

	manager->registerComponent(this, this);
	manager->registerInterface<updatable>(this, this);

	// TODO: resource manager
	if (!spawnerModel) {
		auto [data, _] = loadSceneAsyncCompiled(manager->engine, DEMO_PREFIX "assets/obj/enemy-spawner.glb");
		spawnerModel = data;
	}

	TRS transform = node->getTransformTRS();
	transform.position = position;
	node->setTransform(transform);

	// TODO: this should just be done as part of creating a rigidBody...
	body->registerCollisionQueue(manager->collisions);
	setNode("model", node, spawnerModel);
}

enemySpawner::enemySpawner(entityManager *manager,
                           entity *ent,
                           nlohmann::json properties) 
	: entity(manager, properties)
{
	static gameObject::ptr spawnerModel = nullptr;

	new health(manager, this, 1.f, 1000.f);
	new worldHealthbar(manager, this);
	new projectileCollision(manager, this);
	auto body = new rigidBodySphere(manager, this, node->getTransformTRS().position,
	                                0.0, 1.0);

	manager->registerComponent(this, this);

	// TODO: resource manager
	if (!spawnerModel) {
		auto [data, _] = loadSceneAsyncCompiled(manager->engine, DEMO_PREFIX "assets/obj/enemy-spawner.glb");
		spawnerModel = data;
	}

	// TODO: this should just be done as part of creating a rigidBody...
	body->registerCollisionQueue(manager->collisions);
	setNode("model", node, spawnerModel);
}

nlohmann::json enemySpawner::serialize(entityManager *manager) {
	return entity::serialize(manager);
}

void enemySpawner::update(entityManager *manager, float delta) {
	float curTime = SDL_GetTicks() / 1000.f;

	if (curTime - lastSpawn > 2.0f) {
		lastSpawn = curTime;

		/*
		auto en = new enemy(manager, manager->engine,
		                    this->node->getTransformTRS().position);
							*/
		auto en = new noodler(manager, manager->engine,
		                      this->node->getTransformTRS().position);
		manager->add(en);

		// if this spawner has an associated team, propagate that to
		// spawned enemies
		//if (team *tem = castEntityComponent<team*>(manager, this, "team")) {
		if (team *tem = getComponent<team>(manager, this)) {
			new team(manager, en, tem->name);
		}
	}
}
