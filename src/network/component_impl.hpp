#ifndef NEXUSPOOL_NETWORK_COMPONENT_IMPL_HPP
#define NEXUSPOOL_NETWORK_COMPONENT_IMPL_HPP

#include "create_component.hpp"
#include "socket_factory_impl.hpp"

#include <memory>
#include "asio/io_service.hpp"

namespace nexuspool {
namespace network {

class Component_impl : public Component 
{
public:
    Component_impl(std::shared_ptr<::asio::io_context> io_context)
        : m_socket_factory{std::make_shared<Socket_factory_impl>(std::move(io_context))}
	{
    }

    Socket_factory::Sptr get_socket_factory() override { return m_socket_factory; }

private:
    Socket_factory::Sptr m_socket_factory;
};

} // namespace network
} // namespace nexuspool

#endif // NEXUSPOOL_NETWORK_COMPONENT_IMPL_HPP
