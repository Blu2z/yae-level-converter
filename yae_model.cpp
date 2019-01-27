#include "yae_model.h"

bool compare(CMesh *first, CMesh *last) {return first->texture() < last->texture();};
void CBody::load(xr_reader& r)
{
	r.r_string(m_name);
	m_shape_type = r.r_u8();
	m_root_bone = r.r_u16();
	r.r(xform);
	if (m_shape_type == SHAPE_BOX) {
		x = r.r_float();
		y = r.r_float();
		z = r.r_float();
	} else {
		if (m_shape_type != SHAPE_BALL) {
			if (m_shape_type == SHAPE_CYLINDER) {
				x = r.r_float();
				y = r.r_float();
			}
		} else {
			x = r.r_float();
		}
	}
}
void CAnim::load(xr_reader& r, std::string& version)
{
	r.r_string(m_name);
	m_time = r.r_float();
	m_fps_inv = r.r_float();
	uint16_t num_bones = r.r_u16();
	m_bone_anims.reserve(num_bones);
	for (uint16_t i = 0; i < num_bones; ++i) {
		CBoneAnims *anim = new CBoneAnims;
		anim->load(r);
		m_bone_anims.push_back(anim);
	}
	if (version == "1.0") {
		unk_num = r.r_u16();
		if (unk_num) {
			unk_floats = new float[unk_num];
			unk_strings = new std::string[unk_num];
			for (uint16_t j = 0; j < unk_num; ++j) {
				unk_floats[j] = r.r_float();
				r.r_string(unk_strings[j]);
			}
		}
	}
}
void CBoneAnims::load(xr_reader& r)
{
	m_id = r.r_u16();
	r.r_string(m_name);
	uint32_t num_frames = r.r_u32();
	m_frames.reserve(num_frames);
	for (uint32_t i = 0; i < num_frames; ++i) {
		SKeyFrame *frame = new SKeyFrame;
		frame->load(r);
		m_frames.push_back(frame);
	}
}
void SKeyFrame::load(xr_reader& r)
{
	m_flag = r.r_u8();
	r.r_fvector4(unk_fvector4_1);
	unk_fvector4_1.normalize();
	r.r_fvector3(unk_fvector3_1);
	r.r_fvector3(unk_fvector3_2);
	m_start_time = r.r_float();
}
void yae_model::to_object()
{
	m_flags = EOF_DYNAMIC;
	m_rotation.x += PI / 2;
	xr_mesh_builder* mesh = new xr_mesh_builder;
	size_t vb_reserve = 0, ib_reserve = 0;
	unsigned vb_signature = 0;
	for (CMesh_vec_it it = m_model_meshes.begin(), end = m_model_meshes.end(); it != end; ++it) {
		CMesh* m = *it;
		vb_signature |= m->vb().signature();
		vb_reserve += m->vb().size();
		ib_reserve += m->ib().size();
	}
	mesh->prepare(vb_signature, vb_reserve, ib_reserve);
	uint16_t shader_id = 0;
	for (CMesh_vec_it it = m_model_meshes.begin(), end = m_model_meshes.end(); it != end; ++it) {
		CMesh* m = *it;
		mesh->push(m->vb(), m->ib(), shader_id, shader_id);
		++shader_id;
	}
	mesh->compact_geometry();
	mesh->remove_duplicate_faces();
	mesh->remove_back_faces();
	mesh->commit(*this);
	mesh->name() = m_name;
	denominate_surfaces();
}
struct yae_model::bone_io: public xr_bone {
	void	import(xr_reader& r);
	void	define(uint16_t id, const std::string& name);
};
struct yae_model::partition_io: public xr_partition {
	void	import(xr_reader& r, xr_bone_vec& all_bones);
};
struct yae_model::bone_motion_io: public xr_bone_motion {
	void	import(xr_reader& r, uint_fast32_t num_keys);
};
struct yae_model::motion_io: public xr_skl_motion {
			motion_io();
	void		import_bone_motions(xr_reader& r, xr_bone_vec& all_bones);
};

