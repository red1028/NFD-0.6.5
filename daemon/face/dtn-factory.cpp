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

#include "dtn-factory.hpp"
namespace nfd {
namespace face {

namespace ip = boost::asio::ip;

NFD_LOG_INIT(DtnFactory);
NFD_REGISTER_PROTOCOL_FACTORY(DtnFactory);

const std::string&
DtnFactory::getId() noexcept
{
  static std::string id("dtn");
  return id;
}


shared_ptr<DtnChannel>
DtnFactory::createChannel(const std::string &endpointPrefix, const std::string &endpointAffix, const std::string ibrdtndHost, uint16_t ibrdtndPort)
{
  auto it = m_channels.find(endpointAffix);
  if (it != m_channels.end())
    return it->second;
  
  auto channel = make_shared<DtnChannel>(endpointPrefix, endpointAffix, ibrdtndHost, ibrdtndPort);
  m_channels[endpointAffix] = channel;
  return channel;
}

void
DtnFactory::doProcessConfig(OptionalConfigSection configSection,
                            FaceSystem::ConfigContext& context)
{
  // dtn
  // {
  //   host localhost ;
  //   port 4550 ;
  //   endpointPrefix dtn://vicsnf1 ;
  //   endpointAffix /nfd ;
  // }

  std::string ibrdtndHost = "localhost";
  uint16_t ibrdtndPort = 4550;
  std::string endpointPrefix = "";
  std::string endpointAffix = "nfd";

  if (!configSection) {
    if (!context.isDryRun && !m_channels.empty()) {
      NFD_LOG_WARN("Cannot disable DTN channels after initialization");
    }
    return;
  }

  for (const auto& i : *configSection) {
	  if (i.first == "host") {
		  ibrdtndHost = i.second.get_value<std::string>();
		  NFD_LOG_TRACE("IBRDTND host set to " << ibrdtndHost);
	  }
	  else if (i.first == "port") {
		  ibrdtndPort = ConfigFile::parseNumber<uint16_t>(i, "dtn");
		  NFD_LOG_TRACE("IBRDTND port set to " << ibrdtndPort);
	  }
	  else if (i.first == "endpointPrefix") {
		  endpointPrefix = i.second.get_value<std::string>();
		  NFD_LOG_TRACE("IBRDTND endpoint prefix set to " << endpointPrefix);
	  }
	  else if (i.first == "endpointAffix") {
		  endpointAffix = i.second.get_value<std::string>();
		  NFD_LOG_TRACE("IBRDTND endpoint affix set to " << endpointAffix);
	  }
  }
  
  if (context.isDryRun) {
    return;
  }
  
  providedSchemes.insert("dtn");
  
  NFD_LOG_INFO("Setting up DTN");
  shared_ptr<DtnChannel> dtnChannel = this->createChannel(endpointPrefix, endpointAffix, ibrdtndHost, ibrdtndPort);
  dtnChannel->listen(this->addFace, nullptr);
  NFD_LOG_INFO("DTN setup finished");
}

void
DtnFactory::doCreateFace(const CreateFaceRequest& req,
                         const FaceCreatedCallback& onCreated,
                         const FaceCreationFailedCallback& onFailure)
{
  NFD_LOG_INFO("DTN doCreateFace");
  if (req.localUri) {
    NFD_LOG_TRACE("Cannot create unicast DTN face with LocalUri");
    onFailure(406, "Unicast DTN faces cannot be created with a LocalUri");
    return;
  }

  if (req.params.persistency == ndn::nfd::FACE_PERSISTENCY_ON_DEMAND) {
    NFD_LOG_TRACE("createFace does not support FACE_PERSISTENCY_ON_DEMAND");
    onFailure(406, "Outgoing DTN faces do not support on-demand persistency");
    return;
  }
  
  std::string dtnEndpoint = req.remoteUri.getScheme() + "://" + req.remoteUri.getHost() + req.remoteUri.getPath();
  std::string dtnAffix = req.remoteUri.getPath();

  // very simple logic for now
  for (const auto& i : m_channels) {
	if (i.first == dtnAffix ) {
	  i.second->connect(dtnEndpoint, req.params, onCreated, onFailure);
	  return;
    }
  }

  NFD_LOG_TRACE("No channels available to connect to " << dtnEndpoint);
  onFailure(504, "No channels available to connect");
}

std::vector<shared_ptr<const Channel>>
DtnFactory::doGetChannels() const
{
  return getChannelsFromMap(m_channels);
}


} // namespace face
} // namespace nfd
