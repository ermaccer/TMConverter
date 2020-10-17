#pragma once


struct tm_header {
	char markerType[6] = {};
};
#pragma pack (push,1)
struct tm_model_marker {
	char pad[261] = {};
	int  meshes;

};
#pragma (pop)

#pragma pack (push,1)
struct tm_mesh_entry {
	char modelName[554] = {}; // what are these sizes even
	char textureName[255] = {};
	//int     unkAmount; 
	//float unk[16 * unkAmount];
	//int     faceAmount;
	//int   faces[3 * faceAmount/3];
//	int     trisAmount;

};
#pragma (pop)