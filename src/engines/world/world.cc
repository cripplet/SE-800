#include <stdio.h>
#include <unistd.h>
#include <iostream>

#include "world.h"
#include "../../classes/ship.h"

WorldEngine::WorldEngine(SchedulingEngine *scheduler) : scheduler(scheduler) {
	physics_engine = NULL;
	status = UNINITIALIZED;
}
WorldEngine::~WorldEngine() {}

std::vector<Team *> WorldEngine::get_teams() { return(this->teams); }

void WorldEngine::add_team(Team *team) { this->teams.push_back(team); }
void WorldEngine::del_team(Team *team) {}

void WorldEngine::win() {
	// do something for teams
	this->shutdown();
}

void WorldEngine::ignite() {
	this->status = INITIALIZED;
}

void WorldEngine::cycle() {
	this->status = RUNNING;
	this->scheduler->ignite();
	while(this->scheduler->get_signal()) {
		sleep(1);
	}
}

void WorldEngine::shutdown() {
	this->status = STOPPED;
	this->scheduler->shutdown();
}

/**
 * skeletal function -- should
 *
 * 1) pick an existing ship in physics environment
 * 2) assign pilot to ship with pilot p
 * 3) return ship object
 */
Ship *WorldEngine::join(Pilot *p) {
	std::vector<float> pos { 1, 1, 1 };
	Ship *s = new Ship(1, 10, 10, pos, 0, 0, 0);
	return(s);
}

void WorldEngine::set_physics_engine(PhysicsEngine *p) { this->physics_engine = p; }
PhysicsEngine *WorldEngine::get_physics_engine() { return(this->physics_engine); }
