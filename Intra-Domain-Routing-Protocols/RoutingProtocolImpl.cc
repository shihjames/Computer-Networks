#include <arpa/inet.h>
#include "RoutingProtocolImpl.h"
#include "Node.h"

/* Remember to implement POISON REVERSE */

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n)
{
	sys = n;
	pkt_size = 12;
}

RoutingProtocolImpl::~RoutingProtocolImpl()
{
	// add your own code (if needed)
}

void RoutingProtocolImpl::init(unsigned short numOfPorts, unsigned short routerId, eProtocolType protocolType)
{
	/* CHELSEA */

	/* INIT ROUTER INFO */
	// 1. Number of ports
	// 2. Your router ID
	// 3. Routing protocol used (P DV or P LS)
	this->numOfPorts = numOfPorts;
	this->routerId = routerId;
	this->protocolType = protocolType;

	/* INIT ROUTING TABLE */
	for (auto i = 0; i < this->numOfPorts; i++)
	{
		this->portTable.push_back(new PortStatus(SPECIAL_PORT, INFINITY_COST, 0));
	}

	/* SET UP ALARM */
	// 1. alarm for periodic check every second
	int *periodicData = new int;
	*periodicData = 1;
	sys->set_alarm(this, 1000, periodicData);

	// 2. alarm for ping every 10 seconds
	int *pingData = new int;
	*pingData = 2;
	sys->set_alarm(this, 10000, pingData);

	// first ping
	for (auto i = 0; i < (int)portTable.size(); i++)
	{
		ping(routerId, sys->time(), i);
	}

	// 3. alarm for routing table update every 30 seconds
	int *routeTableData = new int;
	*routeTableData = 3;
	sys->set_alarm(this, 30000, routeTableData);
}

void RoutingProtocolImpl::handle_alarm(void *data)
{
	/* CHELSEA */

	/* TIMES UP AND DO THE CHECK */
	// alarm type 
	// 1:periodic 
	// 2:ping 
	// 3:routing table update

	int *alarmType = (int *)data;

	if (*alarmType == 1)
	{
		port_check();
		router_check();
		sys->set_alarm(this, 1000, alarmType);
	}
	else if (*alarmType == 2)
	{
		for (auto i = 0; i < (int)portTable.size(); i++)
		{
			ping(routerId, sys->time(), i);
		}
		sys->set_alarm(this, 10000, alarmType);
	}
	else
	{
		update_dv_table();
		sys->set_alarm(this, 30000, alarmType);
	}
}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size)
{
	/* JAMES */

	// extract packet to know the type of the packet
	char *pkt = (char *)packet;
	int type = (int)(pkt[0]);

	switch (type)
	{
	case 0:
		recv_regular_pkt(port, pkt);
		break;

	case 1:
		pong(port, pkt);
		break;

	case 2:
		recv_pong_pkt(port, pkt);
		break;

	case 3:
		recv_DV_pkt(port, pkt);
		break;

	case 4:
		recv_LS_pkt(port, pkt);
		break;

	default:
		delete pkt;
		break;
	}
}

void RoutingProtocolImpl::ping(unsigned short routerId, int32_t sending_time, unsigned short port_id)
{
	/* JAMES */

	/* PACKET STRUCTURE */
	// 1. packet type (1 byte)
	// 2. reserved (1 byte)
	// 3. size (2 bytes)
	// 4. source id (2 bytes)
	// 5. dest id (2 bytes)
	// 6. payload

	// 1. create ping packet
	char *packet = new char[pkt_size];
	packet[0] = (char)PING;
	*(unsigned short *)(packet + 2) = htons(pkt_size);
	*(unsigned short *)(packet + 4) = htons(routerId);
	// add sending time to payload
	*(int32_t *)(packet + 8) = htonl(sending_time);

	// 2. send ping packet with the given port, created packet, and size
	sys->send(port_id, packet, pkt_size);
	return;
}

void RoutingProtocolImpl::pong(unsigned short port_id, char *pkt)
{
	/* JAMES */

	/* PACKET STRUCTURE */
	// 1. packet type (1 byte)
	// 2. reserved (1 byte)
	// 3. size (2 bytes)
	// 4. source id (2 bytes)
	// 5. dest id (2 bytes)
	// 6. payload

	// 1. modify packet
	pkt[0] = (char)PONG;
	*(unsigned short *)(pkt + 4) = htons(this->routerId);
	*(unsigned short *)(pkt + 6) = htons(ntohs(*(unsigned short *)(pkt + 4)));

	// 2. send pong packet via the port
	sys->send(port_id, pkt, pkt_size);
	return;
}

