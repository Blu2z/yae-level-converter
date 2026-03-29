#include "yae_level.h"
#include "yae_model.h"
#include "ds2_reader.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <map>

bool compare_split(CLevelMesh *first, CLevelMesh *last) {
	if (first->material() < last->material())
		return true;
	if (first->material() == last->material())
		return first->texture() < last->texture();
	return false;
};
bool compare(CLevelMesh *first, CLevelMesh *last) {return first->texture() < last->texture();};
/* yae_level implementation */ 
void yae_level::read(xr_reader& r)
{
	read_header(r);
	if (m_header.id != "DS2GeometryFile_1") {
		msg("not level geometry file\n");
		abort();
	}

	while (!r.eof()) {
		std::string chunk_start, chunk_end, chunk_id;
		ds2_r_s(r, chunk_start);
		ds2_r_s(r, chunk_id);
		if (chunk_id == "meshes")
			read_meshes(r);
		else if (chunk_id == "buffers")
			read_buffers(r);
		else if (chunk_id == "lightmaps")
			read_lightmaps(r);
		else if (chunk_id == "vistree")
			read_vistree(r);
		else if (chunk_id == "lights")
			read_lights(r);
		else if (chunk_id == "models")
			read_models(r);
		else
			xr_not_implemented();
		ds2_r_s(r, chunk_end);
	}
	xr_assert(r.eof());
}
void yae_level::read_hash(xr_reader& r)
{
	uint32_t size = r.r_u32();
	for (uint32_t i = 0; i < size; ++i) {
		std::string name, path;
		r.r_sz(name);
		r.r_sz(path);
		m_map.insert(std::pair<std::string, std::string>(name, path));
	}
};
void yae_level::read_header(xr_reader& r)
{
	ds2_r_s(r, m_header.id);
	ds2_r_s(r, m_header.version);
};
void yae_level::read_meshes(xr_reader& r)
{
	uint32_t mesh_count = r.r_u32();
	m_meshes.reserve(mesh_count);
	for (uint32_t i = 0; i < mesh_count; ++i) {
		CLevelMesh *mesh = new CLevelMesh;
		mesh->load(r);
		m_meshes.push_back(mesh);
	}
};
void yae_level::read_buffers(xr_reader& r)
{
	uint32_t num_buffers = r.r_u32();
	m_buffers.reserve(num_buffers);
	for (uint32_t i = 0; i < num_buffers; ++i) {
		YAE_BUFFER *buf = new YAE_BUFFER;
		buf->load(r);
		m_buffers.push_back(buf);
	}
};
void yae_level::read_lightmaps(xr_reader& r)
{
	uint32_t lm_count = r.r_u32();
	m_lightmaps.reserve(lm_count);
	for (uint32_t i = 0; i < lm_count; ++i) {
		std::string lm;
		ds2_r_s(r, lm);
		m_lightmaps.push_back(lm);
	}
};
void yae_level::read_vistree(xr_reader& r) 
{
	uint32_t node_count = r.r_u32();
	m_vistree.reserve(node_count);
	for (uint32_t i = 0; i < node_count; ++i) {
		VisTreeNode *node = new VisTreeNode;
		node->load(r);
		m_vistree.push_back(node);
	}
};
void yae_level::read_lights(xr_reader& r) 
{
	uint32_t lights_count = r.r_u32();
	m_lights.reserve(lights_count);
	for (uint32_t i = 0; i < lights_count; ++i) {
		YAEStaticLight *light = new YAEStaticLight;
		light->load(r);
		m_lights.push_back(light);
	}
};
void yae_level::read_models(xr_reader& r) 
{
	uint32_t models_count = r.r_u32();
	m_models.reserve(models_count);
	for (uint32_t i = 0; i < models_count; ++i) {
		YAEStaticModel *model = new YAEStaticModel;
		model->load(r);
		m_models.push_back(model);
	}
};
void yae_level::load_ext_models()
{
	if (m_models.empty() || m_map.empty())
		return;

	xr_file_system& fs = xr_file_system::instance();

	for (YAEStaticModel_vec_it it = m_models.begin(); it != m_models.end(); ++it) {
		YAEStaticModel* sm = *it;
		if (m_loaded_models.count(sm->m_name))
			continue;

		std::map<std::string, std::string>::iterator path_it = m_map.find(sm->m_name);
		if (path_it == m_map.end()) {
			msg("model '%s' not found in hash", sm->m_name.c_str());
			continue;
		}

		xr_reader* r = fs.r_open(path_it->second);
		if (!r) {
			msg("can't open model '%s'", sm->m_name.c_str());
			continue;
		}

		yae_model* model = new yae_model;
		if (model->read(*r, false, false) != MODEL_READ_OK) {
			msg("can't read model '%s'", sm->m_name.c_str());
			delete model;
			fs.r_close(r);
			continue;
		}
		fs.r_close(r);
		m_loaded_models[sm->m_name] = model;
		msg("loaded model '%s' (%zu meshes, %zu instances)",
			sm->m_name.c_str(), model->model_meshes().size(), sm->m_descs.size());
	}
}
void yae_level::save(const char* outpath, bool split, bool max, float scale) 
{
	/* sort meshes by texture */
	if (split)
		std::sort(m_meshes.begin(), m_meshes.end(), compare_split);
	else
		std::sort(m_meshes.begin(), m_meshes.end(), compare);

	/* open file for writing */
	xr_file_system& fs = xr_file_system::instance();
	std::string obj_name = outpath;

	/* prepare constants */
	msg("prepare constants");
	std::vector<std::string> materials;
	materials.reserve(m_meshes.size());
	uint32_t* mins = new uint32_t[m_meshes.size()];
	uint32_t* maxs = new uint32_t[m_meshes.size()];
	memset(mins, 0xFF, m_meshes.size() * sizeof(uint32_t));
	memset(maxs, 0, m_meshes.size() * sizeof(uint32_t));
	uint32_t vertices_count = 0;
	uint32_t normals_count = 0;
	uint32_t faces_count = 0;
	uint32_t mesh_id = 0;
	CObj_vec objects;
	objects.reserve(m_meshes.size());
	std::string current_material;
	if (!split) {
		CObj *obj = new CObj;
		obj->m_material.clear();
		obj->set_name(outpath);
		objects.push_back(obj);
	}
	for (CLevelMesh_vec_it it = m_meshes.begin(), end = m_meshes.end(); it != end; ++it) {
		CLevelMesh *m = *it;
		if (m->vb_id() == 0xFFFFFFFF)
			abort();
		
		/* min and max index in each mesh */
		YAE_BUFFER* ibuf = m_buffers[m->ib_id()];
		for (uint_fast32_t i = 0; i < m->ind_count(); ++i) {
			uint32_t idx = (*ibuf)[i + m->ind_offset()];
			if (mins[mesh_id] > idx)
				mins[mesh_id] = idx;
			if (maxs[mesh_id] < idx)
				maxs[mesh_id] = idx;
		}

		/* overall statistics */
		vertices_count += maxs[mesh_id] - mins[mesh_id] + 1;
		if (m_buffers[m->vb_id()]->has_normals())
			normals_count += maxs[mesh_id] - mins[mesh_id] + 1;
		faces_count += m->face_count();

		/* materials lib */
		if (std::find(materials.begin(), materials.end(), m->texture()) == materials.end())
			materials.push_back(m->texture());

		if (split && (current_material.empty() || (current_material != m->material()))) {
			CObj *obj = new CObj;
			obj->m_material = m->material();
			obj->set_name(outpath);
			objects.push_back(obj);
			current_material = m->material();
		}

		(*(objects.end() - 1))->m_meshes.push_back(mesh_id);

		++mesh_id;
	}

	/* add model instance textures to materials */
	for (YAEStaticModel_vec_it it = m_models.begin(); it != m_models.end(); ++it) {
		std::map<std::string, yae_model*>::iterator mit = m_loaded_models.find((*it)->m_name);
		if (mit == m_loaded_models.end()) continue;
		for (CMesh_vec_cit mi = mit->second->model_meshes().begin(); mi != mit->second->model_meshes().end(); ++mi) {
			if (std::find(materials.begin(), materials.end(), (*mi)->texture()) == materials.end())
				materials.push_back((*mi)->texture());
		}
	}

	/* write mtl file */
	msg("writing materials");
	std::string mtl_name = outpath;
	mtl_name += ".mtl";
	xr_writer *w_mtl = fs.w_open(mtl_name.data());
	std::string textures_path = fs.resolve_path(PA_GAME_TEXTURES);
	for (std::vector<std::string>::iterator it = materials.begin(), end = materials.end(); it != end; ++it) {
		char *tex_name = new char[0x100];
		snprintf(tex_name, 0x100, "%s$dds\\%s", textures_path.data(), it->data());
		w_mtl->w_sf("\nnewmtl \"%s\"\nmap_Kd \"%s.dds\"\n", it->data(), tex_name);
		delete[] tex_name;
	}
	fs.w_close(w_mtl);


	/* get mtl-file name */
	size_t pos = mtl_name.find_last_of("/\\", std::string::npos);
	if (pos != std::string::npos)
		mtl_name.erase(mtl_name.begin(), mtl_name.begin() + pos + 1);

	/* overal statistics */
	msg("writing statistics");
	std::string stat_name = outpath;
	stat_name += "_stat.txt";
	xr_writer *w_stat = fs.w_open(stat_name.data());
	w_stat->w_sf("vertices: %d\r\nnormals: %d\r\nfaces: %d\r\nmeshes: %d\r\ntextures: %d\r\n", vertices_count, normals_count, faces_count, m_meshes.size(), materials.size()); /* \r because of text file*/
	fs.w_close(w_stat);
	stat_name.clear();

	/* writing obj */

	for (CObj_vec_it it = objects.begin(), end = objects.end(); it != end; ++it) {
		CObj *obj = *it;
		msg("writing %s", obj->m_name.data());
		msg("...vertices");
		xr_writer *w = fs.w_open(obj->m_name.data());
		w->w_sf("\n# This file is generated by YAE level converter\n");
		w->w_sf("\nmtllib %s\n", mtl_name.data());

		/* obj statistics */
		vertices_count = 0;
		normals_count = 0;
		faces_count = 0;
		for (uint16_t i = 0; i < obj->m_meshes.size(); ++i) {
			CLevelMesh *mesh = m_meshes[obj->m_meshes[i]];
			vertices_count += maxs[obj->m_meshes[i]] - mins[obj->m_meshes[i]] + 1;
			if (m_buffers[mesh->vb_id()]->has_normals())
				normals_count += maxs[obj->m_meshes[i]] - mins[obj->m_meshes[i]] + 1;
			faces_count += mesh->face_count();
		}

		uint32_t* vertex_offsets = new uint32_t[obj->m_meshes.size() + 1];
		uint32_t* normal_offsets = new uint32_t[obj->m_meshes.size() + 1];

		vertex_offsets[0] = 1;
		normal_offsets[0] = 1;

		w->w_sf("\n#vertices %d\n", vertices_count);
		for (uint16_t i = 0; i < obj->m_meshes.size(); ++i) {
			uint32_t mesh_id = obj->m_meshes[i];
			CLevelMesh *mesh = m_meshes[mesh_id];
			YAE_BUFFER *buf = m_buffers[mesh->vb_id()];
			uint32_t off = mesh->vert_offset();
			vertex_offsets[i + 1] = vertex_offsets[i] + maxs[mesh_id] - mins[mesh_id] + 1;
			for (uint32_t j = mins[mesh_id]; j <= maxs[mesh_id]; ++j) {					
				fvector3 v = buf->p(j + off);
				if (mesh->dynamic()) {
					const fmatrix& xf = *mesh->xform();
					fvector3 t;
					t.x = v.x * xf._11 + v.y * xf._12 + v.z * xf._13 + xf._14;
					t.y = v.x * xf._21 + v.y * xf._22 + v.z * xf._23 + xf._24;
					t.z = v.x * xf._31 + v.y * xf._32 + v.z * xf._33 + xf._34;
					v = t;
				}
				w->w_sf("v %f %f %f\n", v.x * scale, v.y * scale, v.z * scale);
			}
		}

		msg("...normals");
		w->w_sf("\n#normals %d\n", normals_count);
		for (uint16_t i = 0; i < obj->m_meshes.size(); ++i) {
			uint32_t mesh_id = obj->m_meshes[i];
			CLevelMesh *mesh = m_meshes[mesh_id];
			YAE_BUFFER *buf = m_buffers[mesh->vb_id()];
			if (buf->has_normals()) {
				uint32_t off = mesh->vert_offset();
				normal_offsets[i + 1] = normal_offsets[i] + maxs[mesh_id] - mins[mesh_id] + 1;
				for (uint32_t j = mins[mesh_id]; j <= maxs[mesh_id]; ++j) {
					fvector3 norm = buf->n(j + off);
					if (mesh->dynamic()) {
						const fmatrix& xf = *mesh->xform();
						fvector3 tn;
						tn.x = norm.x * xf._11 + norm.y * xf._12 + norm.z * xf._13;
						tn.y = norm.x * xf._21 + norm.y * xf._22 + norm.z * xf._23;
						tn.z = norm.x * xf._31 + norm.y * xf._32 + norm.z * xf._33;
						float len = std::sqrt(tn.x*tn.x + tn.y*tn.y + tn.z*tn.z);
						if (len > 0.f) { tn.x /= len; tn.y /= len; tn.z /= len; }
						norm = tn;
					}
					w->w_sf("vn %f %f %f\n", norm.x, norm.y, norm.z);
				}
			} else {
				normal_offsets[i + 1] = normal_offsets[i];
			}
		}

		msg("...texcoords");
		w->w_sf("\n#texcoords %d\n", vertices_count);
		for (uint16_t i = 0; i < obj->m_meshes.size(); ++i) {
			uint32_t mesh_id = obj->m_meshes[i];
			CLevelMesh *mesh = m_meshes[mesh_id];
			uint32_t off = mesh->vert_offset();
			YAE_BUFFER *buf = m_buffers[mesh->vb_id()];
			for (uint32_t j = mins[mesh_id]; j <= maxs[mesh_id]; ++j) {
				fvector2 tc = buf->tc(j + off);
				if (max)
					tc.y = 1.0 - tc.y;
				w->w_sf("vt %f %f\n", tc.x, tc.y);
			}
		}

		msg("...faces");
		w->w_sf("\n# %d faces\n", faces_count);
		current_material.clear();
		for (uint16_t i = 0; i < obj->m_meshes.size(); ++i) {
			CLevelMesh *m = m_meshes[obj->m_meshes[i]];
			if (current_material.empty() || (current_material != m->texture())) {
				current_material = m->texture();
				w->w_sf("\ng %s\nusemtl %s\n", current_material.data(), current_material.data());
			}
			YAE_BUFFER *vbuf = m_buffers[m->vb_id()];
			YAE_BUFFER *ibuf = m_buffers[m->ib_id()];
			uint32_t size = m->ind_count();
			int32_t v_offset = vertex_offsets[i] - mins[obj->m_meshes[i]];
			uint32_t i_offset = m->ind_offset();

			if (vbuf->has_normals()) {
				int32_t n_offset = normal_offsets[i] - mins[obj->m_meshes[i]];
				if (m->strip()) {
					size -= 2;
					for (uint32_t j = 0; j < size; ++j) {
						uint32_t v_idx1 = (*ibuf)[j + i_offset] + v_offset;
						uint32_t v_idx2 = (*ibuf)[j + i_offset + 1] + v_offset;
						uint32_t v_idx3 = (*ibuf)[j + i_offset + 2] + v_offset;
						uint32_t n_idx1 = (*ibuf)[j + i_offset] + n_offset;
						uint32_t n_idx2 = (*ibuf)[j + i_offset + 1] + n_offset;
						uint32_t n_idx3 = (*ibuf)[j + i_offset + 2] + n_offset;
						xr_assert(v_idx1 > 0 && v_idx1 <= vertices_count);xr_assert(v_idx2 > 0 && v_idx2 <= vertices_count);xr_assert(v_idx3 > 0 && v_idx3 <= vertices_count);
						xr_assert(n_idx1 > 0 && n_idx1 <= normals_count);xr_assert(n_idx2 > 0 && n_idx2 <= normals_count);xr_assert(n_idx3 > 0 && n_idx3 <= normals_count);
						if (v_idx1 == v_idx2 || v_idx1 == v_idx3 || v_idx2 == v_idx3)		// skip degenerated triangles
							continue;
						w->w_sf("f %d/%d/%d %d/%d/%d %d/%d/%d\n", v_idx1, v_idx1, n_idx1, v_idx2, v_idx2, n_idx2, v_idx3, v_idx3, n_idx3);
					}
				} else {
					for (uint32_t j = 0; j < size; j += 3) {
						uint32_t v_idx1 = (*ibuf)[j + i_offset] + v_offset;
						uint32_t v_idx2 = (*ibuf)[j + i_offset + 1] + v_offset;
						uint32_t v_idx3 = (*ibuf)[j + i_offset + 2] + v_offset;
						uint32_t n_idx1 = (*ibuf)[j + i_offset] + n_offset;
						uint32_t n_idx2 = (*ibuf)[j + i_offset + 1] + n_offset;
						uint32_t n_idx3 = (*ibuf)[j + i_offset + 2] + n_offset;
						xr_assert(v_idx1 > 0 && v_idx1 <= vertices_count);xr_assert(v_idx2 > 0 && v_idx2 <= vertices_count);xr_assert(v_idx3 > 0 && v_idx3 <= vertices_count);
						xr_assert(n_idx1 > 0 && n_idx1 <= normals_count);xr_assert(n_idx2 > 0 && n_idx2 <= normals_count);xr_assert(n_idx3 > 0 && n_idx3 <= normals_count);
						w->w_sf("f %d/%d/%d %d/%d/%d %d/%d/%d\n", v_idx1, v_idx1, n_idx1, v_idx2, v_idx2, n_idx2, v_idx3, v_idx3, n_idx3);
					}
				}
			} else {
				if (m->strip()) {
					size -= 2;
					for (uint32_t j = 0; j < size; ++j) {
						uint32_t v_idx1 = (*ibuf)[j + i_offset] + v_offset;
						uint32_t v_idx2 = (*ibuf)[j + i_offset + 1] + v_offset;
						uint32_t v_idx3 = (*ibuf)[j + i_offset + 2] + v_offset;
						xr_assert(v_idx1 > 0 && v_idx1 <= vertices_count);xr_assert(v_idx2 > 0 && v_idx2 <= vertices_count);xr_assert(v_idx3 > 0 && v_idx3 <= vertices_count);
						if (v_idx1 == v_idx2 || v_idx1 == v_idx3 || v_idx2 == v_idx3)		// skip degenerated triangles
							continue;
						w->w_sf("f %d/%d %d/%d %d/%d\n", v_idx1, v_idx1, v_idx2, v_idx2, v_idx3, v_idx3);
					}
				} else {
					for (uint32_t j = 0; j < size; j += 3) {
						uint32_t v_idx1 = (*ibuf)[j + i_offset] + v_offset;
						uint32_t v_idx2 = (*ibuf)[j + i_offset + 1] + v_offset;
						uint32_t v_idx3 = (*ibuf)[j + i_offset + 2] + v_offset;
						xr_assert(v_idx1 > 0 && v_idx1 <= vertices_count);xr_assert(v_idx2 > 0 && v_idx2 <= vertices_count);xr_assert(v_idx3 > 0 && v_idx3 <= vertices_count);
						w->w_sf("f %d/%d %d/%d %d/%d\n", v_idx1, v_idx1, v_idx2, v_idx2, v_idx3, v_idx3);
					}
				}
			}
		}
		/* write model instances */
		if (!m_loaded_models.empty()) {
			uint32_t mv_base = vertex_offsets[obj->m_meshes.size()];
			uint32_t mn_base = normal_offsets[obj->m_meshes.size()];

			msg("...model instances");
			for (YAEStaticModel_vec_cit si = m_models.begin(); si != m_models.end(); ++si) {
				YAEStaticModel* sm = *si;
				std::map<std::string, yae_model*>::iterator mit = m_loaded_models.find(sm->m_name);
				if (mit == m_loaded_models.end()) continue;
				yae_model* loaded = mit->second;
				const CMesh_vec& mmeshes = loaded->model_meshes();

				for (size_t di = 0; di < sm->m_descs.size(); ++di) {
					const fmatrix& xf = sm->m_descs[di].m_xform;

					for (CMesh_vec_cit mi = mmeshes.begin(); mi != mmeshes.end(); ++mi) {
						CMesh* m = *mi;
						const xr_vbuf& vb = m->vb();
						const xr_ibuf& ib = m->ib();
						uint32_t nv = (uint32_t)vb.size();

						w->w_sf("\ng %s_inst%zu\nusemtl %s\n",
							sm->m_name.c_str(), di, m->texture().c_str());

						/* vertices (transformed by instance xform) */
						for (uint32_t vi = 0; vi < nv; ++vi) {
							const fvector3& v = vb.p(vi);
							float tx = v.x * xf._11 + v.y * xf._12 + v.z * xf._13 + xf._14;
							float ty = v.x * xf._21 + v.y * xf._22 + v.z * xf._23 + xf._24;
							float tz = v.x * xf._31 + v.y * xf._32 + v.z * xf._33 + xf._34;
							w->w_sf("v %f %f %f\n", tx * scale, ty * scale, tz * scale);
						}

						/* normals (rotation only, no translation) */
						for (uint32_t vi = 0; vi < nv; ++vi) {
							const fvector3& n = vb.n(vi);
							float nx = n.x * xf._11 + n.y * xf._12 + n.z * xf._13;
							float ny = n.x * xf._21 + n.y * xf._22 + n.z * xf._23;
							float nz = n.x * xf._31 + n.y * xf._32 + n.z * xf._33;
							float len = std::sqrt(nx*nx + ny*ny + nz*nz);
							if (len > 0.f) { nx /= len; ny /= len; nz /= len; }
							w->w_sf("vn %f %f %f\n", nx, ny, nz);
						}

						/* texcoords */
						for (uint32_t vi = 0; vi < nv; ++vi) {
							fvector2 tc = vb.tc(vi);
							if (max) tc.y = 1.0f - tc.y;
							w->w_sf("vt %f %f\n", tc.x, tc.y);
						}

						/* faces */
						for (size_t fi = 0; fi < ib.size(); fi += 3) {
							uint32_t i1 = ib[fi] + mv_base;
							uint32_t i2 = ib[fi + 1] + mv_base;
							uint32_t i3 = ib[fi + 2] + mv_base;
							uint32_t n1 = ib[fi] + mn_base;
							uint32_t n2 = ib[fi + 1] + mn_base;
							uint32_t n3 = ib[fi + 2] + mn_base;
							w->w_sf("f %d/%d/%d %d/%d/%d %d/%d/%d\n",
								i1, i1, n1, i2, i2, n2, i3, i3, n3);
						}

						mv_base += nv;
						mn_base += nv;
					}
				}
			}
		}
		delete[] vertex_offsets;
		delete[] normal_offsets;
		fs.w_close(w);
	}
	current_material.clear();
	mtl_name.clear();
	delete_elements(objects);
}
yae_level::~yae_level()
{
	delete_elements(m_meshes);
	delete_elements(m_buffers);
	delete_elements(m_models);
	delete_elements(m_vistree);
	delete_elements(m_lights);
	for (std::map<std::string, yae_model*>::iterator it = m_loaded_models.begin();
			it != m_loaded_models.end(); ++it)
		delete it->second;
	m_loaded_models.clear();
	m_lightmaps.clear();
}
/* YAE_BUFFER implementation */ 
void YAE_BUFFER::load(xr_reader& r)
{
	m_signature = r.r_u32();
	uint32_t size = r.r_u32();
	set_size(size);
	if (!has_indices()) {
		if (has_points()) {
			m_points = new fvector3[size];
			r.r_cseq(size, m_points);
		}
		if (has_texcoords()) {
			m_texcoords = new fvector2[size];
			r.r_cseq(size, m_texcoords);
		}
		if (has_lightmaps()) {
			m_lightmaps = new fvector2[size];
			r.r_cseq(size, m_lightmaps);
		}
		if (has_colors()) {
			m_raw_colors = new uint32_t[size * 4];
			r.r_cseq(size * 4, m_raw_colors);
		}
		if (has_normals()) {
			m_normals = new fvector3[size];
			r.r_cseq(size, m_normals);
		}
		if (has_tangents()) {
			m_tangents = new fvector3[size];
			r.r_cseq(size, m_tangents);
		}
		if (has_binormals()) {
			m_binormals = new fvector3[size];
			r.r_cseq(size, m_binormals);
		}
		m_indices = NULL;
	} else {
		m_indices = new uint32_t[size];
		r.r_cseq(size, m_indices);
	}
}
YAE_BUFFER::YAE_BUFFER(): m_raw_colors(0), m_indices(0), m_tangents(0), m_binormals(0) {}
YAE_BUFFER::~YAE_BUFFER()
{
	if (m_raw_colors)
		delete[] m_raw_colors;
	if (m_indices)
		delete[] m_indices;
	if (m_tangents)
		delete[] m_tangents;
	if (m_binormals)
		delete[] m_binormals;
};
/* various implementation */ 
void VisTreeNode::load(xr_reader& r)
{
	r.r(unk_fvector3_1);
	r.r(unk_fvector3_2);
	unk_float_1 = r.r_float();
	r.r(unk_fvector3_3);
	bool dynamic = r.r_bool();
	if (dynamic) {
		uint32_t size = r.r_u32();
		unk_vector.reserve(size);
		for (uint_fast32_t i = 0; i < size; ++i)
			unk_vector.push_back(r.r_u32());
	} else {
		unk_vector.reserve(8);
		for (uint_fast32_t i = 0; i < 8; ++i)
			unk_vector.push_back(r.r_u32());
	}
}
void YAEStaticLight::load(xr_reader& r)
{
	type = r.r_u32();
	r.r(unk_fvector3_1);
	r.r(unk_fvector3_2);
	r.r(unk_fvector3_3);
	unk_params.reserve(8);
	for (uint_fast32_t i = 0; i < 8; ++i)
		unk_params.push_back(r.r_u32());
}
void YAEStaticModel::load(xr_reader& r)
{
	ds2_r_s(r, m_name);
	r.r(m_bbox);
	uint32_t num = r.r_u32();
	m_descs.resize(num);
	for (uint_fast32_t i = 0; i < num; ++i)
		r.r<model_desc>(m_descs[i]);
}