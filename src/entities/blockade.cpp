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

		enemyModel->setScale(glm::vec3(1, 2, 1));
	}

	node->setPosition(position);

	manager->registerComponent(this, this);
	attach<health>(1.f, 1000);
	attach<worldHealthbar>();
	attach<syncRigidBodyXZVelocity>();
	attach<enemyCollision>();
	auto body = attach<rigidBodyBox>(position, 100.0,
	                                 AABBExtent(glm::vec3(0, 1, 0),
	                                            glm::vec3(0.5, 1, 0.5)));

	setNode("model", node, enemyModel);
	body->registerCollisionQueue(manager->collisions);
	body->phys->setAngularFactor(0.0);
}

blockade::~blockade() {

}
