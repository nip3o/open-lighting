/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * OlaClientCore.cpp
 * Implementation of OlaClientCore
 * Copyright (C) 2005-2008 Simon Newton
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include <ola/BaseTypes.h>

#include "OlaClientCore.h"
#include "OlaDevice.h"

#ifdef HAVE_PTHREAD
#define acquire_lock pthread_mutex_lock(&m_mutex)
#define release_lock pthread_mutex_unlock(&m_mutex)
#else
#define acquire_lock
#define release_lock
#endif

namespace ola {

using google::protobuf::NewCallback;

OlaClientCore::OlaClientCore(ConnectedSocket *socket):
  m_socket(socket),
  m_client_service(NULL),
  m_channel(NULL),
  m_stub(NULL),
  m_connected(false),
  m_observer(NULL) {
#ifdef HAVE_PTHREAD
  pthread_mutex_init(&m_mutex, NULL);
#endif
}


OlaClientCore::~OlaClientCore() {
  if (m_connected)
    Stop();
}


/*
 * Setup this client
 * @return true on success, false on failure
 */
bool OlaClientCore::Setup() {

  if (m_connected)
    return false;

  m_client_service = new OlaClientServiceImpl(m_observer);

  if (!m_client_service)
    return false;

  m_channel = new StreamRpcChannel(m_client_service, m_socket);

  if (!m_channel) {
    delete m_client_service;
    return false;
  }
  m_stub = new OlaServerService_Stub(m_channel);

  if (!m_stub) {
    delete m_channel;
    delete m_client_service;
    return false;
  }
  m_connected = true;
  return true;
}


/*
 * Close the ola connection.
 * @return true on success, false on failure
 */
bool OlaClientCore::Stop() {
  acquire_lock;
  if (m_connected) {

    m_socket->Close();
    delete m_channel;
    delete m_client_service;
    delete m_stub;
  }
  m_connected = false;
  release_lock;
  return 0;
}


/*
 * Set the observer object
 * @return true
 */
bool OlaClientCore::SetObserver(OlaClientObserver *observer) {
  if (m_client_service)
    m_client_service->SetObserver(observer);
  m_observer = observer;
  return true;
}


/*
 * Fetch information about available plugins
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchPluginInfo(ola_plugin_id filter,
                                    bool include_description) {
  if (!m_connected)
    return false;

  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::PluginInfoRequest request;
  ola::proto::PluginInfoReply *reply = new ola::proto::PluginInfoReply();

  request.set_plugin_id(filter);
  request.set_include_description(include_description);

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandlePluginInfo,
      controller,
      reply);
  m_stub->GetPluginInfo(controller, &request, reply, cb);
  return true;
}


/*
 * Write some dmx data
 * @param universe   universe to send to
 * @param data the DmxBuffer with the data
 * @return true on success, false on failure
 */
bool OlaClientCore::SendDmx(unsigned int universe,
                            const DmxBuffer &data) {
  if (!m_connected)
    return false;

  ola::proto::DmxData request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_universe(universe);
  request.set_data(data.Get());

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandleSendDmx,
      controller,
      reply);
  m_stub->UpdateDmxData(controller, &request, reply, cb);
  return true;
}


/*
 * Read dmx data
 * @param universe the universe id to get data for
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchDmx(unsigned int universe) {
  if (!m_connected)
    return false;

  ola::proto::DmxReadRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::DmxData *reply = new ola::proto::DmxData();

  request.set_universe(universe);

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandleGetDmx,
      controller,
      reply);
  m_stub->GetDmx(controller, &request, reply, cb);
  return true;
}


/*
 * Request a listing of what devices are attached
 * @param filter only fetch devices that belong to this particular plugin
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchDeviceInfo(ola_plugin_id filter) {
  if (!m_connected)
    return false;

  ola::proto::DeviceInfoRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::DeviceInfoReply *reply = new ola::proto::DeviceInfoReply();
  request.set_plugin_id(filter);

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandleDeviceInfo,
      controller,
      reply);
  m_stub->GetDeviceInfo(controller, &request, reply, cb);
  return true;
}


/*
 * Request a universe listing
 * @return true on success, false on failure
 */
bool OlaClientCore::FetchUniverseInfo() {
  if (!m_connected)
    return false;

  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::UniverseInfoRequest request;
  ola::proto::UniverseInfoReply *reply = new ola::proto::UniverseInfoReply();

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandleUniverseInfo,
      controller,
      reply);
  m_stub->GetUniverseInfo(controller, &request, reply, cb);
  return true;
}


/*
 * Set the name of a universe
 * @param universe the id of the universe
 * @param name the new name
 * @return true on success, false on failure
 */
