# FBX Exporter

## Input args:
1. Path to FBX file to import
2. (Optional) Outputname
	- The output file will have the extension ".wobj"
	- That's the price of using this tool (you must adopt the wobject file lol)

## Format
I made the file format simple with the excessive headers so that the importer couldbe very simple. There lies the benefit of the Wobject (.wobj) file. The output will be a binary file with the following header:
```C++
// Mesh header struct
struct MeshHeader
{
	int indexcount, vertexcount;
	int indexstart, vertexstart;
	char texturename[256];
};
```
and the vertices use the following struct:
```C++
struct SimpleVertex
{
	FLOAT3 Pos;
	FLOAT3 Normal;
	FLOAT2 Tex;
};
```

## Example Importer Implementation
```C++
struct SimpleVertex
{
	FLOAT3 Pos;
	FLOAT3 Normal;
	FLOAT2 Tex;
};

struct SimpleMesh
{
	std::vector<SimpleVertex> vertexList;
	std::vector<int> indicesList;
};

// Mesh header struct
struct MeshHeader
{
	int indexcount, vertexcount;
	int indexstart, vertexstart;
	char texturename[256];
};

void LoadMeshAndTexture(std::string meshname)
{
	// Initialize the mesh struct & mesh header
	MeshHeader meshHeader;
	SimpleMesh simpleMesh;

	std::ifstream file(meshname.c_str(), std::ios_base::binary | std::ios_base::in);

	assert(file.is_open());

	// Load header
	file.read((char*)&meshHeader, sizeof(MeshHeader));

	// Load the indices and vertices
	// Resize the vectors to hold the data
	simpleMesh.indicesList.resize(meshHeader.indexcount);
	simpleMesh.vertexList.resize(meshHeader.vertexcount);

	file.read((char*)simpleMesh.indicesList.data(), sizeof(int) * meshHeader.indexcount);
	file.read((char*)simpleMesh.vertexList.data(), sizeof(SimpleVertex) * meshHeader.vertexcount);

	file.close();
}
```