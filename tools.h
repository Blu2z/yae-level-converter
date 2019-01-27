#ifndef __GNUC__
#pragma once
#endif
#ifndef __TOOLS_H__
#define __TOOLS_H__
#include "tools_base.h"
#include "object_tools.h"

using namespace xray_re;

class model_tools: public object_tools {
public:
	virtual bool	process(const cl_parser& cl);
};

class level_tools: public tools_base {
public:
	virtual bool	process(const cl_parser& cl);
};

#endif