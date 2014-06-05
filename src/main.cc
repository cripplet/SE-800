#include <stdio.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <pthread.h>
#include <ctime>

#include <signal.h>
#include <execinfo.h>

#include "engines/world/world.h"
#include "engines/physics/physics.h"
#include "engines/ai/ai.h"
#include "engines/graphics/graphics.h"
#include "networks/server.h"
#include "networks/client.h"
#include "engines/scheduling/scheduling.h"
#include "engines/scheduling/calendar.h"
#include "engines/cleanup/cleanup.h"
#include "classes/asteroid.h"

void handler(int sig) {
	void *array[100];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}


void usage(char *exec) {
	std::cout << "usage: " << exec << " ( server < port > | client < port > < ip > < color > )\n";
	exit(0);
}

int main(int argc, char **argv) {
	signal(SIGABRT, handler);

	if(argc < 3) {
		usage(argv[0]);
	} else if((argc == 4) && !strcmp(argv[1], "client")) {
		usage(argv[0]);
	}
	int port = std::stoi(argv[2]);

	// create empty world and schedule
	SchedulingEngine scheduler = SchedulingEngine();
	WorldEngine *world = new WorldEngine(&scheduler);
	std::thread game;
	std::thread network;

	// engines to start with associated calendars
	//	declare here to preserve scoping
	PhysicsEngine *p;
	Calendar *cal_p;

	AIEngine *a;
	Calendar *cal_a;

	CleanupEngine *c;
	Calendar *cal_c;

	GraphicsEngine *g;
	Calendar *cal_g;

	if(!strcmp(argv[1], "server")) {
		Environment *e = new  Environment({ 3000, 3000, 3000 }, { 10, 10, 10 });
		c = new CleanupEngine(e);
		Server *server = new Server(world, c);
		network = std::thread(&Server::accept_clients, server, (void *) &port);

		p = new PhysicsEngine();
		a = new AIEngine();
		cal_p = new Calendar(300, p);
		cal_a = new Calendar(.5, a);

		world->set_physics_engine(p);
		world->set_ai_engine(a);

		scheduler.add_calendar(cal_p);
		scheduler.add_calendar(cal_a);

		world->get_physics_engine()->set_environment(e);

		cal_c = new Calendar(30, c);
		scheduler.add_calendar(cal_c);

		srand(time(NULL));

		// Initializing asteroids (TODO): Move this to world engine ignite?
		for (int i = 0; i < 25; i++) {
			std::vector<float> pos { (float) (rand() % (50 - 10) + 10), (float) (rand() % (150 - 60) + 60), (float) (float) (rand() % (50 - 17) + 17) };
			world->obj_count_lock.lock();
			Asteroid *a = new Asteroid(world->obj_count, 1, 10, pos, (rand() % (4 - 1) + 1), 0, 0);
			world->obj_count++;
			world->obj_count_lock.unlock();
			world->get_physics_engine()->get_environment()->add_projectile(a);
		}


		for (int i = 0; i < 1; i++) {
			std::vector<float> pos { (float) 15, (float) 70, (float) 17 };
			world->obj_count_lock.lock();
			Asteroid *a = new Asteroid(world->obj_count, 1, 10, pos, (rand() % (4 - 1) + 1), 0, 0);
			world->obj_count++;
			world->obj_count_lock.unlock();
			world->get_physics_engine()->get_environment()->add_projectile(a);
		}

		world->ignite(argv[1], 2);
	} else if(!strcmp(argv[1], "client")) {
		string ip(argv[3]);
		string color(argv[4]);
		g = new GraphicsEngine(color);
		ClientNetUtils *c = new ClientNetUtils(&g->objs_que, &g->ship_que, &g->que_lock);

		if (!c->connect_to_server(port, ip)) {
			std::cout << "Could not connect to server. Exiting." << std::endl;
			exit(1);
		}
		network = std::thread(&ClientNetUtils::receive_from_server, c);

		g->net_utils = c;
		g->ignite();

		cal_g = new Calendar(300, g);
		scheduler.add_calendar(cal_g);

		world->ignite(argv[1]);
	}

	// execute the game
	game = std::thread(&WorldEngine::cycle, world);

	// wait for the network to shutdown when clients == 0
	network.join();

	// end the game
	world->shutdown();
	game.join();

	return(0);
}
