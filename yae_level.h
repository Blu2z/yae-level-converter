#ifndef __GNUC__
#pragma once
#endif
#ifndef __YAE_LEVEL_H__
#define __YAE_LEVEL_H__

#include "xr_file_system.h"
#include "xr_matrix.h"
#include "xr_vector2.h"
#include "xr_vector3.h"
#include "xr_vector4.h"
#include "yae_mesh.h"
#include <map>

const char PA_GAME_MAPS[] = "$game_maps$";
const char PA_CURRENT_MAP[] = "$current_map$";

using namespace xray_re;
// buffer mask
enum level_buffers_id {
	YAE_MASK_INDICES	   = 0x1,
	YAE_MASK_POINTS         = 0x2,
	YAE_MASK_NORMALS        = 0x4,
	YAE_MASK_TEXCOORDS     = 0x8,
	YAE_MASK_LIGHTMAPS     = 0x10,
	YAE_MASK_COLORS	       = 0x20,
	YAE_MASK_TANGENTS       = 0x40,	// guess
	YAE_MASK_BINORMALS      = 0x80,	// guess
};
TYPEDEF_STD_VECTOR(fvector2);
TYPEDEF_STD_VECTOR(fvector3);

struct YAE_HEADER
{
	std::string id;
	std::string version;
};
class YAE_BUFFER:public xr_vbuf
{
public:
	YAE_BUFFER();
	~YAE_BUFFER();
	void		load(xr_reader& r);
	bool		has_points() const;
	bool		has_normals() const;
	bool		has_texcoords() const;
	bool		has_influences() const;
	bool		has_colors() const;
	bool		has_lightmaps() const;
	bool		has_indices() const;
	bool		has_tangents() const;
	bool		has_binormals() const;
	void		set_vertex_offset(uint32_t _offset);
	uint32_t	vertex_offset();
	void		set_normal_offset(uint32_t _offset);
	uint32_t	normal_offset();
	void		make_signature();
	const uint32_t&	operator[](size_t at) const;
	uint32_t&	operator[](size_t at);
				
	enum {
		S_INDICES	= 0x01,
		S_POINTS	= 0x02,
		S_NORMALS	= 0x04,
		S_TEXCOORDS	= 0x08,
		S_LIGHTMAPS	= 0x10,
		S_COLORS	= 0x20,
		S_TANGENTS	= 0x40,
		S_BINORMALS	= 0x80,
	};

private:
	uint32_t	m_vertex_offset;
	uint32_t	m_normal_offset;
	icolor*		m_raw_colors;
	uint32_t*	m_indices;
};
inline bool YAE_BUFFER::has_points() const { return !!(m_signature & S_POINTS); }
inline bool YAE_BUFFER::has_normals() const { return !!(m_signature & S_NORMALS); }
inline bool YAE_BUFFER::has_texcoords() const { return !!(m_signature & S_TEXCOORDS); }
inline bool YAE_BUFFER::has_lightmaps() const { return !!(m_signature & S_LIGHTMAPS); }
inline bool YAE_BUFFER::has_influences() const { return !!(m_signature & S_INFLUENCES); }
inline bool YAE_BUFFER::has_colors() const { return !!(m_signature & S_COLORS); }
inline bool YAE_BUFFER::has_indices() const { return !!(m_signature & S_INDICES); }
inline bool YAE_BUFFER::has_tangents() const { return !!(m_signature & S_TANGENTS); }
inline bool YAE_BUFFER::has_binormals() const { return !!(m_signature & S_BINORMALS); }
inline void YAE_BUFFER::set_vertex_offset(uint32_t offset) {m_vertex_offset = offset;};
inline uint32_t	YAE_BUFFER::vertex_offset() {return m_vertex_offset;};
inline void YAE_BUFFER::set_normal_offset(uint32_t offset) {m_normal_offset = offset;};
inline uint32_t	YAE_BUFFER::normal_offset() {return m_normal_offset;};
inline const uint32_t& YAE_BUFFER::operator[](size_t at) const { return m_indices[at]; }
inline uint32_t& YAE_BUFFER::operator[](size_t at) { return m_indices[at]; }

