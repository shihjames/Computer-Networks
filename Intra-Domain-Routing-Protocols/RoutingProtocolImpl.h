#ifndef ROUTINGPROTOCOLIMPL_H
#define ROUTINGPROTOCOLIMPL_H

#include "RoutingProtocol.h"

// Define a struct to store information about a routing table entry
struct RoutingTableEntry
{
	int nextPort;		// Port to reach destination
	int cost;			// Cost to reach destination
	int lastUpdateTime; // Time when the entry was last updated

	// Constructor to initialize the entry with the given values
	RoutingTableEntry(int port, int cost, int updateTime)
	{
		this->nextPort = port;
		this->cost = cost;
		this->lastUpdateTime = updateTime;
	}
};

// Define the constructor for the RoutingTableEntry struct
// RoutingTableEntry::RoutingTableEntry(int port, int cost, int updateTime)
// {
// 	this->nextPort = port;
// 	this->cost = cost;
// 	this->lastUpdateTime = updateTime;
// }

struct PortStatus
{
	int toRouterID;
	int costToNeighbor;
	int lastPongTime;

	PortStatus(int routerID, int costToNeighbor, int pongTime)
	{
		this->toRouterID = routerID;
		this->costToNeighbor = costToNeighbor;
		this->lastPongTime = pongTime;
	}
};

// PortStatus::PortStatus(int routerID, int costToNeighbor, int pongTime)
// {
// 	this->toRouterID = routerID;
// 	this->costToNeighbor = costToNeighbor;
// 	this->lastPongTime = pongTime;
// }

class RoutingProtocolImpl : public RoutingProtocol
{
public:
	RoutingProtocolImpl(Node *n);
	~RoutingProtocolImpl();

	void init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type);
	// As discussed in the assignment document, your RoutingProtocolImpl is
	// first initialized with the total number of ports on the router,
	// the router's ID, and the protocol type (P_DV or P_LS) that
	// should be used. See global.h for definitions of constants P_DV
	// and P_LS.

	void handle_alarm(void *data);
	// As discussed in the assignment document, when an alarm scheduled by your
	// RoutingProtoclImpl fires, your RoutingProtocolImpl's
	// handle_alarm() function will be called, with the original piece
	// of "data" memory supplied to set_alarm() provided. After you
	// handle an alarm, the memory pointed to by "data" is under your
	// ownership and you should free it if appropriate.

	void recv(unsigned short port, void *packet, unsigned short size);
	// When a packet is received, your recv() function will be called
	// with the port number on which the packet arrives from, the
	// pointer to the packet memory, and the size of the packet in
	// bytes. When you receive a packet, the packet memory is under
	// your ownership and you should free it if appropriate. When a
	// DATA packet is created at a router by the simulator, your
	// recv() function will be called for such DATA packet, but with a
	// special port number of SPECIAL_PORT (see global.h) to indicate
	// that the packet is generated locally and not received from
	// a neighbor router.

	void ping(unsigned short router_id, int32_t sending_time, unsigned short port_id);
	void pong(unsigned short port_id, char *pkt);
	void recv_pong_pkt(unsigned short pport_idort, char *pkt);
	void recv_regular_pkt(unsigned short port_id, char *pkt);
	void recv_DV_pkt(unsigned short port_id, char *pkt);
	void recv_LS_pkt(unsigned short port_id, char *pkt);
	void update_dv_table();
	void router_check();
	void port_check();
	bool update_rt(int pid, int preCost, int newCost);
	bool update_rt(int pid, unordered_map<int, int> &updatedDV);

private:
	Node *sys; // To store Node object; used to access GSR9999 interfaces
	int32_t pkt_size;
	unsigned short numOfPorts;
	unsigned short routerId;
	eProtocolType protocolType;
	vector<PortStatus *> portTable;
	unordered_map<int, RoutingTableEntry *> routingTable;
};

#endif
