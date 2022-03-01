#include <grend/geometryGeneration.hpp>
#include <grend/loadScene.hpp>
#include <stdlib.h>

#include "killedParticles.hpp"
#include <components/boxSpawner.hpp>
#include <components/timedLifetime.hpp>
#include <entities/player.hpp>

// destructor for rtti
killedParticles::~killedParticles() {};
boxParticles::~boxParticles() {};

void killedParticles::onEvent(entityManager *manager, entity *ent, float delta) {
	//auto box = new player(manager, manager->engine, ent->node->transform.position + glm::vec3(0, 0, 1));
	auto box = new boxParticles(manager, ent->node->getTransformTRS().position);

	manager->add(box);
}

boxParticles::boxParticles(entityManager *manager, glm::vec3 pos)
	: entity(manager)
{
	static gameObject::ptr model = nullptr;

	//manager->registerComponent(this, "boxParticles", this);
	//manager->registerComponent(this, "updatable", this);
	manager->registerComponent(this, this);
	manager->registerInterface<updatable>(this, this);

	new timedLifetime(manager, this, 7.f);
	parts = std::make_shared<gameBillboardParticles>(32);

	for (unsigned i = 0; i < 32; i++) {
		velocities[i] = float(rand()) / RAND_MAX * 15.f;
		offsets[i]    = float(rand()) / RAND_MAX * 10.f;

		//parts->positions[i] = (glm::mat4(1));
		parts->positions[i] = (glm::vec4(1));
	}

	asdf = pos;
	parts->activeInstances = 32;
	parts->radius = 50.f;
	parts->update();

	if (!model) {
		//model = loadSceneAsyncCompiled(manager->engine, "assets/obj/emissive-cube.glb");
		auto [data, _] = loadSceneAsyncCompiled(manager->engine, DEMO_PREFIX "assets/obj/emissive-plane.glb");
		model = data;
	}

	setNode("model", parts, model);
	setNode("parts", node, parts);
};

void boxParticles::update(entityManager *manager, float delta) {
	time += delta;

	for (unsigned i = 0; i < 32; i++) {
		glm::vec3 pos =
			glm::vec3(sin(offsets[i] * time),
		              velocities[i] * time,
		              cos(offsets[i]*time))
			+ asdf;

		float scale = sin(time + offsets[i]);
		parts->positions[i] = glm::vec4(pos, scale);
		/*
		parts->positions[i] =
			glm::translate(pos)
			* glm::mat4_cast(glm::quat(glm::vec3(0, time, 0)))
			* glm::scale(glm::vec3(sin(time + offsets[i])));
			* */
	}

	parts->update();
}
