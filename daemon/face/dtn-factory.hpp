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

#ifndef NFD_DAEMON_FACE_DTN_FACTORY_HPP
#define NFD_DAEMON_FACE_DTN_FACTORY_HPP

#include "protocol-factory.hpp"
#include "dtn-channel.hpp"

namespace nfd {
namespace face {

class DtnFactory : public ProtocolFactory
{
public:
  static const std::string&
  getId() noexcept;
  
  using ProtocolFactory::ProtocolFactory;
  
  /**
   * \brief Exception of DtnFactory
   */
  class Error : public ProtocolFactory::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : ProtocolFactory::Error(what)
    {
    }
  };

  /**
   * \brief Create dtn channel using specified dtn socket
   *
   * If this method is called twice with the same path, only one channel
   * will be created.  The second call will just retrieve the existing
   * channel.
   *
   * \returns always a valid pointer to a DtnChannel object,
   *          an exception will be thrown if the channel cannot be created.
   *
   * \throws DtnFactory::Error
   */

  shared_ptr<DtnChannel>
  createChannel(const std::string &endpointPrefix, const std::string &endpointAffix, const std::string ibrdtndHost, uint16_t ibrdtndPort);

private: // from ProtocolFactory
  void
  doProcessConfig(OptionalConfigSection configSection,
                  FaceSystem::ConfigContext& context) override;

  void
  doCreateFace(const CreateFaceRequest& req,
               const FaceCreatedCallback& onCreated,
               const FaceCreationFailedCallback& onFailure) override;

  std::vector<shared_ptr<const Channel>>
  doGetChannels() const override;

private:
  std::map<std::string, shared_ptr<DtnChannel>> m_channels;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_DTN_FACTORY_HPP