void RoutingProtocolImpl::recv_pong_pkt(unsigned short port_id, char *pkt)
{
	/* JAMES */

	/* PACKET STRUCTURE */
	// 1. packet type (1 byte)
	// 2. reserved (1 byte)
	// 3. size (2 bytes)
	// 4. source id (2 bytes)
	// 5. dest id (2 bytes)
	// 6. payload

	// 1. extract the packet we received
	unsigned short neighbor_id = ntohs(*(unsigned short *)(pkt + 4)), pre_cost = portTable[port_id]->costToNeighbor;
	int32_t sending_time = ntohl(*(int32_t *)(pkt + 8)), recving_time = sys->time();
	// 2. update cost
	portTable[port_id]->costToNeighbor = recving_time - sending_time;
	// 3. update neighbor
	portTable[port_id]->toRouterID = neighbor_id;
	// 4. update last receive time
	portTable[port_id]->lastPongTime = recving_time;

	delete pkt;

	// update dv table if needed
	if (protocolType == P_DV)
	{
		// if the update_rt() returns true means that the routing table has been modified
		// we should send the updated routing table to other routers
		if (update_rt(port_id, pre_cost, recving_time - sending_time))
		{
			update_dv_table();
		}
	}
}

void RoutingProtocolImpl::recv_regular_pkt(unsigned short port_id, char *pkt)
{
	/* JAMES */

	/* PACKET STRUCTURE */
	// 1. packet type (1 byte)
	// 2. reserved (1 byte)
	// 3. size (2 bytes)
	// 4. source id (2 bytes)
	// 5. dest id (2 bytes)
	// 6. payload

	// 1. extract the packet we received
	unsigned short size = ntohs(*(unsigned short *)(pkt + 2));
	unsigned short dest_id = ntohs(*(unsigned short *)(pkt + 6));

	// 2. check if this router is the destination
	if (this->routerId == dest_id)
	{
		// if it is, receive the packet and free its memory
		delete pkt;
	}
	else
	{
		// if not, decide which is the next-hop according to the routing table
		if (routingTable.find(dest_id) != routingTable.end())
		{
			// look up the routing table
			unsigned short nexthop = routingTable[dest_id]->nextPort;
			if (routingTable[dest_id]->cost != INFINITY_COST)
			{
				// have possible path to the destination
				sys->send(nexthop, pkt, size);
			}
			else
			{
				// does not have possible path to the destination, simply free the memory
				delete pkt;
			}
		}
		else
		{
			// no existing routing table, free the memory
			delete pkt;
		}
	}
	return;
}

void RoutingProtocolImpl::recv_DV_pkt(unsigned short port_id, char *pkt)
{
	/* JAMES */

	/* PACKET STRUCTURE */
	// 1. packet type (1 byte)
	// 2. reserved (1 byte)
	// 3. size (2 bytes)
	// 4. source id (2 bytes)
	// 5. dest id (2 bytes)
	// 6. several [node (2 bytes), cost (2 bytes)] pairs

	// 1. check the cost of the current port
	if (portTable[port_id]->costToNeighbor == INFINITY_COST)
	{
		// no possible route
		return;
	}

	// 2. extract packet we received
	unsigned short size = ntohs(*(unsigned short *)(pkt + 2));

	// map for storing node cost pair
	unordered_map<int, int> node_cost_pair;

	// 3. add [id, cost] pair to dv table payload
	for (int i = 0; i < ((size - 8) / 4); i++)
	{
		unsigned short cur_id = ntohs(*(unsigned short *)(pkt + 8 + 4 * i));
		unsigned short cur_cost = ntohs(*(unsigned short *)(pkt + 8 + 4 * i + 2));

		if (cur_id != routerId)
		{
			node_cost_pair[cur_id] = cur_cost;
		}
	}

	// 4. update routing table
	// if the update_rt() returns true means that the routing table has been modified
	// we should send the updated routing table to other routers
	if (update_rt(port_id, node_cost_pair))
	{
		update_dv_table();
	}
}

