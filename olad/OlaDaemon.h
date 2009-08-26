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
 * OlaDaemon.h
 * Interface for the OLA Daemon class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLA_DAEMON_H
#define OLA_DAEMON_H

#include <ola/network/SelectServer.h>
#include <ola/network/Socket.h>
#include <ola/BaseTypes.h>
#include <ola/ExportMap.h>
#include "OlaServer.h"

namespace ola {

using ola::network::AcceptingSocket;
using ola::network::SelectServer;

class OlaDaemon {
  public:
    OlaDaemon(ola_server_options &options,
              ExportMap *export_map=NULL,
              unsigned int rpc_port=OLA_DEFAULT_PORT);
    ~OlaDaemon();
    bool Init();
    void Run();
    void Terminate();
    void ReloadPlugins();
    class SelectServer* GetSelectServer() const { return m_ss; }
    class OlaServer *GetOlaServer() const { return m_server; }

    static const unsigned int DEFAULT_RPC_PORT = OLA_DEFAULT_PORT;

  private:
    OlaDaemon(const OlaDaemon&);
    OlaDaemon& operator=(const OlaDaemon&);

    class PluginLoader *m_plugin_loader;
    class SelectServer *m_ss;
    class OlaServer *m_server;
    class PreferencesFactory *m_preferences_factory;
    class AcceptingSocket *m_accepting_socket;
    class OlaServerServiceImplFactory *m_service_factory;
    ola_server_options m_options;
    class ExportMap *m_export_map;
    unsigned int m_rpc_port;

    static const string K_RPC_PORT_VAR;
};

} // ola
#endif