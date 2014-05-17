#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>

#include "world.h"
#include "../../classes/ship.h"

int OBJ_ID = 1;

WorldEngine::WorldEngine(SchedulingEngine *scheduler) : scheduler(scheduler) {
	ai_engine = NULL;
	physics_engine = NULL;
	status = UNINITIALIZED;
}
WorldEngine::~WorldEngine() {}

std::vector<Team *> WorldEngine::get_teams() {
	std::vector<Team *> t;
	for(std::map<int, Team *>::iterator i = this->teams.begin(); i != this->teams.end(); ++i) {
		t.push_back((*i).second);
	}
	return(t);
}

void WorldEngine::add_team(Team *team) { this->teams[team->get_id()] = team; }
void WorldEngine::del_team(Team *team) {}

void WorldEngine::win() {
	// do something for teams
	this->shutdown();
}

void WorldEngine::ignite(char *mode) {
	this->status = INITIALIZED;
}

// settings
void WorldEngine::ignite(char *mode, int n_teams) {
	for(int i = 0; i < n_teams; i++) {
		this->add_team(new Team(i));
	}
}

void WorldEngine::cycle() {
	this->status = RUNNING;
	this->scheduler->ignite();
	while(this->scheduler->get_signal()) {
		sleep(1);
	}
}

void WorldEngine::shutdown() {
	this->signal = 0;
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
	std::vector<float> pos { (float) (rand() % 10 + 1), (float) (rand() % 10 + 1), (float) (rand() % 10 + 1) };
	Ship *s = new Ship(OBJ_ID, 10, 10, pos, 1000, 0, 0);
	std::cout << "SPAWNED NEW SHIP OF ID " << OBJ_ID << std::endl;
	OBJ_ID++;

	p->set_ship(s);
	this->ai_engine->add_pilot(p);

	return(s);
}

void WorldEngine::set_physics_engine(PhysicsEngine *p) { this->physics_engine = p; }
void WorldEngine::set_ai_engine(AIEngine *a) { this->ai_engine = a; }

PhysicsEngine *WorldEngine::get_physics_engine() { return(this->physics_engine); }
AIEngine *WorldEngine::get_ai_engine() { return(this->ai_engine); }
