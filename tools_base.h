#ifndef __GNUC__
#pragma once
#endif
#ifndef __TOOLS_BASE_H__
#define __TOOLS_BASE_H__
#include "xr_cl_parser.h"
#include "xr_file_system.h"
#include "yae_model.h"
#include "yae_level.h"

using namespace xray_re;

class tools_base {
public:
	virtual		~tools_base();

	enum source_format {
		TOOLS_AUTO	= 0,

		TOOLS_MODEL	= 0x001,
		TOOLS_LEVEL	= 0x002,
	};

	virtual bool	process(const cl_parser& cl) = 0;
	void		check_path(const char* path, bool& status) const;
};
inline tools_base::~tools_base() {}

#endif