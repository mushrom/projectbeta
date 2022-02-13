#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>

#include <components/health.hpp>
#include <components/healthbar.hpp>
#include <entities/enemyCollision.hpp>
#include <entities/projectile.hpp>
#include <entities/player.hpp>

#include "generator.hpp"

generator::generator(entityManager *manager, const glm::vec3& position)
	: entity(manager)
{
	static gameObject::ptr enemyModel = nullptr;

	// TODO:
	if (!enemyModel) {
		//enemyModel = loadSceneAsyncCompiled(manager->engine, DEMO_PREFIX "assets/obj/test-enemy.glb");
		auto [data, _] = loadSceneAsyncCompiled(manager->engine, DEMO_PREFIX "assets/obj/potted-plant.glb");
		enemyModel = data;
		//sfx = openAudio(DEMO_PREFIX "assets/sfx/mnstr7.ogg");

		enemyModel->setScale(glm::vec3(0.25));
	}

	node->setPosition(position);

	manager->registerComponent(this, this);
	attach<health>();
	attach<worldHealthbar>();
	attach<syncRigidBodyXZVelocity>();
	attach<enemyCollision>();
	auto body = attach<rigidBodySphere>(position, 1.0, 1.0);

	setNode("model", node, enemyModel);
	body->registerCollisionQueue(manager->collisions);
	body->phys->setAngularFactor(0.0);
}

generator::~generator() {

}
