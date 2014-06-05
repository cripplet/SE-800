#ifndef _PROJECTILE_H
#define _PROJECTILE_H

/**
 * base interface for all objects which needs to be simulated
 */

#include <vector>

enum ObjType {
	SHIP,
	ASTEROID,
	BULLET,
	BOMB
};

class Projectile {
	public:
		Projectile(int id, int is_clippable, float mass, float max_tolerance, std::vector<float> d, std::vector<float> v, float size, std::vector<float> p, std::vector<float> r, std::vector<float> y, float p_dot, float r_dot, float preset_a, float preset_p_dot, float preset_r_dot, int lifetime);

		/**
		 * public-facing functions which can be freely queried
		 */

		int get_id();				// get projectile ID
		float get_size();			// get size
		int get_mass();				// get mass
		int get_type();				// get obj type

		float get_cur_tolerance();		// get current health
		float get_max_tolerance();		// get max health

		int get_is_destroyed();			// return cur_health < 0
		int get_is_clippable();			// returns if the object is clippable OR if object is destroyed
		int get_is_processed();			// returns true if the object has been acknowledged by the network

		std::vector<float> get_d();		// get position
		std::vector<float> get_v();		// get velocity
		float get_a();				// get acceleration

		float get_p_float();			// return in rad the pitch relative to { 0, 0, 1 }
		float get_y_float();			// return in rad the yaw relative to { 0, 1, 0 }
		float get_r_float();			// return in rad the roll relative to { 1, 0, 0 }

		float tot_p_rot;
		float tot_r_rot;
		float tot_y_rot;

		float acc_p;
		float acc_r;

		void set_acc_p(float pangle);
		void set_acc_r(float rangle);
		float get_acc_p();
		float get_acc_r();

		std::vector<float> get_p();		// get pitch axis
		std::vector<float> get_y();		// get yaw axis
		std::vector<float> get_r();		// get roll axis

		float get_p_dot();			// get pitch rotation speed (x < 0 => counterclockwise)
		float get_r_dot();			// get roll rotation speed

		/**
		 * get the preset values of the projectile
		 */
		float get_preset_a();
		float get_preset_p_dot();
		float get_preset_r_dot();
		float get_preset_max_vel();

		int get_lifetime();

		/**
		 * the following should not be called outside the physics engine
		 */

		void set_d(std::vector<float> d);	// set position
		void set_v(std::vector<float> v);	// set velocity
		void set_a(float a);			// set acceleration

		/**
		 * unit orientation vectors
		 */
		void set_p(std::vector<float> p);	// set pitch axis	(0, 1, 0)
		void set_y(std::vector<float> y);	// set yaw axis		(0, 0, 1)
		void set_r(std::vector<float> r);	// set roll axis	(1, 0, 0)

		void set_p_dot(float p_dot);		// set pitch rotation speed
		void set_r_dot(float r_dot);		// set roll rotation speed

		void set_is_processed();		// can only be set to TRUE by the network -- the network should then proceed to BROADCAST event to all clients

		void set_lifetime();

		void damage(float tolerance);		// decrement the object tolerance by the input

	protected:
		int type;				// object type (ship or asteroid)

	private:
		int id;					// global unique ID -- set on construction
		int is_destroyed;
		int is_clippable;			// clippable objects do not check for collision with other clippable objects
		int is_processed;			// set externally to TRUE upon being acknowledged as destroyed by network

		float mass;				// mass of object -- set on construction

		float cur_tolerance;			// current health -- initialized to max_tolerance
		float max_tolerance;			// max health -- set on construction

		/* positional-related vectors */
		std::vector<float> d;
		std::vector<float> v;

		float size;				// collision radius
		float a;

		/* pitch, yaw, roll vectors */
		std::vector<float> p;
		std::vector<float> y;
		std::vector<float> r;
		float p_dot;
		float r_dot;

		float preset_a;
		float preset_p_dot;
		float preset_r_dot;
		float preset_max_vel;

		int lifetime;				// number of ticks to simulate in the engine -- set to -1 for infinite lifetime (until destroyed)
};

#endif
