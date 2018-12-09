/// @file
///
/// @author Elektrobit Automotive GmbH, 91058 Erlangen, Germany
///
/// @copyright 2018 Elektrobit Automotive GmbH
/// All rights exclusively reserved for Elektrobit Automotive GmbH,
/// unless otherwise expressly agreed.

#ifndef NEXUSPOOL_NETWORK_CREATE_COMPONENT_HPP
#define NEXUSPOOL_NETWORK_CREATE_COMPONENT_HPP

#include "component.hpp"
#include "asio/io_service.hpp"

#include <memory>


namespace nexuspool {
namespace network {

// Component factory

Component::Uptr create_component(std::shared_ptr<::asio::io_context> io_context);

} // namespace network
} // namespace nexuspool

#endif // NEXUSPOOL_NETWORK_CREATE_COMPONENT_HPP
