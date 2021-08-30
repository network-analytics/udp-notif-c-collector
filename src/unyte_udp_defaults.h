#ifndef H_UNYTE_UDP_DEFAULTS
#define H_UNYTE_UDP_DEFAULTS

// unyte_collector
#define UNYTE_DEFAULT_VLEN 1
#define DEFAULT_SK_BUFF_SIZE 20971520 // 20MB of socket buffer size
#define OUTPUT_QUEUE_SIZE 1000        // output queue size
#define MONITORING_QUEUE_SIZE 0       // queue monitoring default to 0 (not running monitoring thread)
#define MONITORING_DELAY 3            // in seconds

#endif
