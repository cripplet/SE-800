#include "graphics.h"
#include "../physics/projectile.h"
#include "../common/engine.h"
#include "../../classes/gameaudio.h"
#include "../../classes/control.h"
#include "../../classes/event.h"
#include "../../networks/client.h"

#include <iostream>
#include <osg/Node>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/Geode>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/Material>
#include <osg/Object>
#include <osg/Quat>
#include <osg/StateAttribute>
#include <osg/StateSet>
#include <osg/Texture2D>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/ShapeDrawable>
#include <osgDB/ReadFile>
#include <osgParticle/ExplosionEffect>
#include <osgParticle/ExplosionDebrisEffect>
#include <osgViewer/ViewerBase>
#include <ctime>
#include <math.h>

GraphicsEngine::GraphicsEngine(string color) {
	// Define main color of ship.
	if (color.compare("RED") == 0) {
		this->player_color = osg::Vec4(1, 0, 0, 1);
	} else if (color.compare("BLUE") == 0) {
		this->player_color = osg::Vec4(0, 1, 1, 1);
	} else if (color.compare("YELLOW") == 0) {
		this->player_color = osg::Vec4(1, 1, 0, 1);
	} else if (color.compare("GREEN") == 0) {
		this->player_color = osg::Vec4(0, 1, 0, 1);
	} else if (color.compare("ORANGE") == 0) {
		this->player_color = osg::Vec4(1, 0.5, 0, 1);
	} else {
		// Set ship to white if unknown color provided.
		this->player_color = osg::Vec4(1, 1, 1, 1);
	}
}

void GraphicsEngine::ignite() {
	this->ship_mesh = "../Assets/ship.obj";
	this->asteroid_mesh = "../Assets/asteroid.obj";
	this->bullet_mesh = "../Assets/bullet.obj";

	root = new osg::Group();
	render_world();
	set_light_source();
	ship_init();
	viewer_init();
	audio_init();
}

void GraphicsEngine::cycle() {

	if (this->viewer.done())
		shutdown();

	// check packet queue for updates
	// if found object and event packet, compare each object to list of rendered objects
	// for each object which is already rendered, set UpdateObjectCallback (simply a boolean) to true -- this boolean can be mapped to the object
	// for each object we need to render which is not already present, call a function which creates a new object and adds it to the group, sets its updatecallback, etc
	// for each object which was rendered but isn't anymore, remove this object from the group node and free its update callback and free the node
	// SO for each object we need a struct which encompasses the obj ID, the UpdateObjectCallback bool, the object Node, and perhaps the transform matrix?

	send_update_req();
	update_rendered_objects();
	update_camera();
	render();
	reset_rendered_objects();
}

void GraphicsEngine::shutdown() {
	this->audio->shutdown();
	this->net_utils->close_connection();
	exit(1);
}

osg::Node* GraphicsEngine::create_world_cube() {
	osg::Group* world_cube_root = new osg::Group();

	// Define height, width, and depth values of world cube.
	osg::Box* unit_cube = new osg::Box(osg::Vec3(0, 0, 0), 30000.0f);
	osg::ShapeDrawable* unit_cube_drawable = new osg::ShapeDrawable(unit_cube);
	osg::Geode* basic_shapes_geode = new osg::Geode();

	// Include space texture
	osg::Texture2D* space_face_texture = new osg::Texture2D;
	// Protect texture from being optimized away as a static state.
	space_face_texture->setDataVariance(osg::Object::DYNAMIC);
	osg::Image* space_face = osgDB::readImageFile("../Assets/space.jpg");
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
	protos::RenderedObj *ship = new protos::RenderedObj;

	// Wait for ship init packet to be received.
	while (1) {
		this->que_lock.lock();
		if (this->ship_que.size() > 0) {
			const protos::RenderedObj &ship_from_que = *this->ship_que.front();
			ship->CopyFrom(ship_from_que);
			this->ship_que.pop();
			this->que_lock.unlock();
			break;
		}
		this->que_lock.unlock();
	}

	GraphicsEngine::rendered_obj * ren_obj = create_object(*ship);
	this->main_ship = ren_obj;
}

