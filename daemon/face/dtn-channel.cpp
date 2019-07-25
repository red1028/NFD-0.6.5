/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dtn-channel.hpp"
#include "dtn-transport.hpp"
#include <string>
#include <sys/stat.h> // for chmod()
#include "generic-link-service.hpp"
//#include "core/scheduler.hpp"
#include "common/global.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT(DtnChannel);

DtnChannel::DtnChannel(const std::string &endpointPrefix,
                       const std::string &endpointAffix,
					   const std::string &ibrdtnHost,
					   uint16_t ibrdtndPort)
  : m_endpointPrefix(endpointPrefix),
    m_endpointAffix(endpointAffix),
	m_ibrdtnHost(ibrdtnHost),
	m_ibrdtndPort(ibrdtndPort)
{
  // const std::string& scheme("CBHE");
  m_is_open = false;
  m_pIbrDtnClient = nullptr;
  setUri(FaceUri(m_endpointPrefix, m_endpointAffix));
  NFD_LOG_CHAN_INFO("Creating dtn channel");
}

DtnChannel::~DtnChannel()
{
  if (isListening()) {
    NFD_LOG_CHAN_DEBUG(" Removing dtn socket");
    if (m_pIbrDtnClient != nullptr)
    	delete m_pIbrDtnClient;

    m_is_open = false;
  }
}

void
DtnChannel::listen(const FaceCreatedCallback& onFaceCreated,
                   const FaceCreationFailedCallback& onReceiveFailed)
                   //int backlog/* = acceptor::max_connections*/)
{
  if (isListening()) {
    NFD_LOG_CHAN_WARN("[" << m_endpointAffix << "] Already listening");
    return;
  }
  m_is_open = true;

  m_onFaceCreated = onFaceCreated;
  m_onReceiveFailed = onReceiveFailed;

  std::string app = m_endpointAffix.substr(1); // Remove leading '/'

  m_pIbrDtnClient = new nfd::face::AsyncIbrDtnClient(app, m_ibrdtnHost, m_ibrdtndPort, this, getGlobalIoService());
}

void
DtnChannel::processBundle(dtn::data::Bundle b)
{
  // Do bundle processing
  std::string remoteEndpoint = b.source.getString();

  NFD_LOG_CHAN_INFO("DTN bundle received from " << remoteEndpoint);
  NFD_LOG_CHAN_DEBUG("[" << m_endpointAffix << "] New peer " << remoteEndpoint);

  bool isCreated = false;
  shared_ptr<Face> face = nullptr;

  std::tie(isCreated, face) = createFace(remoteEndpoint, ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);

  if (face == nullptr)
  {
    NFD_LOG_CHAN_WARN("[" << m_endpointAffix << "] Failed to create face for peer " << remoteEndpoint);
    if (m_onReceiveFailed)
	  m_onReceiveFailed(500, "Accept failed: " + remoteEndpoint);
    return;
  }

  if (isCreated)
    m_onFaceCreated(face);

  // dispatch the bundle to the face for processing
  static_cast<face::DtnTransport*>(face->getTransport())->receiveBundle(b);
}

void
DtnChannel::connect(const std::string &remoteEndpoint,
                    const FaceParams& params,
                    const FaceCreatedCallback& onFaceCreated,
                    const FaceCreationFailedCallback& onConnectFailed)
{
  shared_ptr<Face> face;
  face = createFace(remoteEndpoint, params.persistency).second;
  if (face == nullptr)
  {
    NFD_LOG_CHAN_WARN("[" << m_endpointAffix << "] Connect failed");
    if (onConnectFailed)
      onConnectFailed(504, "Connection failed: " + remoteEndpoint);
    return;
  }

  // Need to invoke the callback regardless of whether or not we had already
  // created the face so that control responses and such can be sent
  onFaceCreated(face);
}

