# THIS IS BETA SOFTWARE! #

This Pool can be setup for Prime or Hash channel.

Contact me if you have bug fixes.

# Dependencies: #
 - GMP (Mpir (on windows)

# Prerequisites: #


# Building windows #
There is a VS2017 project available.
 
# pool.conf file: #
Please create a pool.conf (example file included) with the following

```
#!json

"wallet_ip" : "127.0.0.1",   // The IP address of the NXS wallet the pool server should use 
"port" : 9549,              // The port the pool should run on

"daemon_threads" : 10,      // number of individual threads to connect to the wallet daemon 
"connection_threads" : 4,  // number of threads within each daemon connection that should support incoming connections
"enable_ddos" : "true",     // enable / disable DDOS protection
"ddos_rscore" : 20,         // DDOS rscore (look into the code for how this works)
"ddos_cscore" : 2,          // DDOS cscore (look into the code for how this works)
"min_share" : 40000000,     // pool minimum share difficulty
"pool_fee" : 1,             // pool fee percentage
"logfile" : "pool.log",	    // logfile additional to console output. If no logfile is given, than there is only console output
"database_type" : "sqlite",  // database used (currently supported sqlite)
"stats_db_server_ip" : "127.0.0.1",         // the IP address of the server for the statistics database (e.g. MySQL)
"stats_db_server_port" : 3306,              // port of the statistics database server (3306 is default for MySQL)
"stats_db_username" : "nxspool",            // username for the statistics database server
"stats_db_password" : "nxspool",            // password for the statistics database server
"connection_stats_frequency" : 10,          // interval (in seconds) to save the connection stats for each miner.  This is how frequent the current connection count, PPS, and WPS stats are saved
"connection_stats_series_frequency" : 300   // interval (in seconds) for each series of miner connection data (allowing for historical connection data to be collected every X seconds)
```