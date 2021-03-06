#include "ai.h"

AIEngine::AIEngine() {}

void AIEngine::add_pilot(Pilot *p) {
	this->l.lock();
	if(p->get_is_player()) {
		this->players[p->get_id()] = p;
	} else {
		this->robots[p->get_id()] = p;
	}
	this->l.unlock();
}

void AIEngine::del_pilot(Pilot *p) {
	this->l.lock();
	this->players.erase(p->get_id());
	this->robots.erase(p->get_id());
	this->l.unlock();
}

void AIEngine::ignite() {}
void AIEngine::cycle() {
	// cycle through robots and calculate strategies
	// AQCUIRE PILOT SHIP_LOCK BEFORE CALCULATING STRATEGIES
	// RELEASE AFTERWARDS
}
void AIEngine::shutdown() {}