void RoutingProtocolImpl::recv_LS_pkt(unsigned short port_id, char *pkt)
{
	/* JAMES */
	// implemented in ping and pong functions
}

void RoutingProtocolImpl::update_dv_table()
{
	/* JAMES */
	if (routingTable.size() == 0)
	{
		return;
	}

	vector<unsigned short> routers;
	for (auto iter = routingTable.begin(); iter != routingTable.end(); iter++)
	{
		// [rid, RoutingTableEntry]
		if (iter->second->cost == INFINITY_COST)
		{
			continue;
		}
		else
		{
			routers.push_back(iter->first);
		}
	}

	unsigned short size = 8 + 4 * (unsigned short)routers.size();

	int port_id = 0;
	for (auto iter = portTable.begin(); iter != portTable.end(); iter++)
	{
		// find available nexthop
		if ((*iter)->costToNeighbor != INFINITY_COST && (*iter)->toRouterID != 0xffff)
		{
			// create dv packet
			char *new_pkt = new char[size];
			new_pkt[0] = (char)DV;
			*(unsigned short *)(new_pkt + 2) = htons(size);
			*(unsigned short *)(new_pkt + 4) = htons(this->routerId);
			*(unsigned short *)(new_pkt + 6) = htons((*iter)->toRouterID);

			int start = 8;
			for (unsigned short i = 0; i < routers.size(); i++)
			{
				*(unsigned short *)(new_pkt + start) = htons(routers[i]);
				routingTable[routers[i]]->nextPort == port_id ? *(unsigned short *)(new_pkt + start + 2) = htons(INFINITY_COST) : htons(routingTable[routers[i]]->cost);
				start += 4;
			}

			sys->send(port_id, new_pkt, size);
		}
		port_id++;
	}
}

void RoutingProtocolImpl::router_check()
{
	/* CHELSEA */

	int32_t curTime = sys->time();

	for (auto it = routingTable.begin(); it != routingTable.end(); it++)
	{
		unsigned short destId = it->first;
		RoutingTableEntry *entry = it->second;

		// DV, LS entry that is not refreshed within 45 seconds are timed out
		if (curTime - entry->lastUpdateTime > 45000 && protocolType == P_DV)
		{
			entry->cost = INFINITY_COST;
			entry->lastUpdateTime = curTime;

			for (auto i = 0; i < (int)portTable.size(); i++)
			{
				if (portTable[i]->toRouterID == destId && portTable[i]->costToNeighbor < entry->cost)
				{
					entry->cost = portTable[i]->costToNeighbor;
					entry->nextPort = i;
					entry->lastUpdateTime = curTime;
				}
			}
		}
	}
}

void RoutingProtocolImpl::port_check()
{
	/* CHELSEA */

	auto curTime = sys->time();

	for (auto pid = 0; pid < int(portTable.size()); pid++) 
	{
		auto port = portTable[pid];
		// Dead port: status not refreshed for 15 secs
		if (curTime - port->lastPongTime > 15000 && port->costToNeighbor != INFINITY_COST)
		{
			int32_t prevCostToNeighbor = port->costToNeighbor;
			port->costToNeighbor = INFINITY_COST;

			if (protocolType == P_DV and update_rt(pid, prevCostToNeighbor, INFINITY_COST))
				update_dv_table();

		}
	}
}

