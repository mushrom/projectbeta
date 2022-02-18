#pragma once

#include <grend/gameObject.hpp>
#include <grend/animation.hpp>

using namespace grendx;

class animationController {
	public:
		typedef std::shared_ptr<animationController> ptr;
		typedef std::weak_ptr<animationController>   weakptr;

		animationController(gameImport::ptr objs);
		void setAnimation(std::string animation, float weight = 1.0);
		gameImport::ptr getObject(void);
		void update(float delta);

		// bind an animation to a name
		// the animation can be from another animation collection
		void bind(std::string name, animationMap::ptr anim);
		void setSpeed(float speed);

	private:
		float animTime = 0;
		float animSpeed = 1.0;

		animationCollection::ptr animations;
		animationMap::ptr currentAnimation;
		gameImport::ptr objects;
};

void applyAnimation(gameObject::ptr node, animationMap::ptr anim, float time);