bool OlaClientCore::SetUniverseName(unsigned int universe,
                                    const string &name) {
  if (!m_connected)
    return false;

  ola::proto::UniverseNameRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  request.set_universe(universe);
  request.set_name(name);

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandleUniverseName,
      controller,
      reply);
  m_stub->SetUniverseName(controller, &request, reply, cb);
  return true;
}


/*
 * Set the merge mode of a universe
 * @param universe the id of the universe
 * @param mode the new merge mode
 * @return true on success, false on failure
 */
bool OlaClientCore::SetUniverseMergeMode(unsigned int universe,
                                         OlaUniverse::merge_mode mode) {
  if (!m_connected)
    return false;

  ola::proto::MergeModeRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  ola::proto::MergeMode merge_mode = mode == OlaUniverse::MERGE_HTP ?
    HTP : LTP;
  request.set_universe(universe);
  request.set_merge_mode(merge_mode);

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandleUniverseMergeMode,
      controller,
      reply);
  m_stub->SetMergeMode(controller, &request, reply, cb);
  return true;
}


/*
 * Register our interest in a universe, the observer object will then
 * be notifed when the dmx values in this universe change.
 * @param universe the id of the universe
 * @param action the action (register or unregister)
 */
bool OlaClientCore::RegisterUniverse(unsigned int universe,
                                     ola::RegisterAction register_action) {
  if (!m_connected)
    return false;

  ola::proto::RegisterDmxRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  ola::proto::RegisterAction action = (
      register_action == ola::REGISTER ? ola::proto::REGISTER :
        ola::proto::UNREGISTER);
  request.set_universe(universe);
  request.set_action(action);

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandleRegister,
      controller,
      reply);
  m_stub->RegisterForDmx(controller, &request, reply, cb);
  return true;
}


/*
 * (Un)Patch a port to a universe
 * @param device_alias the alias of the device
 * @param port  the port id
 * @param action OlaClientCore::PATCH or OlaClientCore::UNPATCH
 * @param universe universe id
 */
bool OlaClientCore::Patch(unsigned int device_alias,
                          unsigned int port_id,
                          ola::PatchAction patch_action,
                          unsigned int universe) {
  if (!m_connected)
    return false;

  ola::proto::PatchPortRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::Ack *reply = new ola::proto::Ack();

  ola::proto::PatchAction action = (
      patch_action == ola::PATCH ? ola::proto::PATCH : ola::proto::UNPATCH);
  request.set_universe(universe);
  request.set_device_alias(device_alias);
  request.set_port_id(port_id);
  request.set_action(action);

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandlePatch,
      controller,
      reply);
  m_stub->PatchPort(controller, &request, reply, cb);
  return true;
}


/*
 * Sends a device config request
 * @param device_alias the device alias
 * @param msg the data to send
 */
bool OlaClientCore::ConfigureDevice(unsigned int device_alias,
                                    const string &msg) {
  if (!m_connected)
    return false;

  ola::proto::DeviceConfigRequest request;
  SimpleRpcController *controller = new SimpleRpcController();
  ola::proto::DeviceConfigReply *reply = new ola::proto::DeviceConfigReply();

  string configure_request;
  request.set_device_alias(device_alias);
  request.set_data(msg);

  google::protobuf::Closure *cb = NewCallback(
      this,
      &ola::OlaClientCore::HandleDeviceConfig,
      controller,
      reply);
  m_stub->ConfigureDevice(controller, &request, reply, cb);
  return true;
}


// The following are RPC callbacks

/*
 * Called once PluginInfo completes
 */
void OlaClientCore::HandlePluginInfo(SimpleRpcController *controller,
                                     ola::proto::PluginInfoReply *reply) {
  string error_string = "";
  vector<OlaPlugin> ola_plugins;

  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();
    else {
      for (int i=0; i < reply->plugin_size(); ++i) {
        PluginInfo plugin_info = reply->plugin(i);
        OlaPlugin plugin(plugin_info.plugin_id(),
                         plugin_info.name(),
                         plugin_info.description());
        ola_plugins.push_back(plugin);
      }
    }
    std::sort(ola_plugins.begin(), ola_plugins.end());
    release_lock;
    m_observer->Plugins(ola_plugins, error_string);
    acquire_lock;
  }
  delete controller;
  delete reply;
}


/*
 * Called once UpdateDmxData completes
 */
void OlaClientCore::HandleSendDmx(SimpleRpcController *controller,
                                  ola::proto::Ack *reply) {

  string error_string = "";
  if (controller->Failed())
    error_string = controller->ErrorText();

  if (m_observer) {
    release_lock;
    m_observer->SendDmxComplete(error_string);
    acquire_lock;
  }

  delete controller;
  delete reply;
}


/*
 * Called once GetDmx completes
 */
void OlaClientCore::HandleGetDmx(ola::rpc::SimpleRpcController *controller,
                                 ola::proto::DmxData *reply) {
  string error_string;
  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();
    release_lock;
    // TODO: keep a map of registrations and filter
    DmxBuffer buffer;
    buffer.Set(reply->data());
    m_observer->NewDmx(reply->universe(), buffer, error_string);
    acquire_lock;
  }
  delete controller;
  delete reply;
}