void GraphicsEngine::viewer_init() {
	// Set up UI update callback
	this->ui_handler = new ClientControl::UIEventHandler(this->net_utils, this->audio, this->main_ship->trans_matrix);
	this->viewer.addEventHandler(this->ui_handler);

	// Assign the scene we created to the viewer
	this->viewer.setSceneData(this->root);
	// Create the windows and start the required threads
	this->viewer.realize();
}

void GraphicsEngine::audio_init() {
	this->audio = new GameAudio(this->root, &this->viewer);
	this->audio->init();
}

void GraphicsEngine::send_update_req() {
	// Send request to server for objs and events packet.
	int req = 1;
	NetPacket packet;
	PacketUtils::make_packet(&packet, PacketType::OBJS_AND_EVENTS_REQ, (void *) &req, NULL);
	if (!this->net_utils->send_to_server(&packet))
		std::cout << "Could not send objs and events req packet." << std::endl;
}

void GraphicsEngine::update_camera() {
	osg::Matrixd camera_matrix;
	osg::Matrixd cameraRotation;
	osg::Matrixd cameraTrans1;
	osg::Matrixd cameraTrans2;
	osg::Matrixd cameraTransOUT;
	osg::Matrixd transrotate;

	osg::Vec3f vpitch = this->main_ship->obj_pitch_vector;
	osg::Vec3f vroll = this->main_ship->obj_roll_vector;
	osg::Vec3f vyaw = this->main_ship->obj_yaw_vector;

	// Set tolerance on camera roll.
	float camera_roll = this->main_ship->acc_r;
	float camera_pitch = this->main_ship->acc_p;

	float pitch_x = vpitch.x();
	float pitch_y = vpitch.y();
	float pitch_z = vpitch.z();
	float roll_x = vroll.x();
	float roll_y = vroll.y();
	float roll_z = vroll.z();
	
	float mag = (1.0 / 40.0);

	

	float trans_cam_x = - roll_x / mag;
	float trans_cam_y = - roll_y / mag;
	float trans_cam_z = - roll_z / mag;


	cameraRotation.makeRotate(
		camera_pitch, vpitch, 		// roll
          	camera_roll, vroll,	 	// pitch
          	0, osg::Vec3f(0,0,1)); 		// heading 

	// cameraTrans.makeTranslate(cam_x, cam_y, cam_z);
	cameraTrans1.makeTranslate(trans_cam_x, trans_cam_y, trans_cam_z);
	transrotate.makeRotate(
		camera_pitch, vpitch,
		0, vroll,
		0, osg::Vec3f(0,0,1));
	
	//cameraTrans2 = transrotate * cameraTrans1;
	//cameraTrans1.makeScale(-1,-1,-1);
	//cameraTransOUT = cameraTrans1 + cameraTrans2;

	// camera_matrix = cameraRotation * cameraTrans;
	camera_matrix = cameraRotation * cameraTrans1;
	this->viewer.getCamera()->setViewMatrix((
		camera_matrix.inverse(camera_matrix) * osg::Matrixd::rotate(-M_PI / 2.0, 1, 0, 0)));
}

void GraphicsEngine::render() {
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
	//TODO: DETERMINE CORRECT LOCATION OF THE LIGHT SOURCE
	osg::Light *light = new osg::Light();
	light->setLightNum(0);
	light->setPosition(osg::Vec4(0.0, 0.0, 19000.0, 1.0));
	// Set the light color as white
	light->setDiffuse(osg::Vec4(1.0, 1.0, 1.0, 1.0));
	light->setSpecular(osg::Vec4(0.5, 0.5, 0.5, 1.0));
	// Set color of object shadows
	light->setAmbient(osg::Vec4(0.25, 0.25, 0.25, 1.0));

	osg::LightSource *l_source = new osg::LightSource();
	l_source->setLight(light);
	l_source->setLocalStateSetModes(osg::StateAttribute::ON);
	l_source->setStateSetModes(*(this->root->getOrCreateStateSet()), osg::StateAttribute::ON);

	osg::PositionAttitudeTransform *l_trans = new osg::PositionAttitudeTransform();
	l_trans->addChild(l_source);

	this->root->addChild(l_trans);
}

