#include "graphics.h"
#include "../physics/projectile.h"
#include "../common/engine.h"
#include "../../classes/control.h"
#include "../../networks/client.h"

#include <iostream>
#include <osg/Node>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <osg/Object>
#include <osg/Quat>
#include <osg/StateAttribute>
#include <osg/StateSet>
#include <osg/Texture2D>
#include <osg/Vec3>
#include <osg/ShapeDrawable> 
#include <osgDB/ReadFile>
#include <osgViewer/ViewerBase>
#include <ctime>

GraphicsEngine::GraphicsEngine() {}

void GraphicsEngine::ignite() {
	root = new osg::Group();
	render_world();
	ship_init();
	viewer_init();
}

void GraphicsEngine::cycle() {

	if (this->viewer.done())
		shutdown();

	//clock_t time1 = clock();
	// check packet queue for updates
	// if found object and event packet, compare each object to list of rendered objects
	// for each object which is already rendered, set UpdateObjectCallback (simply a boolean) to true -- this boolean can be mapped to the object
	// for each object we need to render which is not already present, call a function which creates a new object and adds it to the group, sets its updatecallback, etc
	// for each object which was rendered but isn't anymore, remove this object from the group node and free its update callback and free the node
	// SO for each object we need a struct which encompasses the obj ID, the UpdateObjectCallback bool, the object Node, and perhaps the transform matrix?
	update_rendered_objects();
	update_camera();
	render_objects();

	//clock_t time2 = clock();
	//clock_t timediff = time2 - time1;
	//float timediff_sec = ((float)timediff) / CLOCKS_PER_SEC;
	//std::cout << "CYCLE LENGTH: " << timediff_sec << "SECONDS" << std::endl;
}

void GraphicsEngine::shutdown() {
	exit(1);
}

void GraphicsEngine::fill_cur_objects(Projectile *projectiles) {
}

osg::Node* GraphicsEngine::create_world_cube() {
	osg::Group* world_cube_root = new osg::Group();

	// Define height, width, and depth values of world cube.
	osg::Box* unit_cube = new osg::Box(osg::Vec3(0, 0, 0), 100.0f);
	osg::ShapeDrawable* unit_cube_drawable = new osg::ShapeDrawable(unit_cube);
	osg::Geode* basic_shapes_geode = new osg::Geode();

	// Include space texture
	osg::Texture2D* space_face_texture = new osg::Texture2D;
	// Protect texture from being optimized away as a static state.
	space_face_texture->setDataVariance(osg::Object::DYNAMIC);
	osg::Image* space_face = osgDB::readImageFile("space.jpg");
	if (!space_face) {
		std::cout << "Couldn't find texture space.jpg" << std::endl;
	}
	space_face_texture->setImage(space_face);

	osg::StateSet* state_set = new osg::StateSet();
	// Assign texture unit 0 of the state set to the texture we just
	// created and enable the texture
	state_set->setTextureAttributeAndModes(
		0, space_face_texture, osg::StateAttribute::ON);
	basic_shapes_geode->setStateSet(state_set);
	basic_shapes_geode->addDrawable(unit_cube_drawable);
	world_cube_root->addChild(basic_shapes_geode);

	return world_cube_root;
}

void GraphicsEngine::render_world() {
	osg::Node* world_cube = create_world_cube();
	osg::PositionAttitudeTransform* world_transform =
		new osg::PositionAttitudeTransform();
	world_transform->addChild(world_cube);
	world_transform->setPosition(osg::Vec3(0, 13, 10));

	this->root->addChild(world_transform);
}

void GraphicsEngine::ship_init() {
	protos::RenderedObj *ship;

	// Wait for ship init packet to be received.
	while (1) {
		this->que_lock.lock();
		if (this->ship_que.size() > 0) {
			ship = this->ship_que.front();
			this->ship_que.pop();
			this->que_lock.unlock();
			break;
		}
		this->que_lock.unlock();
	}

	this->main_ship = *ship;
	osg::Node* ship_mesh;
	ship_mesh = osgDB::readNodeFile("ship.obj");
	osg::PositionAttitudeTransform* main_ship_transform =
		new osg::PositionAttitudeTransform();
	main_ship_transform->addChild(ship_mesh);

	// Set main ship position.
	// TODO: Set main ship tilts.
	protos::vector pos_vector = ship->pos();
	std::cout << pos_vector.x() << pos_vector.y() << pos_vector.z() << std::endl;
	osg::Vec3 ship_pos(pos_vector.x(), pos_vector.y(), pos_vector.z());
	main_ship_transform->setAttitude((osg::Quat(osg::DegreesToRadians(-90.0f),
		osg::Vec3d(0, 0, 1)))*(osg::Quat(osg::DegreesToRadians(25.0f),
		osg::Vec3d(1, 0, 0))));
	main_ship_transform->setPosition(ship_pos);

	// Add main ship to rendered object list.
	rendered_obj* obj = new rendered_obj;
	obj->obj = ship;
	obj->update_pos = false;
	obj->node = ship_mesh;
	obj->trans_matrix = main_ship_transform;
	cur_ships.insert(std::pair<int, rendered_obj*>(ship->id(), obj));

	this->root->addChild(main_ship_transform);
}