// update the routing table when the router receives a pong from a neighbor router and for alarm periodic check
bool RoutingProtocolImpl::update_rt(int pid, int preCost, int newCost)
{
	bool isTableModified = false;

	// nothing changed
	if (preCost == newCost)
	{
		return isTableModified;
	}

	// start to update routing table
	for (auto it = routingTable.begin(); it != routingTable.end(); it++)
	{

		RoutingTableEntry *entry = it->second;

		if (entry->cost != INFINITY_COST && entry->nextPort == pid)
		{
			// router is dead over 30 secs
			if (newCost == INFINITY_COST)
			{
				entry->cost = newCost;
			}
			else
			{
				entry->cost += (newCost - preCost);
			}

			entry->lastUpdateTime = sys->time();
			isTableModified = true;

			// But, if the updated cost increases to get to dest through pid
			// maybe we can achieve better cost through a different pid
			for (int i = 0; i < (int)portTable.size(); i++)
			{
				if (portTable[i]->toRouterID == it->first)
				{
					if (portTable[i]->costToNeighbor < entry->cost)
					{
						entry->cost = portTable[i]->costToNeighbor;
						entry->nextPort = i;
					}
				}
			}
		}
	}

	if (newCost != INFINITY_COST)
	{
		PortStatus *port = portTable[pid];
		int neighborRouterID = port->toRouterID;
		// if this route already exists in the routing table, update the cost
		// if this route does not exist in the routing table, add it
		if (routingTable.count(neighborRouterID))
		{
			RoutingTableEntry *info = routingTable[neighborRouterID];
			if (info->cost > newCost)
			{
				info->cost = newCost;
				info->nextPort = pid;
				info->lastUpdateTime = sys->time();
				isTableModified = true;
			}
		}
		else if (newCost != INFINITY_COST)
		{
			auto t = sys->time();
			RoutingTableEntry *newRTE = new RoutingTableEntry(pid, newCost, t);
			routingTable[neighborRouterID] = newRTE;
			isTableModified = true;
		}
	}

	return isTableModified;
}

// update the routing table when the router receives a Distance Vector payload from a neighbor router.
bool RoutingProtocolImpl::update_rt(int pid, unordered_map<int, int> &updatedDV)
{
	// return value: whether the routing table has been modified or not
	bool modified = false;
	// get the neighbor router ID
	auto neighborID = portTable[pid]->toRouterID;
	// iterate through the routing table
	for (auto &entry : routingTable)
	{
		auto desID = entry.first;
		RoutingTableEntry *rtEntry = entry.second;
		// skip the entry for the neighbor router if it is the destination
		if (desID == neighborID)
		{
			continue;
		}
		// if the destination is in the updated DV, update the routing table if the cost is smaller
		auto it = updatedDV.find(desID);
		if (it != updatedDV.end())
		{
			if (rtEntry->nextPort == pid)
			{
				auto preCost = rtEntry->cost;
				if (it->second == INFINITY_COST)
				{
					rtEntry->cost = INFINITY_COST;
				}
				else
				{
					rtEntry->cost = portTable[pid]->costToNeighbor + it->second;
				}
				if (rtEntry->cost != preCost)
				{
					modified = true;
				}

				rtEntry->lastUpdateTime = sys->time();
				// if the cost is larger than before, check if there is a smaller cost to the destination
				if (rtEntry->cost > preCost)
				{
					for (int i = 0; i < (int)portTable.size(); i++)
					{
						if (portTable[i]->toRouterID == desID)
						{
							if (portTable[i]->costToNeighbor < rtEntry->cost)
							{
								rtEntry->cost = portTable[i]->costToNeighbor;
								rtEntry->nextPort = i;
								modified = true;
							}
						}
					}
				}
			}
			else if (portTable[pid]->costToNeighbor + it->second < rtEntry->cost)
			{
				rtEntry->cost = portTable[pid]->costToNeighbor + it->second;
				rtEntry->lastUpdateTime = sys->time();
				rtEntry->nextPort = pid;
				modified = true;
			}
		}
		else if (rtEntry->nextPort == pid && rtEntry->cost != INFINITY_COST)
		{
			rtEntry->cost = INFINITY_COST;
			rtEntry->lastUpdateTime = sys->time();
			modified = true;

			for (int i = 0; i < (int)portTable.size(); i++)
			{
				if (portTable[i]->toRouterID == desID && portTable[i]->costToNeighbor < rtEntry->cost)
				{
					// rtEntry->cost = portTable[i]->costToNeighbor + rtEntry->cost;
					rtEntry->cost = portTable[i]->toRouterID;
					rtEntry->nextPort = i;
					modified = true;
				}
			}
		}
	}

	// finally, search for new destinations and add them to the routing table
	for (auto it = updatedDV.begin(); it != updatedDV.end(); it++)
	{
		auto desID = it->first;
		if (routingTable.find(desID) == routingTable.end())
		{
			auto cost = portTable[pid]->costToNeighbor + it->second;
			auto curTime = sys->time();
			routingTable[desID] = new RoutingTableEntry(pid, cost, curTime);
			modified = true;
		}
	}

	return modified;
}
