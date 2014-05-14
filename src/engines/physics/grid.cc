#include <cstddef>
#include "grid.h"

Grid::Grid(int id, std::vector<float> corner, std::vector<float> size) : id(id), corner(corner), size(size) {}

/**
 * returns a list of all projectiles within a grid
 *
 * rather expensive -- unless necessary, use get_clippable() and get_unclippable() and manually iterate through the map, calling iterator->second
 *	(as any immediately data-access pattern for get_projectiles() is also to manually iterate through the vector anyways)
 */
std::vector<Projectile *> Grid::get_projectiles() {
	std::vector<Projectile *> p;
	for(std::map<int, Projectile *>::iterator i = this->clippable.begin(); i != this->clippable.end(); ++i) {
		p.push_back(i->second);
	}
	for(std::map<int, Projectile *>::iterator i = this->unclippable.begin(); i != this->unclippable.end(); ++i) {
		p.push_back(i->second);
	}
	return(p);
}

Projectile *Grid::get_projectile(int id) {
	std::map<int, Projectile *>::iterator target = this->clippable.find(id);
	if(target == this->clippable.end()) {
		target = this->unclippable.find(id);
		if(target == this->unclippable.end()) {
			return(NULL);
		}
	}
	return(target->second);
}

std::map<int, Projectile *> Grid::get_clippable() { return(this->clippable); }
std::map<int, Projectile *> Grid::get_unclippable() { return(this->unclippable); }

std::vector<float> Grid::get_size() { return(this->size); }
std::vector<float> Grid::get_corner() { return(this->corner); }

void Grid::add_projectile(Projectile *projectile) {
	if(projectile->is_clippable()) {
		this->clippable[projectile->get_id()] = projectile;
	} else {
		this->unclippable[projectile->get_id()] = projectile;
	}
}

void Grid::del_projectile(Projectile *projectile) {
	this->clippable.erase(projectile->get_id());
	this->unclippable.erase(projectile->get_id());
}
