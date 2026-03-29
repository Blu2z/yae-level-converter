#include "yae_mesh.h"
#include "ds2_reader.h"
#include <cmath>

static bool fvec3_infinite(const fvector3& v) {
	return std::isinf(v.x) || std::isinf(v.y) || std::isinf(v.z);
}

// Helper derived class to access protected xr_vbuf members for DS2MD loading
struct yae_vbuf_loader : public xr_vbuf {
	void do_load_ds2md(xr_reader& r, uint16_t n) {
		clear();
		m_points = new fvector3[n];
		m_normals = new fvector3[n];
		m_texcoords = new fvector2[n];
		set_size(n);
		for (size_t i = 0; i < n; ++i) {
			r.r_fvector3(m_points[i]);
			r.r_fvector3(m_normals[i]);
			r.r_fvector2(m_texcoords[i]);
		}
		make_signature();
	}
};

void load_vbuf_ds2md(xr_vbuf& vb, xr_reader& r, uint16_t num_verts) {
	static_cast<yae_vbuf_loader&>(vb).do_load_ds2md(r, num_verts);
}

void CMesh::load(xr_reader& r, bool rotate_model)
{
	ds2_r_s(r, m_material);
	ds2_r_s(r, m_texture);
	fix_texture_name();
	ds2_r_s(r, m_normal_map);
	ds2_r_s(r, m_unk_texture1);
	ds2_r_s(r, m_unk_texture2);
	ds2_r_s(r, m_unk_texture3);
	r.r(m_bbox);
	uint32_t num_faces = r.r_u32();
	m_no_geometry_check = r.r_bool();
	m_num_bones = r.r_u32();
	m_ib.load(r, r.r_u32());
	load_vbuf_ds2md(m_vb, r, r.r_u16());

	// try to create fbox;
	m_debug_bbox.invalidate();
	for (uint32_t i = 0; i < m_vb.size(); ++i)
		m_debug_bbox.extend(m_vb.p(i));

	// need to change
/*	if (rotate_model) {
		uint_fast32_t v_count = m_vb.size();
		for (uint_fast32_t i = 0; i < v_count; ++i) {
//			m_vb.p(i).swap_yz();
//			m_vb.p(i).x *= -1;
		}
	}*/
	if (!m_no_geometry_check) {
		uint16_t i_count = m_ib.size();
		for (uint32_t i = 0; i < i_count; i += 3) {
			const fvector3& v1 = m_vb.p(m_ib[i]);
			const fvector3& v2 = m_vb.p(m_ib[i + 1]);
			const fvector3& v3 = m_vb.p(m_ib[i + 2]);
			if (fvec3_infinite(v1) || fvec3_infinite(v2) || fvec3_infinite(v3)) {
				m_ib[i] = 0;
				m_ib[i + 1] = 0;
				m_ib[i + 2] = 0;
				msg("triangle %d reset to zero size", i / 3);
			}
		}
	}
}
void CLevelMesh::load(xr_reader& r)
{
	m_dynamic = r.r_bool();
	ds2_r_s(r, m_material);
	ds2_r_s(r, m_texture);
	fix_texture_name();
	ds2_r_s(r, m_normal_map);
	ds2_r_s(r, m_unk_texture1);
	ds2_r_s(r, m_unk_texture2);
	ds2_r_s(r, m_unk_texture3);
	r.r(m_bbox);
	r.r_fvector3(unk_fvector3_3);
	unk_float_1 = r.r_float();
	r.r_fvector3(unk_fvector3_4);
	r.r_fvector3(unk_fvector3_5);
	for (int i = 0; i < 3; ++i)
		r.r_fvector3(unk_fvector3_6[i]);
	for (int i = 0; i < 8; ++i)
		r.r_fvector3(unk_fvector3_7[i]);
	unk_bool_1 = r.r_bool();
	r.r(unk_i32vector3_1);
	unk_int_1 = r.r_u32();
	m_face_count = r.r_u32();
	m_vert_count = r.r_u32();
	unk_int_2 = r.r_u32();
	m_ind_count = r.r_u32();
	m_strip = false;
	m_ib_id = r.r_u32();
	m_ib_offset = r.r_u32();
	if (m_ib_id == 0xFFFFFFFF) {
		m_strip = true;
		m_ind_count = r.r_u32();
		m_ib_id = r.r_u32();
		m_ib_offset = r.r_u32();
	} else {
		r.skip<uint32_t>(3);
	}
	m_vb_id = r.r_u32();
	m_vb_offset = r.r_u32();

	if (m_dynamic) {
		m_cb_id = r.r_u32();
		m_cb_offset = r.r_u32();
		r.r(m_xform);
		unk_int_3 = r.r_u32();
		unk_int_4 = r.r_u32();
	}
}
void CMesh::fix_texture_name() 
{
	size_t pos = m_texture.find_last_of('\\', std::string::npos);
	if (pos != std::string::npos) {
		m_texture.erase(m_texture.begin(), m_texture.begin() + pos + 1);
		std::transform(m_texture.begin(), m_texture.end(), m_texture.begin(), ::tolower);
	}
	pos = m_texture.find(".tga", std::string::npos);
	if (pos != std::string::npos)
		m_texture.erase(m_texture.begin() + pos - 3, m_texture.begin() + pos + 1);
}