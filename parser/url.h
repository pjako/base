#ifndef _URL_
#define _URL_

#ifndef API
#define API extern
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef enum url_errorCode {
	url_errorCode_ok = 0,
	url_errorCode_scheme,
	url_errorCode_userOrPass,
	url_errorCode_hostOrPort,
	url_errorCode_path,
	url_errorCode_query,
	url_errorCode_fragment,
} url_errorCode;

/**
 * Struct describing a parsed url.
 *
 * @example <scheme>://<user>:<pass>@<host>:<port>/<path>?<query>#<fragment>
 */
typedef struct Url {
	url_errorCode error;
	S8  		  scheme;
	S8  		  user;
	S8  		  pass;
	S8  		  host;
	i32   		  port;
	S8  		  path;
	S8  		  query;
	S8  		  fragment;
} Url;

/**
 * Parse an url specified by RFC1738 into its parts.
 *
 * @param urlStr url to parse.
 *
 * @return parsed url. If mem is NULL this value will need to be free:ed with free().
 */
API Url url_fromStr(S8 urlStr);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // _URL_