#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"
#include "parser/url.h"

LOCAL u32 url__defaultPortForScheme(S8 scheme) {
	if (scheme.size == 0) return 0;
	if (str_isEqual(scheme, str_lit("http"))   == 0) return 80;
	if (str_isEqual(scheme, str_lit("https"))  == 0) return 443;
	if (str_isEqual(scheme, str_lit("ftp"))    == 0) return 21;
	if (str_isEqual(scheme, str_lit("ssh"))    == 0) return 22;
	if (str_isEqual(scheme, str_lit("telnet")) == 0) return 23;
	return 0;
}

LOCAL S8 url__parseScheme(S8 url, Url* out) {
    i64 start = str_findChar(url, ':');
    if (start < 0) return url;
	S8 schemesep = str_subStr(url, start, 0);

    // ... is this the user part of a user/pass pair or the separator host:port? ...
    if (str_findChar(schemesep, '/') != 1) return url;

    if (str_findChar(schemesep, '/') != 2) {
        out->error = url_errorCode_scheme;
        return str_lit("");
    }
    out->scheme = str_subStr(url, 0, start);
    if (out->scheme.size == 0) {
        out->error = url_errorCode_scheme;
        return str_lit("");
    }
    return str_subStr(url, 3, 0);
}

LOCAL S8 url__parseUserPass(S8 url, Url* out) {
    i64 start = str_findChar(url, '@');
	i64 atpos = str_findChar(str_subStr(url, start, 0), '@');
	if (start >= 0) {
		// ... check for a : before the @ ...
        i64 passsep = str_findChar(str_subStr(url, 0, atpos), ':');
		if (passsep < 0) {
            out->pass = str_lit("");
            out->user = str_subStr(url, 0, atpos);
		} else {
            out->user = str_subStr(url, 0, passsep);
            out->pass = str_subStr(url, passsep + 1, atpos - passsep - 1);
		}

		if(out->user.size == 0 || out->pass.size == 0) {
            out->error = url_errorCode_userOrPass;
            return str_lit("");
        }

		return str_subStr(url, atpos + 1, 0);
	} else {
       out->pass = str_lit("");
       out->user = str_lit(""); 
    }

	return url;
}

LOCAL S8 url__parseHostPort(S8 url, Url* out) {
	out->port = url__defaultPortForScheme(out->scheme);

	i64 portsep = str_findChar(url, ':');
	i64 pathsep = str_findChar(url, '/');

	u64 hostlen = 0;

	if (portsep < 0) {
		pathsep = str_findChar(url, '/');
		hostlen = pathsep == 0x0 ? url.size : pathsep - url.size;
	} else {
		if(pathsep && portsep && (pathsep < portsep)) {
			// ... path separator was before port-separator, i.e. the : was not a port-separator! ...
			hostlen = pathsep;
		} else {
			out->port = str_parseU32(str_subStr(url, portsep + 1, 0));
			hostlen = portsep;
			pathsep = str_findChar(str_subStr(url, portsep, 0), '/');
		}
	}

	if ( hostlen > 0 ) {
		out->host = str_subStr(url, 0, hostlen);
		if (out->host.size == 0) return str_lit("");
	}

	// ... parse path ... TODO: extract to own function.
	if (pathsep >= 0) {
		// ... check if there are any query or fragment to parse ...
		i64 pathEnd = str_find(str_subStr(url, pathsep, 0), str_lit("?#"));

		u64 reslen = 0;
		if (pathEnd >= 0) {
            reslen = pathEnd;
        } else {
            reslen = 0;
        }

        out->path = str_subStr(url, pathsep, reslen);
		if (out->path.size == 0) {
            out->error = url_errorCode_hostOrPort;
            return str_lit("");
        }
		return str_subStr(url, pathsep + reslen, 0);
	}

	return url;
}

LOCAL S8 url__parseQuery(S8 url, Url* out) {
	// ... do we have a query? ...
	if (str_startsWithChar(url, '?')) return url;
		

	// ... skip '?' ...
    url = str_subStr(url, 1, 0);

	// ... find the end of the query ...
	u64 queryLen = 0;

	i64 fragmentStart = str_startsWithChar(url, '#');
	if (fragmentStart < 0) {
        queryLen = 0;
    }

    out->query = str_subStr(url, 0, queryLen);

    if (out->fragment.size == 0) {
        out->error = url_errorCode_query;
        return str_lit(""); 
    }

    return str_subStr(url, queryLen, 0);
}

LOCAL void url__parseFragment(S8 url, Url* out) {
	// ... do we have a fragment? ...
	if (str_startsWithChar(url, '#')) return;

	// ... skip '#' ...
    url = str_subStr(url, 1, 0);

    out->fragment = url;

    if (out->fragment.size == 0) {
        out->error = url_errorCode_fragment;
    }
}

Url url_fromStr(S8 url) {
	Url out = {0};
    out.error = url_errorCode_ok;
	out.path = str_lit("/");

	url = url__parseScheme(url, &out);
    if (out.error != url_errorCode_ok) return out;
	url = url__parseUserPass(url, &out);
    if (out.error != url_errorCode_ok) return out;
	url = url__parseHostPort(url, &out);
    if (out.error != url_errorCode_ok) return out;
	url = url__parseQuery(url, &out);
    if (out.error != url_errorCode_ok) return out;
	url__parseFragment(url, &out);

    if (out.error == url_errorCode_ok) {
        if (out.host.size == 0) {
	        out.host = str_lit("localhost");
        }
        if (out.path.size == 0) {
	        out.path = str_lit("/");
        }
    }

	return out;
}