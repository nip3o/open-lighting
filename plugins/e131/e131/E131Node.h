/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * E131Node.h
 * Header file for the E131Node class, this is the interface between OLA and
 * the E1.31 library.
 * Copyright (C) 2007-2009 Simon Newton
 */

#ifndef OLA_E131_NODE
#define OLA_E131_NODE

#include <string>
#include <ola/Closure.h>
#include <ola/DmxBuffer.h>

#include "CID.h"
#include "E131Layer.h"
#include "RootLayer.h"
#include "UDPTransport.h"
#include "DMPE131Inflator.h"

namespace ola {
namespace e131 {

class E131Node {
  public:
    E131Node(const std::string &ip_address,
             const CID &cid=CID::Generate());
    ~E131Node();

    bool Start();
    bool Stop();

    bool SetSourceName(unsigned int universe, const string &source);
    bool SetSourcePriority(unsigned int universe, uint16_t priority);
    bool SendDMX(unsigned int universe, const ola::DmxBuffer &buffer);
    bool SetHandler(unsigned int universe, ola::DmxBuffer *buffer,
                    ola::Closure *handler);
    bool RemoveHandler(unsigned int universe);

    ola::network::UdpSocket* GetSocket() { return m_transport.GetSocket(); }

  private:
    typedef struct {
      string source;
      uint16_t priority;
      uint16_t sequence;
    } tx_universe;

    string m_preferred_ip;
    CID m_cid;
    UDPTransport m_transport;
    RootLayer m_root_layer;
    E131Layer m_e131_layer;
    DMPE131Inflator m_dmp_inflator;
    map<unsigned int, tx_universe> m_tx_universes;

    tx_universe &SetupOutgoingSettings(unsigned int universe);

    E131Node(const E131Node&);
    E131Node& operator=(const E131Node&);

    static const uint16_t DEFAULT_PRIORITY = 100;
};

} //e131
} //ola

#endif