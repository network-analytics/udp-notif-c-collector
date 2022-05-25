#ifndef H_UNYTE_UDP_CONSTANTS
#define H_UNYTE_UDP_CONSTANTS

// Space constants
#define UNYTE_SPACE_STANDARD 0
#define UNYTE_SPACE_NON_STANDARD 1

// Supported media type TODO: to be defined in IANA
#define UNYTE_MEDIATYPE_RESERVED 0
#define UNYTE_MEDIATYPE_JSON 1
#define UNYTE_MEDIATYPE_XML 2
#define UNYTE_MEDIATYPE_CBOR 3

// Supported encoding in legacy draft: draft-ietf-netconf-udp-pub-channel-05
#define UNYTE_LEGACY_ENCODING_GPB 0
#define UNYTE_LEGACY_ENCODING_CBOR 1
#define UNYTE_LEGACY_ENCODING_JSON 2
#define UNYTE_LEGACY_ENCODING_XML 3

// Options types TODO: to define in the IANA
#define UNYTE_TYPE_RESERVED 0
#define UNYTE_TYPE_SEGMENTATION 1
#define UNYTE_TYPE_PRIVATE_ENCODING 2

static const int LEGACY_ET_TO_IANA[] = {UNYTE_MEDIATYPE_RESERVED, UNYTE_MEDIATYPE_CBOR, UNYTE_MEDIATYPE_JSON, UNYTE_MEDIATYPE_XML};
static const int SUPPORTED_ET_LEN = sizeof(LEGACY_ET_TO_IANA) / sizeof(int);

#endif