std::pair<bool, shared_ptr<Face>>
DtnChannel::createFace(const std::string& remoteEndpoint, ndn::nfd::FacePersistency persistency)
{
  auto it = m_channelFaces.find(remoteEndpoint);
  if (it != m_channelFaces.end()) {
    // we already have a face for this endpoint, just reuse it
	auto face = it->second;
    // only on-demand -> persistent -> permanent transition is allowed
    /*
    bool isTransitionAllowed = persistency != face->getPersistency() &&
                               (face->getPersistency() == ndn::nfd::FACE_PERSISTENCY_ON_DEMAND ||
                                persistency == ndn::nfd::FACE_PERSISTENCY_PERMANENT);
    if (isTransitionAllowed) {
      face->setPersistency(persistency);
    }
    */
	face->setPersistency(persistency);
    return {false, face};
  }

  // else, create a new face

  auto linkService = make_unique<face::GenericLinkService>();
  std::string localEndpoint = m_endpointPrefix + m_endpointAffix;
  auto transport = make_unique<face::DtnTransport>(localEndpoint, remoteEndpoint, m_ibrdtnHost, m_ibrdtndPort );
  auto face = make_shared<Face>(std::move(linkService), std::move(transport));

  face->setPersistency(persistency);

  m_channelFaces[remoteEndpoint] = face;
  connectFaceClosedSignal(*face,
    [this, remoteEndpoint] {
      NFD_LOG_CHAN_TRACE("Erasing " << remoteEndpoint << " from channel face map");
      m_channelFaces.erase(remoteEndpoint);
    });

  return {true, face};
}

void
DtnChannel::accept(const FaceCreatedCallback& onFaceCreated,
                   const FaceCreationFailedCallback& onAcceptFailed)
{
  /*m_acceptor.async_accept(m_socket, bind(&DtnChannel::handleAccept, this,
                                         boost::asio::placeholders::error,
                                         onFaceCreated, onAcceptFailed));*/
}

void
DtnChannel::handleAccept(const boost::system::error_code& error,
                         const FaceCreatedCallback& onFaceCreated,
                         const FaceCreationFailedCallback& onAcceptFailed)
{
  if (error) {
    if (error == boost::asio::error::operation_aborted) // when the socket is closed by someone
      return;

    NFD_LOG_CHAN_DEBUG("[] Accept failed: " << error.message());
    if (onAcceptFailed)
      onAcceptFailed(500, "Accept failed: " + error.message());
    return;
  }

  NFD_LOG_CHAN_DEBUG("[] Incoming connection");

  /*auto linkService = make_unique<face::GenericLinkService>();
  auto transport = make_unique<face::DtnTransport>(std::move(m_socket));
  auto face = make_shared<Face>(std::move(linkService), std::move(transport));
  onFaceCreated(face);*/

  // prepare accepting the next connection
  accept(onFaceCreated, onAcceptFailed);
}

AsyncIbrDtnClient::AsyncIbrDtnClient(const std::string &app,
                                     const std::string &host,
									 uint16_t port,
									 DtnChannel *pChannel,
									 boost::asio::io_service &ioService)
  : dtn::api::Client(app, m_socketStream),
    m_ibrdtndAddress(host, port),
	m_socketStream(new ibrcommon::tcpsocket(m_ibrdtndAddress)),
	m_ioService(ioService)
{
  NFD_LOG_CHAN_TRACE("AsyncIbrDtnClient CONSTRUCTOR");
  m_pChannel = pChannel;
  connect();
}

AsyncIbrDtnClient::~AsyncIbrDtnClient()
{
  NFD_LOG_CHAN_INFO("AsyncIbrDtnClient DESTRUCTOR");
}

void AsyncIbrDtnClient::received(const dtn::data::Bundle &b)
{
  NFD_LOG_CHAN_TRACE("AsyncIbrDtnClient RECEIVE BUNDLE");

  auto f1 = std::bind(&DtnChannel::processBundle, m_pChannel, _1);
  m_ioService.post([f1, b] {
    f1(b);
  });
}

void AsyncIbrDtnClient::eventConnectionDown() throw ()
{
  NFD_LOG_CHAN_INFO("AsyncIbrDtnClient receiver connection down!");
  Client::eventConnectionDown();
}

} // namespace face
} // namespace nfd
