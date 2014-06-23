// dummygraphics.cpp: provides stub implementation for the server




#include "common.h"
#include "engine.h"
#include "profile.h"
#include "tokenizer.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>

bool hasgraph = false;


void WallPart::draw(DrawManager& dm, double x1, double y1, double jx, double jy, double len) {
}

void Wall::draw(DrawManager& dm) {
}
void Plane::draw(DrawManager& dm, Area* area) {
}
void World::drawview (vect3d pt, Area* area, double ang, double vofs,
		      double xfov, double yfov, ActorFilter& filt)
{
}


void AreaTess::call() {
}

void AreaTess::release() {
}

void AreaTess::revalidate() {
}

void Area::draw(DrawManager& dm,  bool walls) {
}


void Plane::setup_texture_matrix(Texture* ti) {
}