struct VisTreeNode {
	fvector3	unk_fvector3_1;
	fvector3	unk_fvector3_2;
	float		unk_float_1;
	fvector3	unk_fvector3_3;
	std::vector<uint32_t>	unk_vector;
	void		load(xr_reader& r);
};
struct YAEStaticLight {
	uint32_t	type;
	fvector3	unk_fvector3_1;
	fvector3	unk_fvector3_2;
	fvector3	unk_fvector3_3;
	std::vector<uint32_t> unk_params;
	void	load(xr_reader& r);
};
struct model_desc {
	fmatrix		m_xform;
	fbox		m_bbox;
};
struct YAEStaticModel {
	std::string		m_name;
	fbox			m_bbox;
	std::vector<model_desc> m_descs;
	void	load(xr_reader& r);
};

TYPEDEF_STD_VECTOR_PTR(VisTreeNode);
TYPEDEF_STD_VECTOR_PTR(YAEStaticLight);
TYPEDEF_STD_VECTOR_PTR(YAEStaticModel);
TYPEDEF_STD_VECTOR_PTR(YAE_BUFFER);

class yae_level {
public:
					~yae_level();
	void			read(xr_reader& r);
	void			read_hash(xr_reader& r);
	void			read_header(xr_reader& r);
	void			read_meshes(xr_reader& r);
	void			read_buffers(xr_reader& r);
	void			read_lightmaps(xr_reader& r);
	void			read_vistree(xr_reader& r);
	void			read_lights(xr_reader& r);
	void			read_models(xr_reader& r);
	void			save(const char* outpath, bool split, bool max);
	std::string		id();
	std::string		version();
	const YAE_BUFFER_vec&	buffers();
	const CLevelMesh_vec&	meshes();

private:
	YAE_HEADER		m_header;
	CLevelMesh_vec	m_meshes;
	YAE_BUFFER_vec	m_buffers;
	std::vector<std::string> m_lightmaps;
	VisTreeNode_vec	m_vistree;
	YAEStaticLight_vec	m_lights;
	YAEStaticModel_vec	m_models;
	std::map<std::string, std::string>	m_map;
};

inline std::string yae_level::id() {return m_header.id;};
inline std::string yae_level::version() {return m_header.version;};
inline const YAE_BUFFER_vec& yae_level::buffers(){return m_buffers;};
inline const CLevelMesh_vec& yae_level::meshes(){return m_meshes;};
class CMeshObject {		// for debug
public:
	CMeshObject();
	~CMeshObject();

public:
	uint32_t				face_count;
	uint32_t				vertex_count;
	uint32_t				index_count;
	std::vector<fvector3>	vertices;
	std::vector<fvector3>	normals;
	std::vector<fvector2>	texcoords;
	std::vector<uint32_t>	indices;
};
inline CMeshObject::CMeshObject()
{
	vertices.clear();
	normals.clear();
	texcoords.clear();
	indices.clear();
}
inline CMeshObject::~CMeshObject()
{
	vertices.clear();
	normals.clear();
	texcoords.clear();
	indices.clear();
}
TYPEDEF_STD_VECTOR_PTR(CMeshObject);

class CObj {
public:
	~CObj();
	void	set_name(const char* name);
	bool	skipping(const std::string& material);

public:
	std::string				m_name;
	std::string				m_material;
	std::vector<uint16_t>	m_meshes;
};
inline CObj::~CObj() {m_meshes.clear();}
inline void CObj::set_name(const char* name)
{
	m_name = name;
	if (!m_material.empty()) {
		m_name += '_';
		m_name += m_material;
	}
	m_name += ".obj";
}
inline bool CObj::skipping(const std::string& material) {return (material != m_material);}
TYPEDEF_STD_VECTOR_PTR(CObj);
#endif