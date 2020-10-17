// tmconverter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include "eObj.h"
#include "tm.h"

enum eModes {
	MODE_EXPORT = 1,
	MODE_IMPORT

};

int main(int argc, char* argv[])
{
	if (argc == 1) {
		std::cout << "TMConverter - work with TM model format by ermaccer\n"
			<< "Usage: tmconverter <params> <file>\n"
			<< "    -e              Exports .tm to .obj\n"
			<< "    -i              Converts folder with .obj to .tm\n"
			<< "    -l              Specifies list file\n"
			<< "    -o              Specifies a folder (if meshes > 1) for extraction/output file\n" ;
		return 1;
	}

	int mode = 0;
	std::string o_param;
	std::string l_param;
	// params
	for (int i = 1; i < argc - 1; i++)
	{
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
			return 1;
		}
		switch (argv[i][1])
		{
		case 'e': mode = MODE_EXPORT;
			break;
		case 'i': mode = MODE_IMPORT;
			break;
		case 'o':
			i++;
			o_param = argv[i];
			break;
		case 'l':
			i++;
			l_param = argv[i];
			break;
		default:
			std::cout << "ERROR: Param does not exist: " << argv[i] << std::endl;
			break;
		}
	}


	if (mode == MODE_EXPORT)
	{
		std::ifstream pFile(argv[argc - 1], std::ifstream::binary);

		if (!pFile)
		{
			std::cout << "ERROR: Could not open: " << argv[argc - 1] << "!" << std::endl;
			return 1;
		}

		tm_header header;

		pFile.read((char*)&header, sizeof(tm_header));

		if (!(strcmp(header.markerType, "model") == 0))
		{
			std::cout << "ERROR: Unsupported marker type! Only model is supported." << std::endl;
		}

		tm_model_marker marker;
		pFile.read((char*)&marker, sizeof(tm_model_marker));

		

		if (!o_param.empty())
		{
			if (!std::filesystem::exists(o_param))
				std::filesystem::create_directory(o_param);
			std::filesystem::current_path(o_param);
		}


		std::string list = argv[argc - 1];
		list += "_list.txt";
		list.insert(0, "!");
		std::ofstream pList(list, std::ofstream::binary);


		for (int i = 0; i < marker.meshes; i++)
		{
			tm_mesh_entry mesh;
			pFile.read((char*)&mesh, sizeof(tm_mesh_entry));

			int unkAmount = 0;
			pFile.read((char*)&unkAmount, sizeof(int));
			for (int i = 0; i < unkAmount; i++)
			{

				float unk[16];
				pFile.read((char*)&unk, sizeof(unk));

			}

			int faceAmount = 0; 
			pFile.read((char*)&faceAmount, sizeof(int));

			std::vector<obj_face> vFaces;

			for (int i = 0; i < faceAmount / 3; i++)
			{
				obj_face face;
				pFile.read((char*)&face, sizeof(obj_face));
				vFaces.push_back(face);
			}

			int vertsAmount = 0;
			pFile.read((char*)&vertsAmount, sizeof(int));
			std::vector<obj_v> vVerts;
			std::vector<obj_uv> vMaps;
			std::vector<obj_vn> vNorm;

			for (int i = 0; i < vertsAmount; i++)
			{
				obj_v v;
				pFile.read((char*)&v, sizeof(obj_v));
				vVerts.push_back(v);
			}

			for (int i = 0; i < vertsAmount; i++)
			{

				obj_vn vn;
				pFile.read((char*)&vn, sizeof(obj_vn));
				vn.norm[0] = 0.0f;
				vNorm.push_back(vn);
			}


			for (int i = 0; i < vertsAmount; i++)
			{
				obj_uv uv;
				pFile.read((char*)&uv, sizeof(obj_uv));
				vMaps.push_back(uv);
			}


			eObj obj;
			obj.SetModelData(vVerts, vMaps, vNorm, vFaces);
			obj.SaveFile(mesh.modelName, mesh.modelName, mesh.textureName);

			std::string name_obj = mesh.modelName;
			name_obj += ".obj";

			pList << name_obj.c_str() << std::endl;
			
		}	



	}
	if (mode == MODE_IMPORT)
	{
		if (!std::filesystem::exists(argv[argc - 1]))
		{
			std::cout << "ERROR: Folder does not exist: " << argv[argc - 1] << "!" << std::endl;
			return 1;
		}
		else
		{
			std::vector<eObj> vModels;

			if (!l_param.empty())
			{

				std::ifstream pList(l_param, std::ifstream::binary);

				std::string line;
				std::string path;
				std::vector<std::string> fileNames;
				std::filesystem::current_path(argv[argc - 1]);
				while (std::getline(pList, line))
				{
					std::stringstream ss(line);
					ss >> path;
					eObj model;
					model.LoadFile(path.c_str());	
					vModels.push_back(model);
				}
				std::filesystem::current_path("..");
			}
			else
			{
				for (const auto & file : std::filesystem::recursive_directory_iterator(argv[argc - 1]))
				{
					if (file.path().has_extension())
					{
						if (file.path().extension().string() == ".obj")
						{
							eObj model;
							model.SetFolder(argv[argc - 1]);
							model.LoadFile(file.path().string().c_str());
							std::filesystem::current_path("..");
							vModels.push_back(model);
						}
					}
				}
			}


			std::string output;
			if (o_param.empty())
				output = "new.tm";
			else
				output = o_param;

			std::ofstream oFile(output, std::ofstream::binary);

			tm_header header;
			sprintf(header.markerType, "model");

			oFile.write((char*)&header, sizeof(tm_header));

			tm_model_marker marker;
			marker.meshes = vModels.size();
			oFile.write((char*)&marker, sizeof(tm_model_marker));

			for (int i = 0; i < vModels.size(); i++)
			{
				std::cout << "INFO: Processing model " << i + 1 << "/" << vModels.size() << std::endl; 
				tm_mesh_entry mesh;
				sprintf(mesh.modelName, "%s", vModels[i].GetMeshName().c_str());
				sprintf(mesh.textureName, "%s", vModels[i].GetTextureName().c_str());

				oFile.write((char*)&mesh, sizeof(tm_mesh_entry));

				int unkAmount = 0;
				oFile.write((char*)&unkAmount, sizeof(int));


				int faceAmount = vModels[i].GetFaces().size() * 3;
				oFile.write((char*)&faceAmount, sizeof(int));

				for (int a = 0; a < vModels[i].GetFaces().size(); a++)
				{
					int f[3];
					f[0] = vModels[i].GetFaces()[a].face[0];
					f[1] = vModels[i].GetFaces()[a].face[1];
					f[2] = vModels[i].GetFaces()[a].face[2];
					oFile.write((char*)&f, sizeof(f));
				}

				int vertexAmount = vModels[i].GetVertexes().v.size();
				oFile.write((char*)&vertexAmount, sizeof(int));

				for (int a = 0; a < vModels[i].GetVertexes().v.size(); a++)
				{
					oFile.write((char*)&vModels[i].GetVertexes().v[a], sizeof(obj_v));
				}
				for (int a = 0; a < vModels[i].GetVertexes().v.size(); a++)
				{
					oFile.write((char*)&vModels[i].GetVertexes().vn[a], sizeof(obj_vn));
				}
				for (int a = 0; a < vModels[i].GetVertexes().v.size(); a++)
				{
					obj_uv newUV = vModels[i].GetVertexes().uv[a];
					newUV.v = 1.0f - newUV.v;
					oFile.write((char*)&newUV, sizeof(obj_uv));
				}

				//vModels[i].Print();
			}

			std::cout << "Saved as " << output.c_str() << std::endl;
		}
	}

	return 0;
}