void GraphicsEngine::set_shader() {
}

GraphicsEngine::rendered_obj* GraphicsEngine::create_object(protos::RenderedObj obj) {
	osg::Node *node;
	osg::PositionAttitudeTransform* obj_transform =
		new osg::PositionAttitudeTransform();
	if (obj.type() == ObjType::SHIP) {
		node = osgDB::readNodeFile(this->ship_mesh);
		obj_transform->addChild(node);

		osg::StateSet* state_set = new osg::StateSet();
		osg::Material *mat = new osg::Material;
		mat->setDiffuse(osg::Material::FRONT, this->player_color);
//		mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4(1.0, 1.0, 1.0, 1.0));
//		mat->setAmbient(osg::Material::FRONT, osg::Vec4(0.25, 0.25, 0.25, 1.0));
//		mat->setEmission(osg::Material::FRONT, osg::Vec4(0.0, 0.0, 0.0, 1.0));
//		mat->setShininess(osg::Material::FRONT, 63.0);
		state_set->setAttribute(mat);
		obj_transform->setStateSet(state_set);


	} else if (obj.type() == ObjType::ASTEROID) {
		node = osgDB::readNodeFile(this->asteroid_mesh);
		obj_transform->addChild(node);
	} else if (obj.type() == ObjType::BULLET) {
		std::cout << "rendering bullet" << std::endl;
		node = osgDB::readNodeFile(this->bullet_mesh);
		obj_transform->addChild(node);
	} else {
		std::cout << "RECEIVE OBJECT OF UNKNOWN TYPE!!" << std::endl;
	}

	// Set position.
	protos::vector pos_vector = obj.pos();
	std::cout << pos_vector.x() << pos_vector.y() << pos_vector.z() << std::endl;
	osg::Vec3 obj_pos(pos_vector.x(), pos_vector.y(), pos_vector.z());
	obj_transform->setPosition(obj_pos);

	float orig_mesh_radius = obj_transform->getBound().radius();
	float desired_radius = obj.size();
	float scale_amt = 1 / (orig_mesh_radius / desired_radius);
	obj_transform->setScale(osg::Vec3(scale_amt, scale_amt, scale_amt));

	if (obj.type() == ObjType::SHIP) {
		// Ship is initally rotated such that camera sees its side -- it needs to be
		// rotated such that camera sees its back.
			/*
			obj_transform->setAttitude((osg::Quat(osg::DegreesToRadians(-90.0f),
			osg::Vec3d(0, 0, 1))) * (osg::Quat(osg::DegreesToRadians(20.0f + obj.pitch() * 180 / M_PI),
			osg::Vec3d(1, 0, 0))) * (osg::Quat(osg::DegreesToRadians(obj.roll() * 180 / M_PI),
			osg::Vec3d(0, 1, 0))));
			 */
			obj_transform->setAttitude(
				(
					osg::Quat(osg::DegreesToRadians(0.0f), osg::Vec3d(0, 0, 1))
				) * (
					osg::Quat(osg::DegreesToRadians(0.0f), osg::Vec3d(1, 0, 0))
				) * (
					osg::Quat(osg::DegreesToRadians(0.0f), osg::Vec3d(0, 1, 0))));
	} else if (obj.type() == ObjType::ASTEROID) {
			obj_transform->setAttitude((osg::Quat(osg::DegreesToRadians(0.0f),
			osg::Vec3d(0, 0, 1)))*(osg::Quat(osg::DegreesToRadians(obj.pitch() * 180 / M_PI),
			osg::Vec3d(1, 0, 0)))*(osg::Quat(osg::DegreesToRadians(obj.roll() * 180 / M_PI),
			osg::Vec3d(0, 1, 0))));
	}

	std::cout << "THIS IS BOUNDING SPHERE RADIUS " << obj_transform->getBound().radius() << std::endl;

	// Add object to rendered object list.
	rendered_obj *ren_obj = new rendered_obj;
	ren_obj->obj_roll = obj.roll();
	ren_obj->obj_pitch = obj.pitch();
	ren_obj->obj_yaw = obj.yaw();

	ren_obj->obj_size = obj.size();
	ren_obj->obj_pos = osg::Vec3(obj.pos().x(), obj.pos().y(), obj.pos().z());
	ren_obj->obj_roll_vector = osg::Vec3(obj.roll_vector().x(), obj.roll_vector().y(), obj.roll_vector().z());
	ren_obj->obj_pitch_vector = osg::Vec3(obj.pitch_vector().x(), obj.pitch_vector().y(), obj.pitch_vector().z());
	ren_obj->obj_yaw_vector = osg::Vec3(obj.yaw_vector().x(), obj.yaw_vector().y(), obj.yaw_vector().z());
	ren_obj->acc_p = obj.acc_p();
	ren_obj->acc_r = obj.acc_r();
	ren_obj->should_render = true;
	ren_obj->trans_matrix = obj_transform;
	cur_objs.insert(std::pair<int, rendered_obj*>(obj.id(), ren_obj));

	this->root->addChild(obj_transform);
	return ren_obj;
}

