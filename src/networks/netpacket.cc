#include	<cstdint>
#include	<iostream>
#include	<map>
#include	<list>

#include	"../classes/ship.h"
#include	"netpacket.h"
#include	"packetprotos.pb.h"

using	namespace	std;

int	PacketUtils::get_packet_type(
	NetPacket	packet)	{
	protos::GeneralPacket	gen_packet;
	if	(!gen_packet.ParseFromString(packet.serialized_packet))	{
		cerr	<<	"Failed	to	parse	general	proto	buffer."	<<	endl;
	}	else	{
		return	gen_packet.packettype();
	}
	return	-1;
}

void	PacketUtils::get_packet_payload(
	NetPacket	packet,	void*	proto_packet)	{
	protos::GeneralPacket	gen_packet;
	int	type;
	if	(!gen_packet.ParseFromString(packet.serialized_packet))	{
		cerr	<<	"Failed	to	parse	general	proto	buffer."	<<	endl;
	}	else	{
		type	=	gen_packet.packettype();
	}
	if	(type	==	PacketType::SHIP_INIT)	{
		protos::ShipInitPacket	*ship_init_packet	=	(protos::ShipInitPacket	*)	proto_packet;
		if	(!ship_init_packet->ParseFromString(gen_packet.packet()))	{
			cerr	<<	"Failed	to	parse	ship	init	packet."	<<	endl;
		}
	}	else	if	(type	==	PacketType::OBJS_AND_EVENTS)	{
		protos::ObjsAndEventsPacket*	objs_and_events_packet	=	(protos::ObjsAndEventsPacket	*)	proto_packet;
		if	(!objs_and_events_packet->ParseFromString(gen_packet.packet()))	{
			cerr	<<	"Failed	to	parse	objs	and	events	packet."	<<	endl;
		}
	}	else	if	(type	==	PacketType::EVENT_ACK)	{
		protos::EventAckPacket*	event_ack_packet	=	(protos::EventAckPacket	*)	proto_packet;
		if	(!event_ack_packet->ParseFromString(gen_packet.packet()))	{
			cerr	<<	"Failed	to	parse	event	ack	packet."	<<	endl;
		}
	}	else	if	(type	==	PacketType::CONTROL_INPUT)	{
		protos::ControlInputPacket*	control_input_packet	=	(protos::ControlInputPacket	*)	proto_packet;
		if	(!control_input_packet->ParseFromString(gen_packet.packet()))	{
			cerr	<<	"Failed	to	parse	control	input	packet."	<<	endl;
		}
	}	else	{
		cerr	<<	"Incorrect	packet	type	parsed."	<<	endl;
	}
}

