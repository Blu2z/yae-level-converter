/*
	YAE level converter.
	usage: yae_converter -level <level_folder> -out <output_path> [-mode <max|maya>] [-split]
			yae_converter -model <model_file> [-obj|object] <output_path> [-mode <max|maya>]

	Version history:

	v.0.6. (26 Mar 2013)
		[+]	added logging
		[i] fixed duplicate extensions while converting models into objs
		[i] fixed converting some models
		[i] fixed converting some Instincts' maps

	v.0.5. (19 Mar 2013)
		[+] added converting models to *.object with bones
		[i] fixed wrong indices assignment

	v.0.4. (09 Mar 2013)
		[+] added model geometry converting
		[+] added split mode for splitting level geometry into separate models by material
		[i] fixed ignoring some meshes while converting

	v.0.3. (02 Mar 2013)
		[i] fixed wrong converting of some meshes
		[i] fixed wrong mesh splitting
		[i] fixed deleting some vertices while importing obj in editor

	v.0.2. (01 Mar 2013)
		[+] added configuration file and option keys
		[i] fixed wrong path to mtl file when using outpath with folders
		[i] fixed wrong texturing in 3Ds Max

	v.0.1. (26 Feb 2013)
		[+] initial release


	-=TODO=-
	level_tools:
	- handle models chunk
	model_tools:
	- proper scaling of point coords
	- proper setup of bones
	- handle anims

*/

#include "yae_converter.h"

using namespace xray_re;

void usage()
{
	printf("YAE Converter v.0.6 (%s)\n", BUILD_DATE);
	printf("Vendor: K.D. with use of xray_re code\n");
	printf("Level tools:\n");
	printf("Usage: yae_converter -level <level_folder> -out <output_path> [-mode <max|maya>] [-split]\n");
	printf(" -level <level_folder>	level to convert \n");
	printf(" -split					splitting geometry into separate files by material \n");
	printf("Model tools:\n");
	printf("Usage: yae_converter -model <model_file> -obj|object <outpath>  [-mode <max|maya>]\n");
	printf(" -model <model_file>	model to convert \n");
	printf(" -obj <outpath>			convert to obj (geometry only)\n");
	printf(" -object <outpath>		convert to object (geometry and bones by now)\n");
	printf("Common options:\n");
	printf(" -out <output_path>		output\n");
	printf(" -mode <max|maya>		target 3D editor\n");
}
int main(int argc, char* argv[])
{
/* parse options */
	static const cl_parser::option_desc options[] = {
		// level tools
		{"-level",	cl_parser::OT_STRING},
		{"-split",	cl_parser::OT_BOOL},
		{"-ext_models",	cl_parser::OT_BOOL},
		// model tools
		{"-model",	cl_parser::OT_STRING},
//		{"-skl",	cl_parser::OT_STRING},
//		{"-skls",	cl_parser::OT_STRING},
//		{"-bones",	cl_parser::OT_STRING},
		{"-obj",	cl_parser::OT_STRING},
		{"-object",	cl_parser::OT_STRING},
		// common options
		{"-out",	cl_parser::OT_STRING},
		{"-mode",	cl_parser::OT_STRING},
	};

	cl_parser cl;
	if (!cl.parse(argc, argv, xr_dim(options), options)) {
		usage();
		return 1;
	}

	if (!cl.exist("-level") && !cl.exist("-model"))  {
		usage();
		return 1;
	}
	
	/* init file system */
	xr_file_system& fs = xr_file_system::instance();

	if (!fs.initialize(DEFAULT_FS_SPEC, 0)) {
		msg("can't initialize file system");
		return 1;
	}

	xr_log::instance().init("yae_converter", 0);

	/* check nessesary paths  */
	if (!fs.resolve_path(PA_GAME_TEXTURES)) {
		msg("$game_textures$ not found\n");
		return 1;
	}
	if (!fs.resolve_path(PA_GAME_MAPS)) {
		msg("$game_maps$ not found\n");
		return 1;
	}
	if (!fs.resolve_path(PA_GAME_MESHES)) {
		msg("$game_meshes$ not found\n");
		return 1;
	}
	
	/* init requested mode */
	unsigned format = tools_base::TOOLS_AUTO;
	if (cl.exist("-model"))
		format |= tools_base::TOOLS_MODEL;
	if (cl.exist("-level"))
		format |= tools_base::TOOLS_LEVEL;

	tools_base* tools = 0;
	switch (format) {
	case tools_base::TOOLS_MODEL:
		tools = new model_tools;
		break;
	case tools_base::TOOLS_LEVEL:
		tools = new level_tools;
		break;
	}
	if (tools == 0) {
		msg("locked");
		return 0;
	}

	/* process */
	if (!tools->process(cl)) {
		msg("can't process");
		return 1;
	}
	delete tools;
	msg("done!");
	return 0;
}