void GraphicsEngine::remove_object(rendered_obj *ren_obj) {
	root->removeChild(ren_obj->trans_matrix);
	ren_obj->trans_matrix = NULL;
	free(ren_obj);
}

void GraphicsEngine::update_object_transform(rendered_obj *ren_obj, protos::RenderedObj update_obj) {
	// Bring in vectors from object source
	ren_obj->obj_pitch = update_obj.pitch();
	ren_obj->obj_roll = update_obj.roll();
	ren_obj->obj_size = update_obj.size();
	ren_obj->obj_pos = osg::Vec3(update_obj.pos().x(), update_obj.pos().y(), update_obj.pos().z());
	ren_obj->obj_roll_vector = osg::Vec3(update_obj.roll_vector().x(), update_obj.roll_vector().y(), update_obj.roll_vector().z());
	ren_obj->obj_pitch_vector = osg::Vec3(update_obj.pitch_vector().x(), update_obj.pitch_vector().y(), update_obj.pitch_vector().z());
	ren_obj->obj_yaw_vector = osg::Vec3(update_obj.yaw_vector().x(), update_obj.yaw_vector().y(), update_obj.yaw_vector().z());
	ren_obj->acc_p = update_obj.acc_p();
	ren_obj->acc_r = update_obj.acc_r();
	ren_obj->should_render = true;

	// Set Position Object and matrix
	protos::vector pos_vector = update_obj.pos();
	osg::Vec3 obj_pos(pos_vector.x(), pos_vector.y(), pos_vector.z());
	ren_obj->trans_matrix->setPosition(osg::Vec3(obj_pos.x(), obj_pos.y(), obj_pos.z()));

	osg::Vec3 a = ren_obj->trans_matrix->getPivotPoint();

	ren_obj->trans_matrix->setPivotPoint(osg::Vec3(0,0,0));// obj_pos.x(), obj_pos.y(), obj_pos.z()));


	if (update_obj.type() == ObjType::SHIP) {
		ren_obj->trans_matrix->setAttitude((osg::Quat(osg::DegreesToRadians(-90.0f),
			osg::Vec3d(0, 0, 1)))*(osg::Quat(osg::DegreesToRadians(20.0f + ren_obj->acc_p * 180/M_PI),
			osg::Vec3d(1, 0, 0)))*(osg::Quat(osg::DegreesToRadians(ren_obj->acc_r * 180/M_PI),
			osg::Vec3d(0, 1, 0))));
	} else if (update_obj.type() == ObjType::ASTEROID) {
		ren_obj->trans_matrix->setAttitude((osg::Quat(osg::DegreesToRadians(0.0f),
			osg::Vec3d(0, 0, 1)))*(osg::Quat(osg::DegreesToRadians(ren_obj->acc_p),
			osg::Vec3d(1, 0, 0)))*(osg::Quat(osg::DegreesToRadians(ren_obj->acc_r),
			osg::Vec3d(0, 1, 0))));
	}

}

