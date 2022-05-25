#ifndef H_UNYTE_UDP_DEFAULTS
#define H_UNYTE_UDP_DEFAULTS

// unyte_collector
#define UNYTE_DEFAULT_VLEN 1
#define DEFAULT_SK_BUFF_SIZE 20971520 // 20MB of socket buffer size
#define OUTPUT_QUEUE_SIZE 1000        // output queue size
#define MONITORING_QUEUE_SIZE 0       // queue monitoring default to 0 (not running monitoring thread)
#define MONITORING_DELAY 3            // in seconds

// listening_worker
#define UDP_SIZE 65535         // max UDP packet size
#define PARSER_QUEUE_SIZE 500  // input queue size
#define DEFAULT_NB_PARSERS 10  // number of parser workers instances
#define CLEANUP_FLAG_CRON 1000 // clean up cron in milliseconds

// monitoring_worker
#define ODID_COUNTERS 10    // hashmap modulo
#define ACTIVE_GIDS 500    // how many active odids do we are waiting for
#define ODID_TIME_TO_LIVE 4 // times monitoring thread consider observation_domain_id active without stats

// Segmentation_buffer parameters  | not settable by the user
#define SIZE_BUF 10000         // size of buffer
#define CLEAN_UP_PASS_SIZE 500 // number of iterations to clean up
#define CLEAN_COUNT_MAX 50     // clean up segment buffer when count > CLEAN_COUNT_MAX
#define EXPIRE_MSG 3           // seconds to consider the segmented message in the buffer expired (not receiving segments anymore)

#endif
