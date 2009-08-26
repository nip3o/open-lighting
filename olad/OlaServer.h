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
 * OlaServer.h
 * Interface for the ola server class
 * Copyright (C) 2005-2008 Simon Newton
 */

#ifndef OLA_SERVER_H
#define OLA_SERVER_H

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <map>
#include <string>

#include <ola/ExportMap.h>
#include <ola/plugin_id.h>
#include <ola/network/SelectServer.h>
#include <ola/network/Socket.h>

namespace ola {

#ifdef HAVE_LIBMICROHTTPD
typedef class OlaHttpServer OlaHttpServer_t;
#else
typedef int OlaHttpServer_t;
#endif

typedef struct {
  bool http_enable; // run the http server
  bool http_localhost_only; // restrict access to localhost only
  bool http_enable_quit; // enable /quit
  unsigned int http_port; // port to run the http server on
  std::string http_data_dir; // directory that contains the static content
} ola_server_options;


/*
 * The main OlaServer class
 */
class OlaServer {
  public:
    OlaServer(class OlaServerServiceImplFactory *factory,
              class PluginLoader *plugin_loader,
              class PreferencesFactory *preferences_factory,
              ola::network::SelectServer *network,
              ola_server_options *ola_options,
              ola::network::AcceptingSocket *socket=NULL,
              ExportMap *export_map=NULL);
    ~OlaServer();
    bool Init();
    void ReloadPlugins();
    int AcceptNewConnection(ola::network::AcceptingSocket *socket);
    bool NewConnection(ola::network::ConnectedSocket *socket);
    int SocketClosed(ola::network::ConnectedSocket *socket);
    int GarbageCollect();

    static const unsigned int DEFAULT_HTTP_PORT = 9090;

  private :
    OlaServer(const OlaServer&);
    OlaServer& operator=(const OlaServer&);
    void StopPlugins();
    void StartPlugins();
    void CleanupConnection(class OlaServerServiceImpl *service);

    class OlaServerServiceImplFactory *m_service_factory;
    class PluginLoader *m_plugin_loader;
    ola::network::SelectServer *m_ss;
    ola::network::AcceptingSocket *m_accepting_socket;

    class DeviceManager *m_device_manager;
    class PluginAdaptor *m_plugin_adaptor;
    class PreferencesFactory *m_preferences_factory;
    class Preferences *m_universe_preferences;
    class UniverseStore *m_universe_store;
    class ExportMap *m_export_map;

    bool m_init_run;
    bool m_free_export_map;
    std::map<int, class OlaServerServiceImpl*> m_sd_to_service;
    OlaHttpServer_t *m_httpd;
    ola_server_options m_options;

    static const string UNIVERSE_PREFERENCES;
    static const string K_CLIENT_VAR;
    static const unsigned int K_GARBAGE_COLLECTOR_TIMEOUT_MS;
};


} // ola
#endif