#ifndef __GNUC__
#pragma once
#endif
#ifndef __YAE_MODEL_MESH_H__
#define __YAE_MODEL_MESH_H__

#include "xr_aabb.h"
#include "xr_file_system.h"
#include "xr_mesh_builder.h"
#include "xr_utils.h"

using namespace xray_re;

class CMesh {
public:
						CMesh();
						~CMesh();
	void				load(xr_reader& r, bool rotate_model = 0);
	void				fix_texture_name();
	const xr_vbuf&		vb() const;
	xr_vbuf&			vb();
	const xr_ibuf&		ib() const;
	xr_ibuf&			ib();
	const std::string&	texture() const;
	const std::string&	material() const;
	const fbox&			bbox() const;

protected:
	std::string		m_material;
	std::string		m_texture;
	std::string		m_normal_map;
	std::string		m_unk_texture1;
	std::string		m_unk_texture2;
	std::string		m_unk_texture3;
	fbox			m_bbox;
	bool			m_no_geometry_check;
	uint32_t		m_num_bones;
	xr_vbuf			m_vb;
	xr_ibuf			m_ib;

public:
	fbox			m_debug_bbox;
};
class CLevelMesh: public CMesh {
public:
	void		load(xr_reader& r);
	uint32_t	ib_id();
	uint32_t	vb_id();
	bool		strip();	
	bool		dynamic();	
	uint32_t	face_count();
	uint32_t	vert_count();
	uint32_t	ind_count();
	uint32_t	vert_offset();
	uint32_t	ind_offset();
	fmatrix*	xform();

private:
	/* config */
	bool			m_dynamic;
	bool			m_strip;
	fvector3		unk_fvector3_3;	//12
	float			unk_float_1;	//4
	fvector3		unk_fvector3_4;	//12
	fvector3		unk_fvector3_5;	//12
	fvector3		unk_fvector3_6[3];	//36
	fvector3		unk_fvector3_7[8];	//96
	bool			unk_bool_1;			//1
	i32vector3		unk_i32vector3_1;	//12
	uint32_t		unk_int_1;
	uint32_t		m_face_count;
	uint32_t		m_vert_count;
	uint32_t		m_ind_count;
	uint32_t		unk_int_2;	//4
	uint32_t		m_ib_id;	//4
	uint32_t		m_ib_offset;	//4
	uint32_t		m_vb_id;	//4
	uint32_t		m_vb_offset;	//4
	//dynamic
	uint32_t		m_cb_id;		//4			// color_buffer_id
	uint32_t		m_cb_offset;		//4			// color_buffer_offset
	fmatrix			m_xform;			//64		// transformation matrix
	uint32_t		unk_int_3;		//4
	uint32_t		unk_int_4;		//4
};
inline CMesh::CMesh() {};
inline const xr_vbuf& CMesh::vb() const { return m_vb; }
inline const xr_ibuf& CMesh::ib() const { return m_ib; }
inline xr_vbuf& CMesh::vb() { return m_vb; }
inline xr_ibuf& CMesh::ib() { return m_ib; }
inline const std::string& CMesh::texture() const { return m_texture; }
inline const std::string& CMesh::material() const { return m_material; }
inline const fbox& CMesh::bbox() const {return m_bbox;}
inline CMesh::~CMesh() {m_bbox.null();};
inline bool CLevelMesh::strip() { return m_strip; }
inline bool CLevelMesh::dynamic() { return m_dynamic; }
inline uint32_t CLevelMesh::face_count() { return m_face_count; }
inline uint32_t CLevelMesh::vert_count() { return m_vert_count; }
inline uint32_t CLevelMesh::ind_count() { return m_ind_count; }
inline uint32_t CLevelMesh::vert_offset() { return m_vb_offset; }
inline uint32_t CLevelMesh::ind_offset() { return m_ib_offset; }
inline uint32_t CLevelMesh::vb_id() { return m_vb_id; }
inline uint32_t CLevelMesh::ib_id() { return m_ib_id; }
inline fmatrix* CLevelMesh::xform() { return &m_xform; }
TYPEDEF_STD_VECTOR_PTR(CMesh);
TYPEDEF_STD_VECTOR_PTR(CLevelMesh);

#endif