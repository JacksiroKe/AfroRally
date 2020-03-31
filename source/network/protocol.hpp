#pragma once

/**
 * @file
 * This file describes the protocol that is used for network communication.
 * It contains structures and serializations that are transmitted.
 *
 * Implementation notes:
 *   - First member must be uint8_t packet_type
 *   - Serialization is basically a naive memcpy, so use only primitive types
 */

#include <boost/lexical_cast.hpp>
#include "../vdrift/mathvector.h"
#include "../vdrift/quaternion.h"
#include "types.hpp"
#include "address.hpp"

namespace protocol {

// Check the comments in the packet structs to find out which one of these they affect.
const uint32_t GAME_PROTOCOL_VERSION = 4;
const uint32_t MASTER_PROTOCOL_VERSION = 6;

const unsigned DEFAULT_PORT = 4243;


/**
 * @brief Contains all possible message types.
 * It will be transmitted as 8-bit unsigned int.
 * On change, bump GAME_PROTOCOL_VERSION, MASTER_PROTOCOL_VERSION
 */
enum PacketType
{
	HANDSHAKE = 0,
	PING,
	PONG,
	PEER_ADDRESS,       // Packet for peer discovery
	PLAYER_INFO,        // Player attributes of the sender
	TEXT_MESSAGE,       // Text string that should be displayed somewhere
	CAR_UPDATE,         // Car state update
	GAME_LIST,          // Client requests master server to list games
	GAME_ACCEPTED,      // Master server sends response for newly accepted games
	GAME_STATUS,        // An available game (either client updates, or server reports)
	START_GAME,         // Signal to start loading the game
	START_COUNTDOWN,    // Signal that loading has finished, start race
	TIME_INFO,          // Lap / race time info
	RETURN_LOBBY        // Signal to go back to lobby
};


/**
 * @brief Contains error codes that tell the reason of disconnecting.
 */
enum ErrorCodes
{
	WRONG_PASSWORD = 1,
	INCOMPATIBLE_GAME_PROTOCOL,
	INCOMPATIBLE_MASTER_PROTOCOL
};

/**
 * @brief Contains authentication etc. info.
 * This stuff needs to be validated before accepting a new connection as a peer.
 * On change, bump GAME_PROTOCOL_VERSION
 */
struct HandshakePackage
{
	uint8_t packet_type;
	uint32_t master_protocol_version;
	uint32_t game_protocol_version;
	char password[16];

	HandshakePackage(std::string passwd = "")
		:packet_type(HANDSHAKE)
		,master_protocol_version(MASTER_PROTOCOL_VERSION)
		,game_protocol_version(GAME_PROTOCOL_VERSION)
	{
		memset(password, 0, sizeof(password));
		strcpy(password, passwd.c_str());
	}
};


/**
 * @brief Contains information about one game that is available for joining.
 * On change, bump GAME_PROTOCOL_VERSION, MASTER_PROTOCOL_VERSION
 */
struct GameInfo
{
	uint8_t packet_type;
	uint32_t id;        // Set by server
	uint32_t address;   // Set by server
	uint16_t port;      // Set by client
	uint32_t timestamp; // Set by server

	uint8_t players;    // Set by client, all below
	uint8_t collisions;
	uint8_t laps;
	uint8_t locked;    // game
	uint8_t reversed;  // track

	uint8_t flip_type;
	uint8_t boost_type;  float boost_power;
	char name[32], track[32], sim_mode[32];

	uint8_t start_order;
	uint8_t tree_collis;  float tree_mult;   // trees setup from host
	uint8_t damage_type, rewind_type;
	float damage_lap_dec, boost_lap_inc, rewind_lap_inc;  //todo

	GameInfo()
		:packet_type(GAME_STATUS), id(0), address(0), port(0), timestamp(0)
		,players(0), collisions(0), laps(0), locked(0), reversed(0)
		,flip_type(0), boost_type(0), boost_power(0.8f)
		,start_order(0)
		,tree_collis(0), tree_mult(1.f)
		,damage_type(0), rewind_type(0)
		,damage_lap_dec(0), boost_lap_inc(0), rewind_lap_inc(0)
	{
		name[0] = '\0';  track[0] = '\0';  track[31] = '\0';  sim_mode[0] = '\0';
	}

	bool operator==(const GameInfo& other) {  return id == other.id;  }
	bool operator!=(const GameInfo& other) {  return !(*this == other);  }
	operator bool() {  return id > 0;  }
};

typedef std::map<uint32_t, protocol::GameInfo> GameList;


/**
 * @brief Contains the address of a peer to connect.
 * These structs are passed around to create the complete network topography.
 * On change, bump GAME_PROTOCOL_VERSION
 */
struct PeerAddressPacket
{
	uint8_t packet_type;
	net::Address address;

	PeerAddressPacket(net::Address addr = net::Address())
		:packet_type(PEER_ADDRESS), address(addr)
	{	}

	bool operator==(const PeerAddressPacket& other) {  return address == other.address;  }
	bool operator!=(const PeerAddressPacket& other) {  return !(*this == other);  }
};


/**
 * @brief Contains player info.
 * These structs are passed around to update player information.
 * On change, bump GAME_PROTOCOL_VERSION
 */
struct PlayerInfoPacket
{
	uint8_t packet_type;
	int32_t random_id;
	char name[16];
	char car[10];
	char password[16];
	uint8_t peers;
	uint8_t ready;
	//TODO:  car colors (also change nick colors in game tab)
	//float car_hue, car_sat, car_val, car_gloss, car_refl;  // car color

	PlayerInfoPacket()
		:packet_type(PLAYER_INFO), random_id(-1), ready(), peers()
	{
		memset(name, 0, sizeof(name));
		memset(car, 0, sizeof(car));
		memset(password, 0, sizeof(password));
	}
};


/**
 * @brief Contains the car state.
 * These structs are passed around to update car position etc.
 * On change, bump GAME_PROTOCOL_VERSION
 */
struct CarStatePackage
{
	uint8_t packet_type;
	MATHVECTOR<float,3> pos;
	QUATERNION<float> rot;
	MATHVECTOR<float,3> linearVel;
	MATHVECTOR<float,3> angularVel;

	float steer;
	uint8_t trackPercent;
	uint8_t boost;
	uint8_t brake;
	//uint8_t damage;  //todo:..
	// boostFuel,gear, vel,rpm for replays ?..

	CarStatePackage()
		:packet_type(CAR_UPDATE)
		,steer(0.f),trackPercent(0),boost(0),brake(0)
	{	}
	operator bool()
	{	return packet_type == CAR_UPDATE;  }
};

typedef std::map<int8_t, CarStatePackage> CarStates;


/**
 * @brief Contains lap / race timing info.
 * On change, bump GAME_PROTOCOL_VERSION
 */
struct TimeInfoPackage
{
	uint8_t packet_type;
	uint8_t lap; // 0 for total time
	double time;

	TimeInfoPackage(uint8_t initLap, double initTime):
		packet_type(TIME_INFO), lap(initLap), time(initTime)
	{	}
};


} // namespace protocol
