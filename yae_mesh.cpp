#include "yae_mesh.h"

void CMesh::load(xr_reader& r, bool rotate_model)
{
	r.r_string(m_material);
	r.r_string(m_texture);
	fix_texture_name();
	r.r_string(m_normal_map);
	r.r_string(m_unk_texture1);
	r.r_string(m_unk_texture2);
	r.r_string(m_unk_texture3);
	r.r(m_bbox);
	uint32_t num_faces = r.r_u32();
	m_no_geometry_check = r.r_bool();
	m_num_bones = r.r_u32();
	m_ib.load(r, r.r_u32(), num_faces);
	m_vb.load_ds2md(r, r.r_u16());

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
			fvector3& v1 = m_vb.p(m_ib[i]);
			fvector3& v2 = m_vb.p(m_ib[i + 1]);
			fvector3& v3 = m_vb.p(m_ib[i + 2]);
			if (v1.infinite() || v2.infinite() || v3.infinite()) {
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
	r.r_string(m_material);
	r.r_string(m_texture);
	fix_texture_name();
	r.r_string(m_normal_map);
	r.r_string(m_unk_texture1);
	r.r_string(m_unk_texture2);
	r.r_string(m_unk_texture3);
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
	r.r_i32vector3(unk_i32vector3_1);
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