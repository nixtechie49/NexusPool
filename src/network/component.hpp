#ifndef NEXUSPOOL_NETWORK_COMPONENT_HPP
#define NEXUSPOOL_NETWORK_COMPONENT_HPP

#include "socket_factory.hpp"

#include <memory>

namespace nexuspool {
namespace network {

class Component {
public:
    /// \brief Pointer to component
    using Uptr = std::unique_ptr<Component>;

    // Get the Socket_factory interface
    virtual Socket_factory::Sptr get_socket_factory() = 0;
};


} // namespace network
} // namespace nexuspool

#endif // NEXUSPOOL_NETWORK_COMPONENT_HPP
