#ifndef NEXUSPOOL_BANNED_USERS_HPP
#define NEXUSPOOL_BANNED_USERS_HPP

#include <mutex>
#include <unordered_set>
#include <string>
#include <fstream>
#include <memory>

namespace nexuspool
{
	
	using BannedUsersType = std::unordered_set<std::string>;

// TODO: add database persister
// currently only file persister available
class Banned_persister_file
{
public:

	static void save(std::string target_file, BannedUsersType const& banned_list)
	{
		if(!banned_list.empty())
		{
			std::ofstream banned_list_file(target_file, std::ios::out);
			for(auto const banned_entry : banned_list)
			{
				banned_list_file << banned_entry << std::endl;
			}

			banned_list_file.close();
		}
	}
	
	static void load(std::string const& source_file, BannedUsersType banned_list)
	{
		if(banned_list.empty())
		{
			std::ifstream banned_list_file(source_file);
			std::string line;
			
			while(std::getline(banned_list_file, line))
			{
				banned_list.emplace(line);  
			}
		}
	}
	
};


// maintains a list of banned accounts and ip addresses. During server runtime, all operations are handled on
// the list, no IO involved. The banned accounts/ip addresses are loaded and saved on server startup/shutdown
// Threadsafe
template<typename PersisterType>
class Banned_users
{
public:

	Banned_users(std::string banned_accounts_source, std::string banned_ip_addresses_source) 
	: m_banned_accounts_source{std::move(banned_accounts_source)}
	, m_banned_ip_addresses_source{std::move(banned_ip_addresses_source)}
	{}

	// load the banned accounts and ip addresses on server startup
	void load_banned_users();
	// save the banned accounts and ip addresses on server shutdown
	void save_banned_users();
	
	void add_banned_account(std::string const& account);
	void add_banned_ip_address(std::string const& ip_address);
	
	bool is_ip_address_banned(std::string const& ip_address) const;	
	bool is_account_banned(std::string const& account) const;

private:

	std::string m_banned_accounts_source;
	std::string m_banned_ip_addresses_source;

	mutable std::mutex m_banned_accounts_mutex;
	mutable std::mutex m_banned_ip_addresses_mutex;
	BannedUsersType m_banned_accounts;
	BannedUsersType m_banned_ip_addresses;
	
	PersisterType m_persister_accounts;
	PersisterType m_persister_ip_addresses;
};

template<typename PersisterType>
inline void Banned_users<PersisterType>::load_banned_users()
{
	// scoped_lock only available in C++17
	std::lock_guard<std::mutex> lk1(m_banned_ip_addresses_mutex);
    std::lock_guard<std::mutex> lk2(m_banned_accounts_mutex);
    {
		m_persister_accounts.load(m_banned_accounts_source, m_banned_accounts);
    }
	m_persister_ip_addresses.load(m_banned_ip_addresses_source, m_banned_ip_addresses);
}

template<typename PersisterType>
inline void Banned_users<PersisterType>::save_banned_users()
{
	// scoped_lock only available in C++17
	std::lock_guard<std::mutex> lk1(m_banned_ip_addresses_mutex);
    std::lock_guard<std::mutex> lk2(m_banned_accounts_mutex);
    {
		m_persister_accounts.save(m_banned_accounts_source, m_banned_accounts);
    }
	m_persister_ip_addresses.save(m_banned_ip_addresses_source, m_banned_ip_addresses);
}

template<typename PersisterType>
inline void Banned_users<PersisterType>::add_banned_account(std::string const& account)
{
	std::lock_guard<std::mutex> lk(m_banned_accounts_mutex);
	
	m_banned_accounts.emplace(account);  
}

template<typename PersisterType>
inline void Banned_users<PersisterType>::add_banned_ip_address(std::string const& ip_address)
{
	std::lock_guard<std::mutex> lk(m_banned_ip_addresses_mutex);
	
	m_banned_ip_addresses.emplace(ip_address);  
}

template<typename PersisterType>
inline bool Banned_users<PersisterType>::is_ip_address_banned(std::string const& ip_address) const
{
	std::lock_guard<std::mutex> lk(m_banned_ip_addresses_mutex);
	if(m_banned_ip_addresses.find(ip_address) != m_banned_ip_addresses.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

template<typename PersisterType>
inline bool Banned_users<PersisterType>::is_account_banned(std::string const& account) const
{
	std::lock_guard<std::mutex> lk(m_banned_accounts_mutex);
	if(m_banned_accounts.find(account) != m_banned_accounts.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

}


#endif // NEXUSPOOL_BANNED_USERS_HPP