void GraphicsEngine::viewer_init() {
	// Set up UI update callback
	ClientControl::UIEventHandler* ui_handler = new ClientControl::UIEventHandler(this->net_utils);
	this->viewer.addEventHandler(ui_handler);

	// Assign the scene we created to the viewer
	this->viewer.setSceneData(this->root);
//	this->viewer.setThreadingModel(osgViewer::ViewerBase::ThreadingModel::SingleThreaded);
	// Create the windows and start the required threads
	this->viewer.realize();
}

void GraphicsEngine::update_camera() {
	osg::Matrixd camera_matrix;
	osg::Matrixd camera_rotation;
	osg::Matrixd camera_trans;

	// TODO: Update camera based on ship tilts.
	camera_rotation.makeRotate(
		osg::DegreesToRadians(0.0), osg::Vec3(0, 1, 0),	// roll
		osg::DegreesToRadians(0.0), osg::Vec3(1, 0, 0),	// pitch
		osg::DegreesToRadians(0.0), osg::Vec3(0, 0, 1));	// heading

	camera_trans.makeTranslate(this->main_ship.pos().x()-1, this->main_ship.pos().y()-27, this->main_ship.pos().z()+8);
	camera_matrix = camera_rotation * camera_trans;
	osg::Matrixd inverse = camera_matrix.inverse(camera_matrix);
	this->viewer.getCamera()->setViewMatrix((
		camera_matrix.inverse(camera_matrix) * osg::Matrixd::rotate(-M_PI/2.0, 1, 0, 0)));
}

void GraphicsEngine::render_objects() {
	if (!this->viewer.done()) {
		// Dispatch the new frame, this wraps the following viewer operations:
		// 	advance() to the new frame
		//	eventTraversal() that collects events and passes them on to the 
		// event handlers and event callbacks
		// 	updateTraversal() to call the update callbacks
		//	renderingTraversals() that synchornizes all the rendering threads
		// (if any) and dispatch cull, draw, and swap buffers

		this->viewer.frame();
	}
}

void GraphicsEngine::set_light_source() {
}

void GraphicsEngine::set_shader() {
}

void GraphicsEngine::update_rendered_objects() {
	// TODO: Scan for deleted obj events and update nodes.

	// Find new objects and events packet
	protos::ObjsAndEventsPacket *packet = new protos::ObjsAndEventsPacket;
	this->que_lock.lock();
	if (this->objs_que.size() > 0) {
		packet = this->objs_que.front();
		this->objs_que.pop();
	} else {
		this->que_lock.unlock();
		return;
	}
	this->que_lock.unlock();
	// Iterate through all rendered object entries in packet
	std::cout << "NUMBER OF OBJECTS IS " << packet->obj_size() << std::endl;
	for (int i = 0; i < packet->obj_size(); ++i) {
		protos::RenderedObj obj = packet->obj(i);
		if (obj.type() == ObjType::SHIP) {
			// Case when object has already been rendered in prev cycle.
			if (cur_ships.count(obj.id())!= 0) {
				rendered_obj* rendered_ship = cur_ships.at(obj.id());
				rendered_ship->update_pos = true;
				// FREEING OLD OBJECT? HOW WILL THIS WORK???
				rendered_ship->obj = &obj;

				// TODO: MOVE ALL OF THESE UPDATES TO CALLBACK FXN
				protos::vector pos_vector = obj.pos();
				osg::Vec3 ship_pos(pos_vector.x(), pos_vector.y(), pos_vector.z());
				rendered_ship->trans_matrix->setPosition(ship_pos);
				// TODO: UPDATE TILT
			} else {
				// Case when object has not yet been rendered.
				osg::Node* ship_mesh;
				ship_mesh = osgDB::readNodeFile("ship.obj");
				osg::PositionAttitudeTransform* ship_transform =
					new osg::PositionAttitudeTransform();
				ship_transform->addChild(ship_mesh);

				// Set main ship position.
				// TODO: Set main ship tilts.
				protos::vector pos_vector = obj.pos();
				std::cout << pos_vector.x() << pos_vector.y() << pos_vector.z() << std::endl;
				osg::Vec3 ship_pos(pos_vector.x(), pos_vector.y(), pos_vector.z());
				ship_transform->setAttitude((osg::Quat(osg::DegreesToRadians(-90.0f),
					osg::Vec3d(0, 0, 1)))*(osg::Quat(osg::DegreesToRadians(25.0f),
					osg::Vec3d(1, 0, 0))));
				ship_transform->setPosition(ship_pos);

				// Add main ship to rendered object list.
				rendered_obj *ren_obj = new rendered_obj;
				ren_obj->obj = &obj;
				ren_obj->update_pos = false;
				ren_obj->node = ship_mesh;
				ren_obj->trans_matrix = ship_transform;
				cur_ships.insert(std::pair<int, rendered_obj*>(obj.id(), ren_obj));

				this->root->addChild(ship_transform);
			}
		} else if (obj.type() == ObjType::ASTEROID) {
		} else {
			std::cout << "Received projectile of unknown type." << std::endl;
		}
	}
}