/*
 * Called once DeviceInfo completes.
 */
void OlaClientCore::HandleDeviceInfo(ola::rpc::SimpleRpcController *controller,
                                     ola::proto::DeviceInfoReply *reply) {
  string error_string = "";
  vector<OlaDevice> ola_devices;

  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();
    else {
      for (int i=0; i < reply->device_size(); ++i) {
        DeviceInfo device_info = reply->device(i);
        vector<OlaPort> ports;

        for (int j=0; j < device_info.port_size(); ++j) {
          PortInfo port_info = device_info.port(j);
          OlaPort::PortCapability capability = port_info.output_port() ?
            OlaPort::OLA_PORT_CAP_OUT : OlaPort::OLA_PORT_CAP_IN;
          OlaPort port(port_info.port_id(),
                       capability,
                       port_info.universe(),
                       port_info.active(),
                       port_info.description());
          ports.push_back(port);
        }

        OlaDevice device(device_info.device_id(),
                         device_info.device_alias(),
                         device_info.device_name(),
                         device_info.plugin_id(),
                         ports);
        ola_devices.push_back(device);
      }
    }
    std::sort(ola_devices.begin(), ola_devices.end());
    release_lock;
    m_observer->Devices(ola_devices, error_string);
    acquire_lock;
  }
  delete controller;
  delete reply;
}



/*
 * Called once PluginInfo completes
 */
void OlaClientCore::HandleUniverseInfo(SimpleRpcController *controller,
                                       ola::proto::UniverseInfoReply *reply) {
  string error_string = "";
  vector<OlaUniverse> ola_universes;

  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();
    else {
      for (int i=0; i < reply->universe_size(); ++i) {
        UniverseInfo universe_info = reply->universe(i);
        OlaUniverse::merge_mode merge_mode = (
          universe_info.merge_mode() == ola::proto::HTP ?
          OlaUniverse::MERGE_HTP: OlaUniverse::MERGE_LTP);

        OlaUniverse universe(universe_info.universe(),
                             merge_mode,
                             universe_info.name());
        ola_universes.push_back(universe);
      }
    }
    release_lock;
    m_observer->Universes(ola_universes, error_string);
    acquire_lock;
  }

  delete controller;
  delete reply;
}


/*
 * Called once SetUniverseName completes
 */
void OlaClientCore::HandleUniverseName(SimpleRpcController *controller,
                                       ola::proto::Ack *reply) {
  string error_string = "";
  if (controller->Failed())
    error_string = controller->ErrorText();

  if (m_observer) {
    release_lock;
    m_observer->UniverseNameComplete(error_string);
    acquire_lock;
  } else if (!error_string.empty())
    printf("set name failed: %s\n", error_string.c_str());

  delete controller;
  delete reply;
}


/*
 * Called once SetMergeMode completes
 */
void OlaClientCore::HandleUniverseMergeMode(SimpleRpcController *controller,
                                            ola::proto::Ack *reply) {
  string error_string = "";
  if (controller->Failed())
    error_string = controller->ErrorText();

  if (m_observer) {
    release_lock;
    m_observer->UniverseMergeModeComplete(error_string);
    acquire_lock;
  } else if (!error_string.empty())
    printf("set merge mode failed: %s\n", controller->ErrorText().c_str());
  delete controller;
  delete reply;
}


/*
 * Called once SetMergeMode completes
 */
void OlaClientCore::HandleRegister(SimpleRpcController *controller,
                                   ola::proto::Ack *reply) {
  if (controller->Failed())
    printf("register failed: %s\n", controller->ErrorText().c_str());
  delete controller;
  delete reply;
}


/*
 * Called once SetMergeMode completes
 */
void OlaClientCore::HandlePatch(SimpleRpcController *controller,
                                ola::proto::Ack *reply) {
  string error_string = "";
  if (controller->Failed())
    error_string = controller->ErrorText();

  if (m_observer) {
    release_lock;
    m_observer->PatchComplete(error_string);
    acquire_lock;
  } else if (!error_string.empty())
    printf("patch failed: %s\n", controller->ErrorText().c_str());
  delete controller;
  delete reply;
}


/*
 * Handle a device config response
 *
 */
void OlaClientCore::HandleDeviceConfig(ola::rpc::SimpleRpcController *controller,
                                       ola::proto::DeviceConfigReply *reply) {
  string error_string;
  if (m_observer) {
    if (controller->Failed())
      error_string = controller->ErrorText();

    release_lock;
    //TODO: add the device id here
    m_observer->DeviceConfig(reply->data(), error_string)
    acquire_lock;
  }
  delete controller;
  delete reply;
}


} // ola