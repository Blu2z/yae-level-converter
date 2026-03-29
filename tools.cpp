#include "tools.h"
#include <cstring>
#include <filesystem>

// Helper: open first file with given extension in a resolved path
static xr_reader* r_open_by_ext(xr_file_system& fs, const char* path, const char* ext)
{
	std::string dir = fs.resolve_path(path);
	std::string dot_ext = std::string(".") + ext;
	for (auto& entry : std::filesystem::directory_iterator(dir)) {
		if (entry.is_regular_file() && entry.path().extension() == dot_ext)
			return fs.r_open(entry.path().string());
	}
	return nullptr;
}

// Helper: collect files with given extension recursively
static void get_file_list(std::vector<std::string>& out, const char* dir, const char* ext)
{
	for (auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
		if (entry.is_regular_file() && entry.path().extension() == ext)
			out.push_back(entry.path().string());
	}
}

bool model_tools::process(const cl_parser& cl) 
{
	
	xr_file_system& fs = xr_file_system::instance();

	const char *src, *mode, *outpath;

	cl.get_string("-model", src);
	if (!cl.get_string("-mode", mode))
		mode = "max";

	unsigned format = 0;
	if (cl.exist("-skl"))
		format |= TARGET_SKL;
	if (cl.exist("-skls"))
		format |= TARGET_SKLS;
	if (cl.exist("-bones"))
		format |= TARGET_BONES;
	if (cl.exist("-obj"))
		format |= TARGET_OBJ;
	if (cl.exist("-object"))
		format |= TARGET_OBJECT;

	msg("reading %s", src);

	/* open file */
	xr_reader *r = fs.r_open(PA_GAME_MESHES, src);
	xr_assert(r);
	/* read file*/
	yae_model *model = new yae_model;
	if (model->read(*r, false, cl.exist("-object")) != MODEL_READ_OK)
		return false;
	fs.r_close(r);

	switch(format) {
	case TARGET_SKL:
		if (!cl.get_string("-skl", outpath))
			outpath = src;
		save_skl(*model, outpath, cl);
		break;
	case TARGET_SKLS:
		if (!cl.get_string("-skls", outpath))
			outpath = src;
		save_skls(*model, outpath);
		break;
	case TARGET_BONES:
		if (!cl.get_string("-bones", outpath))
			outpath = src;
		save_bones(*model, outpath);
		break;
	case TARGET_OBJ:
		if (!cl.get_string("-obj", outpath))
			outpath = src;
		save_obj(*model, outpath, !strcmp(mode, "max"));
		break;
	case TARGET_OBJECT:
		if (!cl.get_string("-object", outpath))
			outpath = src;
		save_object(*model, outpath);
		break;
	default:
		break;
	}
	delete model;
	return true;
}
bool level_tools::process(const cl_parser& cl) {
	
	xr_file_system& fs = xr_file_system::instance();

	const char *level_folder, *mode, *outpath;

	cl.get_string("-level", level_folder);
	if (!cl.get_string("-mode", mode))
		mode = "max";

	if (!fs.folder_exist(PA_GAME_MAPS, level_folder)) {
		msg("%s not found in maps folder\n", level_folder);
		return false;
	}
	fs.update_path(PA_CURRENT_MAP, PA_GAME_MAPS, level_folder);
	if (!cl.get_string("-out", outpath))
		outpath = level_folder;

	msg("reading %s", level_folder);

	/* open file */
	xr_reader *r = r_open_by_ext(fs, PA_CURRENT_MAP, "ds2");
	/* read file*/
	yae_level *level = new yae_level;
	level->read(*r);

	/* load external models for instancing */
	{
		xr_reader *models_hash = NULL;
		if (xr_file_system::file_exist("paths.bin"))
			models_hash = fs.r_open("paths.bin");
		if (!models_hash) {
			std::string meshes_dir = fs.resolve_path(PA_GAME_MESHES);
			if (!meshes_dir.empty()) {
				std::vector<std::string> paths;
				get_file_list(paths, meshes_dir.c_str(), ".ds2md");
				std::vector<std::string> names;
				names.reserve(paths.size());
				for (uint32_t i = 0; i < paths.size(); ++i) {
					msg("model %s", paths[i].data());
					xr_reader* r_model = fs.r_open(paths[i]);
					if (!r_model) continue;
					yae_model* model = new yae_model;
					if (model->read(*r_model, true, false) == MODEL_READ_OK) {
						if (!model->texture().empty()) {
							names.push_back(model->texture());
						} else {
							paths.erase(paths.begin() + i);
							--i;
						}
					} else {
						paths.erase(paths.begin() + i);
						--i;
					}
					fs.r_close(r_model);
					delete model;
				}
				if (!names.empty()) {
					xr_writer* w = fs.w_open("paths.bin");
					w->w_u32(names.size());
					for (uint32_t i = 0; i < paths.size(); ++i) {
						w->w_sz(names[i]);
						w->w_sz(paths[i]);
					}
					fs.w_close(w);
					models_hash = fs.r_open("paths.bin");
				}
			}
		}
		if (models_hash) {
			level->read_hash(*models_hash);
			fs.r_close(models_hash);
		}
		level->load_ext_models();
	}

	fs.r_close(r);
	/* save */
	const char* scale_str;
	float scale = 1.f;
	if (cl.get_string("-scale", scale_str))
		scale = (float)atof(scale_str);
	level->save(outpath, cl.exist("-split"), !strcmp(mode, "max"), scale);
	delete level;
	return true;
}