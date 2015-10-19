/*
 * BaseMaster.cpp
 *
 *  Created on: Aug 7, 2014
 *      Author: thardin
 */

#include "master/BaseMaster.h"
#ifndef WIN32
#include <unistd.h>
#endif

using namespace fmitcp_master;
using namespace fmitcp;

#ifdef USE_LACEWING
BaseMaster::BaseMaster(EventPump *pump, vector<FMIClient*> slaves) :
        m_pendingRequests(0),
        m_pump(pump),
#else
BaseMaster::BaseMaster(vector<FMIClient*> slaves) :
        m_pendingRequests(0),
#endif
        m_slaves(slaves) {
}

BaseMaster::~BaseMaster() {
}

void BaseMaster::wait() {
    while (m_pendingRequests > 0) {
#ifdef USE_LACEWING
        m_pump->tick();
#ifdef WIN32
        Yield();
#else
        usleep(10);
#endif
#else
    //poll all clients, decrease m_pendingRequests as we see REPlies coming in
    vector<zmq::pollitem_t> items(m_slaves.size());
    for (size_t x = 0; x < m_slaves.size(); x++) {
        items[x].socket = m_slaves[x]->m_socket;
        items[x].events = ZMQ_POLLIN;
    }
    fprintf(stderr, "polling %li sockets, %li pending\n", m_slaves.size(), m_pendingRequests);
    int n = zmq::poll(items.data(), m_slaves.size());
    fprintf(stderr, "zmq::poll(): %i new events vs %li pending\n", n, m_pendingRequests);
    for (size_t x = 0; x < m_slaves.size(); x++) {
        if (items[x].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            m_slaves[x]->m_socket.recv(&msg);
            fprintf(stderr, "Got message of size %li\n", msg.size());
            m_pendingRequests--;
            m_slaves[x]->Client::clientData(static_cast<char*>(msg.data()), msg.size());
        }
    }
#endif
    }
}
