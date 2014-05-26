#ifndef _MUNITION_H
#define _MUNITION_H

#define MUNITION_SIZE 1
#define MUNITION_VEL 100

#include <vector>

#include "../engines/physics/projectile.h"

/**
 * bullets and bombs would inherit from tihs interface
 */

class Munition : public Projectile {
	public:
		Munition(int id, float max_tolerance, std::vector<float> d, std::vector<float> v, std::vector<float> p, std::vector<float> r, std::vector<float> y, float aoe, int lifetime);	// spawn in bullet with appropriate speed and direction
		~Munition();

		/**
		 * if distance to surrounding ships < aoe, apply damage as returned by get_damage()
		 */
		void explode();																// override this

	private:
		float aoe;																// area of effect

		/**
		 * for ships near the munition at explosion, get damage output
		 *
		 * called in explode()
		 */
		float get_damage(float distance);													// override this
};

#endif
