#ifndef CONSUL_PEER_HPP
#define CONSUL_PEER_HPP

#include <cstdint>
#include <sstream>
#include <string>

#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"
#include "json11.hpp"

namespace consul {
struct Peer final {
private:
  using HostT = std::string;
  using PortT = std::uint16_t;
public:

  HostT host;
  PortT port = 0;
  bool leader = false;

  Peer() {}
  Peer(HostT host_, PortT port_) : host{host_}, port{port_} {}

  static bool InitFromJson(Peer& peer, const json11::Json& json, std::string& err) {
    if (!json.is_string()) {
      err = "Expected a JSON string object as input";
      return false;
    }

    // WTF do you do when the server returns an empty string?  The cluster is
    // broken or split-brained.  Quorum isn't available for the agent.  Set
    // the peer's leader state to false and move on, but return true.  We
    // successfully parsed nothing, meaning no leader, but no error in the
    // API call.
    if (json.string_value().size() == 0) {
      peer.leader = false;
      return true;
    }

    std::vector<std::string> toks;
    boost::split(toks, json.string_value(), boost::is_any_of(":"), boost::token_compress_on);
    if (toks.size() != 2) {
      std::ostringstream errMsg;
      errMsg << "Expected a host:port pattern from string \"" << json.string_value() << "\"";
      err = errMsg.str();
      return false;
    }

    if (!peer.setHost(toks[0])) {
      std::ostringstream errMsg;
      errMsg << "Failed to set host: " << toks[0];
      err = errMsg.str();
      return false;
    }

    if (!peer.setPort(toks[1])) {
      std::ostringstream errMsg;
      errMsg << "Failed to set port: " << toks[1];
      err = errMsg.str();
      return false;
    }

    return true;
  }

  static bool InitFromJson(Peer& peer, const std::string& json_, std::string& err) {
    using json11::Json;

    auto jr = Json::parse(json_, err);
    if (!err.empty()) {
      std::ostringstream errMsg;
      errMsg << "Failed to parse JSON from (" << json_ << "): " << err;
      err = errMsg.str();
      return false;
    }

    if (!jr.is_string()) {
      std::ostringstream errMsg;
      errMsg << "Expected a JSON string (" << json_ << "): " << err;
      err = errMsg.str();
      return false;
    }

    return InitFromJson(peer, jr, err);
  }

  static bool InitFromJson(Peer& peer, const std::string& json_) {
    std::string err; // unused
    return InitFromJson(peer, json_, err);
  }

  std::string json() const { return json11::Json(*this).dump(); };
  json11::Json to_json() const {
    std::ostringstream ss;
    ss << host << ":" << port;
    return json11::Json{ss.str()};
  }

  std::string portStr() const noexcept {
    std::string str;
    try {
      str = ::boost::lexical_cast<std::string>(port);
    } catch(const ::boost::bad_lexical_cast &) {
      // Grr... shouldn't happen unless ENOMEM
    }
    return str;
  }

  std::string str() const {
    std::ostringstream ss;
    ss << host << ":" << port;
    return ss.str();
  }

  bool setHost(const HostT host_) noexcept {
    host = host_;
    return true;
  }

  bool setPort(const PortT port_) noexcept {
    port = port_;
    return true;
  }

  bool setPort(const std::string port_) noexcept {
    try {
      port = ::boost::lexical_cast<PortT>(port_);
      return true;
    } catch(const ::boost::bad_lexical_cast &) {
      return false;
    }
  }
};


bool
operator==(const Peer& a, const Peer& b) noexcept {
  return (a.host == b.host && a.port == b.port);
}


} // namespace consul

#endif // CONSUL_PEER_HPP
