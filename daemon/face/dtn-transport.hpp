/*
 * dtn-transport.hpp
 *
 *  Created on: Jul 21, 2016
 *      Author: root
 */

#ifndef NFD_DAEMON_FACE_DTN_TRANSPORT_HPP
#define NFD_DAEMON_FACE_DTN_TRANSPORT_HPP

#include "transport.hpp"
#include <ibrdtn/api/Client.h>

namespace nfd {
namespace face {

class DtnTransport : public Transport
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  DtnTransport(std::string localEndpoint, std::string remoteEndpoint, std::string ibrdtndHost, uint16_t ibrdtndPort);

  void
  receiveBundle(dtn::data::Bundle b);

protected:
  //void
  //canChangePersistencyToImpl(ndn::nfd::FacePersistency newPersistency) const final;

  void
  doClose() override;

private:
  virtual void
  doSend(Transport::Packet&& packet) override;

private:
  std::string m_ibrdtndHost;
  uint16_t m_ibrdtndPort;
};

} // namespace face
} // namespace nfd


#endif /* NFD_DAEMON_FACE_DTN_TRANSPORT_HPP */
