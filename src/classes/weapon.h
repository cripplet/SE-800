#ifndef _WEAPON_H
#define _WEAPON_H

#include <string>
#include <vector>

#include "../classes/munition.h"

/**
 * all weapons should inherit from this class
 */

class Weapon {
	public:
		Weapon(std::string name, int max_ammo, std::vector<float> offset, float rate, int munition_type);
		~Weapon();

		std::string get_name();
		int get_max_ammo();			// get maximum ammo
		int get_cur_ammo();			// get current ammo
		float get_rate();			// get the rate of fire
		Munition *get_munition();		// get munition instance

		/**
		 * creates an instance of the munition and sets instance d, r and v
		 *
		 * delays in some way the firing if
		 *	current_time - last_fire < rate
		 *
		 * set last to update the last time a munition is fired and decrement ammo
		 */
		void fire(std::vector<float> d, std::vector<float> r, std::vector<float> v);

	private:
		std::string name;			// weapon name -- set in constructor
		int max_ammo;
		int cur_ammo;
		std::vector<float> offset;		// given the ship's position, calculate the position at which the munition will spawn

		float rate;				// minimum time between shots
		float last;				// last time that a munition was fired
		int munition_type;
};

#endif