inline yae_model::motion_io::motion_io() { m_fps = OGF4_MOTION_FPS; }
inline void yae_model::bone_io::import(xr_reader& r)
{
	r.r_string(m_name);
	m_vmap_name = m_name;
	r.r(m_bind_xform);

	/* 
		calculate full invert xform here
	*/

	m_bind_xform.get_xyz_i(m_bind_rotate);
	m_bind_offset.set(m_bind_xform.c);
	m_bind_offset.mul(0.01);

	m_parent_id = r.r_u16();
	uint16_t num_bones_connected = r.r_u16();
	m_children.reserve(num_bones_connected);
	r.skip<uint16_t>(num_bones_connected);			// избыточная инфа, все дочерние кости ищутся далее
	uint16_t unk_uint16_t2 = r.r_u16();
}
void yae_model::bone_io::define(uint16_t id, const std::string& name)
{
	m_id = id;
	m_name = name;
}
inline void yae_model::partition_io::import(xr_reader& r, xr_bone_vec& all_bones)
{
	r.r_string(m_name);
	uint16_t num = r.r_u16();
	for (uint_fast32_t n = num; n; --n) {
		uint16_t id = r.r_u16();
		if (all_bones.size() <= id)
			all_bones.resize(id + 1);
		if (all_bones[id] == 0) {
			yae_model::bone_io* bone = new yae_model::bone_io;
			bone->define(uint16_t(id), "default_bone");
			all_bones[id] = bone;
		} else {
			xr_assert(all_bones.at(id)->id() == id);
		}
//		if (num < all_bones.size())
			m_bones.push_back(all_bones[id]->name().c_str());
	}
	uint16_t num_parts = r.r_u16();
	r.skip<uint16_t>(num_parts);
}
struct read_bone {
	yae_model& model;
	read_bone(yae_model& _model): model(_model) {}
	void operator()(xr_bone*& bone, xr_reader& r) {
		yae_model::bone_io* _bone = new yae_model::bone_io;
		_bone->import(r);
		bone = _bone;
	}
};
struct read_partition {
	xr_bone_vec& all_bones;
	read_partition(xr_bone_vec& _all_bones): all_bones(_all_bones) {}
	void operator()(xr_partition*& _part, xr_reader& r) {
		yae_model::partition_io* part = new yae_model::partition_io;
		_part = part;
		part->import(r, all_bones);
	}
};
void yae_model::setup_bones()
{
	uint16_t bone_id = 0;
	for (xr_bone_vec_it it = m_bones.begin(), end = m_bones.end();
			it != end; ++it, ++bone_id) {
		(*it)->setup_yae(bone_id, *this);
	}
//	m_bones[0]->calculate_bind();
}
xr_surface* yae_model::create_surface(const xr_raw_surface& raw_surface) const
{
	xr_surface* surface = new xr_surface(true);
	if (m_model_meshes.size() > 1)
		surface->texture() = m_model_meshes.at(raw_surface.texture)->texture();
	else
		surface->texture() = m_model_meshes.at(0)->texture();
	surface->eshader() = "models\\model";
	if (raw_surface.two_sided())
		surface->set_two_sided();
	return surface;
}
bool yae_model::read(xr_reader& r, bool name_only, bool rotate_model)
{
	r.r_string(m_id);
	if (m_id != "DS2ModelFile_1") {
		msg("model has incompatible file version (0.5) or this is not model at all");
		return MODEL_READ_FAIL;
	}
	r.r_string(m_version);
	if (m_version != "1.0" && m_version != "0.9") {
		msg("model has incompatible file version (%s)", m_version.data());
		return MODEL_READ_FAIL;
	}
	r.r_string(m_name);

	if (!name_only) {
		// meshes
		uint8_t num_meshes = r.r_u8();
		m_model_meshes.reserve(num_meshes);
		m_debug_bbox.invalidate();
		for (uint8_t i = 0; i < num_meshes; ++i) {
			CMesh *mesh = new CMesh;
			mesh->load(r, rotate_model);
			m_debug_bbox.merge(mesh->m_debug_bbox);
			m_model_meshes.push_back(mesh);
		}
		// bodies
		uint8_t num_bodies = r.r_u8();
		m_bodies.reserve(num_bodies);
		for (uint8_t i = 0; i < num_bodies; ++i) {
			CBody *body = new CBody;
			body->load(r);
			m_bodies.push_back(body);
		}
		// bones
		if (r.r_u8()) {
			uint16_t num_bones = r.r_u16();
			assert(m_bones.empty());
			r.r_seq(num_bones, m_bones, read_bone(*this));
			setup_bones();
		}
		if (!r.eof()) {
			// anims
			uint16_t unk1 = r.r_u16();
			uint16_t *unk2 = new uint16_t[unk1];
			r.r_cseq(unk1, unk2);
			uint16_t num_anims = r.r_u16();
			m_anims.reserve(num_anims);
			for (uint16_t i = 0; i < num_anims; ++i) {
				CAnim *anim = new CAnim;
				anim->load(r, m_version);
				m_anims.push_back(anim);
			}
			// partitions
			assert(m_partitions.empty());
			r.r_seq(r.r_u16(), m_partitions, read_partition(m_bones));
			setup_partitions();
			m_unk_uint16_t = r.r_u16();
			assert(r.eof());
//			m_bones.clear();
		}

	}
	return MODEL_READ_OK;
}
bool yae_model::save_obj(const char *outpath, bool max) 
{
	xr_file_system& fs = xr_file_system::instance();
	std::string obj_name = outpath;
	obj_name += ".obj";
	xr_writer *w = fs.w_open(obj_name.data());
	obj_name.clear();

	/* sort meshes by texture */
	std::sort(m_model_meshes.begin(), m_model_meshes.end(), compare);

	/* prepare constants */
	msg("calculating constants");
	std::vector<std::string> materials;
	materials.reserve(m_model_meshes.size());
	uint32_t vert_count = 0;
	uint32_t face_count = 0;
	for (CMesh_vec_it it = m_model_meshes.begin(), end = m_model_meshes.end(); it != end; ++it) {
		face_count += (*it)->ib().size() / 3;
		vert_count += (*it)->vb().size();
		if (materials.empty() || (std::find(materials.begin(), materials.end(), (*it)->texture()) == materials.end()))
			materials.push_back((*it)->texture());
	}

	/* write mtl file */
	msg("writing materials");
	std::string mtl_name = outpath;
	mtl_name += ".mtl";
	xr_writer *w_mtl = fs.w_open(mtl_name.data());
	std::string textures_path = fs.resolve_path(PA_GAME_TEXTURES);
	for (std::vector<std::string>::iterator it = materials.begin(), end = materials.end(); it != end; ++it) {
		char *tex_name = new char[0x100];
		strcpy_s(tex_name, 0x100, textures_path.data());
		strcat_s(tex_name, 0x100, "$dds\\");
		strcat_s(tex_name, 0x100, it->data());
		w_mtl->w_sf("\nnewmtl \"%s\"\nmap_Kd \"%s.dds\"\n", it->data(), tex_name);
		delete[] tex_name;
	}
	fs.w_close(w_mtl);

	/* statistics */
	std::string stat_name = outpath;
	stat_name += "_stat.txt";
	xr_writer *w_stat = fs.w_open(stat_name.data());
	w_stat->w_sf("vertices: %d\r\nfaces: %d\r\nmeshes: %d\r\ntextures: %d\r\n", vert_count, face_count, m_model_meshes.size(), materials.size()); /* \r because of text file*/
	fs.w_close(w_stat);
	stat_name.clear();
	materials.clear();

	/* get mtl-file name */
	size_t pos = mtl_name.find_last_of("/\\", std::string::npos);
	if (pos != std::string::npos)
		mtl_name.erase(mtl_name.begin(), mtl_name.begin() + pos + 1);

	/* prepare buffers */
	uint32_t v_offset = 0;
	for (uint_fast32_t i = 0; i < m_model_meshes.size(); ++i) {
		CMesh *&curr_m = m_model_meshes[i];
		xr_ibuf &ib = curr_m->ib();
		uint32_t ib_size = curr_m->ib().size();
		if (i == 0)
			v_offset = 0;
		else
			v_offset += m_model_meshes[i - 1]->vb().size();
		for (uint_fast32_t j = 0; j < ib_size; ++j)
			ib[j] += v_offset + 1;
	}
	v_offset += m_model_meshes[0]->vb().size();

	/* write obj file */
	msg("writing %s", outpath);
	w->w_sf("\n# This file is generated by YAE level converter\n");
	w->w_sf("\nmtllib %s\n", mtl_name.data());
	mtl_name.clear();

		/* vertices */
	msg("...vertices");
	w->w_sf("\n# %d vertices\n", v_offset);
	for (CMesh_vec_it it = m_model_meshes.begin(), end = m_model_meshes.end(); it != end; ++it) {	
		const xr_vbuf *vb = &((*it)->vb());
		uint32_t size = vb->size();
		for (uint32_t i = 0; i < size; ++i)
			w->w_sf("v %f %f %f\n", vb->p(i).x, vb->p(i).y, vb->p(i).z);
	}

		/* vertex normals */
	msg("...normals");
	w->w_sf("\n# %d vertex normals\n", v_offset);
	for (CMesh_vec_it it = m_model_meshes.begin(), end = m_model_meshes.end(); it != end; ++it) {	
		const xr_vbuf *vb = &((*it)->vb());
		uint32_t size = vb->size();
		for (uint32_t i = 0; i < size; ++i)
			w->w_sf("vn %f %f %f\n", vb->n(i).x, vb->n(i).y, vb->n(i).z);
	}

		/* texture coordinates */
	msg("...texcoords");
	w->w_sf("\n# %d texcoords\n", v_offset);
	for (CMesh_vec_it it = m_model_meshes.begin(), end = m_model_meshes.end(); it != end; ++it) {	
		const xr_vbuf *vb = &((*it)->vb());
		uint32_t size = vb->size();
		for (uint32_t i = 0; i < size; ++i) {
			fvector2 vt = vb->tc(i);
			if (max)
				vt.y = 1.0f - vt.y;
			w->w_sf("vt %f %f\n", vt.x, vt.y);
		}
	}

		/* faces */
	msg("...faces");
	for (CMesh_vec_it it = m_model_meshes.begin(), end = m_model_meshes.end(); it != end; ++it) {	
		const xr_ibuf *ib = &((*it)->ib());
		uint32_t size = ib->size();
		w->w_sf("\ng %s\nusemtl %s\n", (*it)->texture().data(), (*it)->texture().data());
		w->w_sf("\n# %d faces\n", size / 3);
		for (uint_fast32_t i = 0; i < size; i += 3)
			w->w_sf("f %d/%d/%d %d/%d/%d %d/%d/%d\n", (*ib)[i], (*ib)[i], (*ib)[i], (*ib)[i + 1], (*ib)[i + 1], (*ib)[i + 1], (*ib)[i + 2], (*ib)[i + 2], (*ib)[i + 2]);
	}
	fs.w_close(w);
	return true;
}
void yae_model::clear() 
{
	xr_object::clear();
	m_id.clear(); 
	m_name.clear(); 
	m_version.clear(); 
	delete_elements(m_model_meshes);
	delete_elements(m_bodies);
	delete_elements(m_anims);
}
yae_model::~yae_model() 
{
	delete_elements(m_model_meshes);
	delete_elements(m_bodies);
//	delete_elements(m_bones);
	delete_elements(m_anims);
//	delete_elements(m_parts);
}