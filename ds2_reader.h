#ifndef __DS2_READER_H__
#define __DS2_READER_H__

#include "xr_reader.h"
#include <string>

// DS2 files use uint16 length-prefixed strings, unlike xray_re's r_s()
// which reads newline-terminated strings.
inline void ds2_r_s(xray_re::xr_reader& r, std::string& value)
{
	uint16_t len = r.r_u16();
	const char* p = r.pointer<const char>();
	value.assign(p, len);
	r.advance(len);
}

#endif
