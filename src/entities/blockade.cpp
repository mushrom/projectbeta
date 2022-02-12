#include <grend/geometryGeneration.hpp>
#include <grend/gameEditor.hpp>

#include <components/health.hpp>
#include <components/healthbar.hpp>
#include <entities/enemyCollision.hpp>
#include <entities/projectile.hpp>
#include <entities/player.hpp>

#include "blockade.hpp"

blockade::blockade(entityManager *manager, const glm::vec3& position)
	: entity(manager)
{
	static gameObject::ptr enemyModel = nullptr;

	// TODO:
	if (!enemyModel) {
		//enemyModel = loadSceneAsyncCompiled(manager->engine, DEMO_PREFIX "assets/obj/test-enemy.glb");
		auto [data, _] = loadSceneAsyncCompiled(manager->engine, DEMO_PREFIX "assets/obj/smoothcube.glb");
		enemyModel = data;
		//sfx = openAudio(DEMO_PREFIX "assets/sfx/mnstr7.ogg");

		TRS transform = enemyModel->getTransformTRS();
		transform.scale = glm::vec3(1, 2, 1);
		enemyModel->setTransform(transform);
	}

	TRS transform = node->getTransformTRS();
	transform.position = position;
	node->setTransform(transform);

	manager->registerComponent(this, this);
	new health(manager, this, 1.f, 1000);
	new worldHealthbar(manager, this);
	new syncRigidBodyXZVelocity(manager, this);
	new enemyCollision(manager, this);
	auto body = new rigidBodyBox(manager, this, transform.position, 100.0, AABBExtent(glm::vec3(0, 1, 0), glm::vec3(0.5, 1, 0.5)));

	setNode("model", node, enemyModel);
	body->registerCollisionQueue(manager->collisions);
	body->phys->setAngularFactor(0.0);
}

blockade::~blockade() {

}
