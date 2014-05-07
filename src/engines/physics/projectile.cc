#include "projectile.h"

#include <iostream>
#include <vector>

using namespace std;

Projectile::Projectile(int id, int clippable, float mass, float max_tolerance)
		: id(id), clippable(clippable), mass(mass), max_tolerance(max_tolerance) {}

// GET FUNCTIONS

int Projectile::get_id() {
	return this->id;
}

int Projectile::get_size() {
	return this->size;
}

int Projectile::get_mass() {
	return this->mass;
}

float Projectile::get_cur_tolerance() {
	return this->cur_tolerance;
}

float Projectile::get_max_tolerance() {
	return this->max_tolerance;
}

int Projectile::is_destroyed() {
	return this->destroyed;
}

int Projectile::is_clippable() {
	return this->clippable;
}

vector<float> Projectile::get_d() {
  return this->d;
}

vector<float> Projectile::get_v() {
	return this->v;
}

vector<float> Projectile::get_a() {
	return this->a;
}

vector<float> Projectile::get_p() {
	return this->p;
}

vector<float> Projectile::get_y() {
	return this->y;
}

vector<float> Projectile::get_r() {
	return this->r;
}

float Projectile::get_p_dot() {
	return this->p_dot;
}

float Projectile::get_y_dot() {
	return this->y_dot;
}

float Projectile::get_r_dot() {
	return this->r_dot;
}

// SET FUNCTIONS

void Projectile::set_d(vector<float> d) {
	this->d = d;
}

void Projectile::set_v(vector<float> v) {
	this->v = v;
}

void Projectile::set_a(vector<float> a) {
	this->a = a;
}

void Projectile::set_p(vector<float> p) {
	this->p = p;
}

void Projectile::set_y(vector<float> y) {
	this->y = y;
}

void Projectile::set_r(vector<float> r) {
	this->r = r;
}

void Projectile::set_p_dot(float p_dot) {
	this->p_dot = p_dot;
}

void Projectile::set_y_dot(float y_dot) {
	this->y_dot = y_dot;
}

void Projectile::set_r_dot(float r_dot) {
	this->r_dot = r_dot;
}