void GraphicsEngine::reset_rendered_objects() {
	for (std::map<int, rendered_obj *>::iterator i = cur_objs.begin(); i != cur_objs.end(); ++i) {
		(*i).second->should_render = false;
	}
}

void GraphicsEngine::update_rendered_objects() {
	// Find new objects and events packet
	protos::ObjsAndEventsPacket *packet = new protos::ObjsAndEventsPacket;
	this->que_lock.lock();
	if (this->objs_que.size() > 0) {
		// If graphics engine is more than 4 packets behind, dump all older packets
		// such that only 4 remain in queue
		while (this->objs_que.size() > 3) {
			this->objs_que.pop();
		}
		packet = this->objs_que.front();
		this->objs_que.pop();
	} else {
		this->que_lock.unlock();
		return;
	}
	this->que_lock.unlock();
	// Iterate through all rendered object entries in packet
	int updated = 0;
	std::cout << "RECEIVED " << packet->obj_size() << " objects " << std::endl;
	for (int i = 0; i < packet->obj_size(); ++i) {
		protos::RenderedObj *obj = new protos::RenderedObj;
		const protos::RenderedObj &obj_from_que = packet->obj(i);
		obj->CopyFrom(obj_from_que);
		if (cur_objs.count(obj->id())!= 0) {
			updated++;
			// Case when object has already been rendered in prev cycle.
			rendered_obj* ren_obj = cur_objs.at(obj->id());
			update_object_transform(ren_obj, *obj);
		} else {
			// Case when object is being rendered for the first time.
			create_object(*obj);
		}
	}

	// Process destroy events.
	for (int i = 0; i < packet->event_size(); ++i) {
		protos::Event event = packet->event(i);
		if (event.event_type() == EventType::DESTROY) {
			std::cout << "RECEIVED REQUEST TO DESTROY OBJECT OF ID " << event.id() << std::endl;
			// If event pertains to object which is not being rendered, ignore.
			if (cur_objs.count(event.id()) == 0)
				continue;
			osg::Vec3 explosion_pos = osg::Vec3(
				cur_objs.at(event.id())->obj_pos.x(),
				cur_objs.at(event.id())->obj_pos.y(),
				cur_objs.at(event.id())->obj_pos.z());
			float explosion_size = cur_objs.at(event.id())->obj_size;

			osgParticle::ExplosionEffect* explosion = new osgParticle::ExplosionEffect(explosion_pos, explosion_size);
			osgParticle::ExplosionDebrisEffect* explosion_debri = new osgParticle::ExplosionDebrisEffect(explosion_pos, explosion_size);
			audio->explosion_audio(explosion_pos);
			root->addChild(explosion);
			root->addChild(explosion_debri);
		}
	}

	// Do not render any object which was not sent in update packet.
	for (std::map<int, rendered_obj *>::iterator i = cur_objs.begin(); i != cur_objs.end(); ) {
		if ((*i).second->should_render == false) {
			rendered_obj* not_rendered_obj = (*i).second;
			i = cur_objs.erase(i);
			// Lose game scenario when main ship is destroyed
			// TODO: Put in another function
			if (not_rendered_obj == this->main_ship) {
				this->viewer.removeEventHandler(this->ui_handler);
			}
			remove_object(not_rendered_obj);
		} else {
			++i;
		}
	}

//	std::cout << "RENDERING " << updated << " OBJECTS" << std::endl;
}
