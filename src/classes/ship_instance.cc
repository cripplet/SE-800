#include "ship_instance.h"

ShipCarrier::ShipCarrier(int id, std::vector<float> d) : Ship(id, 1000, 400, d, 1, 100, .5, .5) {}
ShipScout::ShipScout(int id, std::vector<float> d) : Ship(id, 200, 100, d, .2, 500, 1, 1) {}
ShipFighter::ShipFighter(int id, std::vector<float> d) : Ship(id, 400, 200, d, .4, 200, 5, 5) {}
