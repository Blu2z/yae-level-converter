#ifndef __GNUC__
#pragma once
#endif
#ifndef __YAE_MODEL_H__
#define __YAE_MODEL_H__
#include "xr_object.h"
#include "yae_mesh.h"
#include <algorithm>
#include <map>

using namespace xray_re;

const uint32_t _INF = 0x7F800000;

const unsigned SHAPE_BALL = 0;
const unsigned SHAPE_BOX = 1;
const unsigned SHAPE_CYLINDER = 2;

const unsigned MODEL_READ_OK = 0;
const unsigned MODEL_READ_FAIL = 1;

const float	PI = 3.14159265359;


class CBody {
public:
	void	load(xr_reader& r);
public:
	std::string		m_name;
	uint8_t			m_shape_type;
	uint16_t		m_root_bone;
	fmatrix			xform;
	float			x;
	float			y;
	float			z;
};
TYPEDEF_STD_VECTOR_PTR(CBody);
struct SKeyFrame {
	void	load(xr_reader& r);
	uint8_t		m_flag;
	fvector4	unk_fvector4_1;
	fvector3	unk_fvector3_1;
	fvector3	unk_fvector3_2;
	float		m_start_time;
};
TYPEDEF_STD_VECTOR_PTR(SKeyFrame);
class CBoneAnims {
public:
	void	load(xr_reader& r);
public:
	uint16_t		m_id;
	std::string		m_name;
	SKeyFrame_vec	m_frames;
};
TYPEDEF_STD_VECTOR_PTR(CBoneAnims);
class CAnim {
public:
	void	load(xr_reader& r, std::string& version);
public:
	std::string		m_name;
	float			m_time;
	float			m_fps_inv;
	CBoneAnims_vec	m_bone_anims;
	uint16_t		unk_num;
	float*			unk_floats;
	std::string*	unk_strings;
};
TYPEDEF_STD_VECTOR_PTR(CAnim);
class yae_model: public xr_object {
public:
					~yae_model();
	virtual void	clear();
	bool			read(xr_reader& r, bool name_only, bool rotate_model = 0);
	bool			save_obj(const char *outpath, bool max);
	void			save_skl(const char *outpath);
	void			save_skls(const char *outpath);
	void			save_bones(const char *outpath);
	void			to_object();
	void			setup_bones();
	xr_surface*		create_surface(const xr_raw_surface& raw_surface) const;
	const std::string&	texture() const;

public:
	struct bone_io;
	struct partition_io;
	struct motion_io;
	struct bone_motion_io;

private:
	std::string		m_id;
	std::string		m_name;
	std::string		m_version;
	fbox			m_debug_bbox;
	CMesh_vec		m_model_meshes;
	CBody_vec		m_bodies;
	CAnim_vec		m_anims;
	uint16_t		m_unk_uint16_t;
};
inline const std::string& yae_model::texture() const { return m_name; }
#endif