void	PacketUtils::make_packet(
	NetPacket	*packet,	PacketType	type,	void*	payload,	void*	extra_payload=NULL)	{
	protos::GeneralPacket	gen_packet;
	string	serialized_packet;

	if	(type	==	PacketType::SHIP_INIT)	{
		//	Read	argument
		Ship*	ship	=	(Ship	*)	payload;
		
		//	Fill	specific	proto	packet
		protos::ShipInitPacket	ship_init_packet;
		protos::RenderedObj	*ship_packet	=	ship_init_packet.mutable_ship();
		fill_obj_packet(ship_packet,	ship);
		
		//	ship_packet.set_weapon_index(ship->get_weapon_index());

		//	Serialize	specific	proto	packet
		if	(!ship_init_packet.SerializeToString(&serialized_packet))	{
			cerr	<<	"Could	not	serialize	ship	init	package	to	string."	<<	endl;
		}

	}	else	if	(type	==	PacketType::OBJS_AND_EVENTS)	{
		//	Read	arguments
		map<int,	Projectile*>	*objs	=	(map<int,	Projectile*>	*)	payload;
		//	TODO(DEFINE	EVENT	CLASS!!
		//list<Event>	*events;
		//if	(extra_payload	!=	NULL)	{
		//	events	=	(list<Event>	*)	extra_payload;
		//}
		
		//	Fill	specific	proto	packet
		protos::ObjsAndEventsPacket	objs_and_events_packet;
		
		for	(map<int,	Projectile*>::iterator	i	=	objs->begin();	i	!=	objs->end();	++i)	{
			protos::RenderedObj*	obj_packet	=	objs_and_events_packet.add_obj();
			fill_obj_packet(obj_packet,	&(*i->second));
			string	obj_type	=	typeid(*i->second).name();
			//	(TODO):	Set	weapon	type!`
			//if	(obj_type.compare("4Ship")	==	0)	{
			//	Ship	*ship	=	&(*i->second);
			//	obj_packet->set_weapon_index(ship->get_weapon_index());
			//}
		}
		
		//for	(list<Event>::iterator	i	=	events->begin();	i	!=	events->end();	++i)	{
		//	protos::Event*	event_packet	=	objs_and_events_packet.add_event();
		//	event_packet->set_event_type(*i.type);
		//}
		
		//	Serialize	specific	proto	packet
		if	(!objs_and_events_packet.SerializeToString(&serialized_packet))	{
			cerr	<<	"Could	not	serialize	objs	and	event	packet	to	string."	<<	endl;
		}
	}	else	if	(type	==	PacketType::EVENT_ACK)	{
		//	Read	arguments
		int	*ack	=	(int	*)	payload;
		
		//	Fill	specific	proto	packet
		protos::EventAckPacket	event_ack_packet;
		event_ack_packet.set_ack(*ack);

		//	Serialize	specific	proto	packet
		if	(!event_ack_packet.SerializeToString(&serialized_packet))	{
			cerr	<<	"Could	not	serialize	event	ack	package	to	string."	<<	endl;
		}
		
	}	else	if	(type	==	PacketType::CONTROL_INPUT)	{
		//(TODO):	fill	in	when	control	interface	is	done.
	}	else	{
		cerr	<<	"Incorrect	type	argument	specified."	<<	endl;
	}


	//	Fill	general	proto	packet
	gen_packet.set_packettype(type);
	gen_packet.set_packet(serialized_packet);

	//	Serialize	general	proto	packet
	string	serialized_gen_packet;
	if	(!gen_packet.SerializeToString(&serialized_gen_packet))	{
		cerr	<<	"Could	not	serialize	gen	packet	to	string."	<<	endl;
	}


	//	Construct	final	packet	
		//	Packet	length	is	composed	of	packet	size	(int),
		//	packet	type	(int),	and	a	serialized	payload	packet	(string)
	int	packet_size	=	sizeof(uint32_t)	+	sizeof(uint32_t)	+
				sizeof(gen_packet.packet());
	packet->packet_len	=	packet_size;
	packet->serialized_packet	=	serialized_gen_packet;
}

void	PacketUtils::fill_obj_packet(protos::RenderedObj	*obj_packet,	Projectile*	obj)	{

	obj_packet->set_id(obj->get_id());
	obj_packet->set_mass(obj->get_mass());
	obj_packet->set_cur_tolerance(obj->get_cur_tolerance());
	obj_packet->set_max_tolerance(obj->get_max_tolerance());
	obj_packet->set_is_destroyed(obj->is_destroyed());

	protos::vector	*pos_vector	=	obj_packet->mutable_pos();
	pos_vector->set_x(obj->get_d().at(0));
	pos_vector->set_y(obj->get_d().at(1));
	pos_vector->set_z(obj->get_d().at(2));

	protos::vector	*vel_vector	=	obj_packet->mutable_vel();
	vel_vector->set_x(obj->get_v().at(0));
	vel_vector->set_y(obj->get_v().at(1));
	vel_vector->set_z(obj->get_v().at(2));

  obj_packet->set_accel(obj->get_a());

	protos::vector	*pitch_vector	=	obj_packet->mutable_pitch();
	pitch_vector->set_x(obj->get_p().at(0));
	pitch_vector->set_y(obj->get_p().at(1));
	pitch_vector->set_z(obj->get_p().at(2));

	protos::vector	*roll_vector	=	obj_packet->mutable_roll();
	roll_vector->set_x(obj->get_r().at(0));
	roll_vector->set_y(obj->get_r().at(1));
	roll_vector->set_z(obj->get_r().at(2));

	protos::vector	*yaw_vector	=	obj_packet->mutable_yaw();
	yaw_vector->set_x(obj->get_y().at(0));
	yaw_vector->set_y(obj->get_y().at(1));
	yaw_vector->set_z(obj->get_y().at(2));
}
