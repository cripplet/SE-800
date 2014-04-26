#ifndef _SHIP_H
#define _SHIP_H

#include <vector>

#include "../interfaces/weapon.h"
#include "../engines/physics/projectile.h"

/**
 * all ships directly implement this interface
 */

class Ship : public Projectile {
	public:
		virtual int get_weapon_index();			// get current weapon index

		virtual void del_weapon(Weapon *weapon);	// delete weapon from the ship
		virtual void add_weapon(Weapon *weapon);	// add a weapon to the ship
		virtual void sel_weapon(int index);		// sets currently active weapon

	private:
		Weapon *weapons;
		int weapon_index;		// current active weapon
		float max_v;			// maximum cruising speed of the ship -- limits set_v() (override function from IProjectile)
		float max_a;			// maximum acceleration of the ship -- limits set_a()
};

#endif
