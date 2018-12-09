#ifndef NEXUSPOOL_NETWORK_UTILITY_HPP
#define NEXUSPOOL_NETWORK_UTILITY_HPP

#include <cstdlib>
#include <string>

//#ifdef _WIN32_WINNT
//#include <Iphlpapi.h>
//#include <Netioapi.h>
//#else
//#include <net/if.h>
//#endif

namespace nexuspool {
namespace network {

using Scope_id = unsigned int;

//inline Scope_id get_scope_id(std::string const& if_name)
//{
//    Scope_id scope_id = 0;
//    if (!if_name.empty()) {
//        scope_id = if_nametoindex(if_name.c_str());
//        if (scope_id == 0) {
//            char* endptr = nullptr;
//            scope_id = std::strtol(if_name.c_str(), &endptr, 10);
//        }
//    }
//    return scope_id;
//}

} // namespace network
} // namespace nexuspool


#endif // NEXUSPOOL_NETWORK_UTILITY_HPP
