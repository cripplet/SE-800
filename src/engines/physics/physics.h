#ifndef _PHYSICSENGINE_H
#define _PHYSICSENGINE_H

#include "../../engines/common/engine.h"
#include "../../engines/physics/environment.h"
#include <cmath>

class PhysicsEngine : public Engine {
	public:
		PhysicsEngine();
		~PhysicsEngine();

		void set_environment(Environment *environment);

		/**
		 * public-facing functions -- given an object ID, modify the object in the world
		 */
		void toggle_a(int id, int vala);
		void toggle_p_dot(int id, float valp);
		void toggle_r_dot(int id, float valr);

	private:
		Environment *environment;
		void apply_rotation(float angle, int flag, Projectile *p);
		void verlet_step(float t, Projectile *p);

};

#endif
