#include "animationController.hpp"

void animationController::setAnimation(std::string animation, float weight) {
	if (!animations) return;

	auto it = animations->find(animation);
	//SDL_Log("setting animation '%s'!", animation.c_str());

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

animationController::animationController(gameImport::ptr objs) {
	// TODO: should copy this as part of duplicating a gameImport
	/*
	animations = std::make_shared<animationCollection>();

	for (auto& [name, anims] : objs->animations) {
		(*animations)[name] = anims;
	}
	*/

	animations = objs->animations;
	objects = objs;

#if 1
	if (animations) {
		for (auto& [name, _] : *animations) {
			SDL_Log("Have animation: '%s'", name.c_str());
		}

	} else {
		SDL_Log("No animations!");
	}
#endif
}

void animationController::bind(std::string name, animationMap::ptr anim) {
	if (!animations) {
		SDL_Log("No animation collection set!");
		return;
	}

	animations->insert({name, anim});
}

gameImport::ptr animationController::getObject(void) {
	return objects;
}


void animationController::update(float delta) {
	if (!currentAnimation) return;

	//float time = SDL_GetTicks() / 1000.f;
	animTime = fmod(animTime + delta*animSpeed, currentAnimation->endtime);
	applyAnimation(objects, currentAnimation, animTime);
}

void animationController::setSpeed(float speed) {
	animSpeed = speed;
}

