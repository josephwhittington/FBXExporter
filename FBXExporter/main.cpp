#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// FBX includes
#include <fbxsdk.h>
#include <fstream>

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

struct FLOAT3
{
	float x, y, z;
};
struct FLOAT2
{
	float x, y;
};

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

// Forward declarations
void ProcessFbxMesh(FbxNode* Node, std::string& filename);
void GrabUvs(FbxMesh* mesh, std::vector<FLOAT2>& tempuv);
void Compactify();
void LoadAndWriteMesh(SimpleMesh& simplemesh, const char* inputname, const char* outputname);
void WriteMesh(std::string texturename, const char* outputname);
// Forward declarations

// Global simpleMesh
SimpleMesh simpleMesh;
MeshHeader header;

// Global strings
const char* g_asset_directory = ""; // Prev "Assets/"


int main(int argc, char** argv)
{
	/* CLI args
	 * 1. FBX filepath
	 * 2. Output Name (optional)
	*/

	// Declare the strings for the arguments
	std::string importname, outputname;

	// If the first argument doesn't exist, exit early
	if (argc == 1) {
		std::cout << "To use the Whittington FBX converter specify an FBX file to convert as a CLI argument";
		return EXIT_SUCCESS;
	}
	// You get to this point so the first argument exists, let's validate that the file is an FBX and it exists
	importname = argv[1];
	// Check that the file ends with FBX
	if (importname.find_last_of(".fbx") == importname.length() - 1)
	{
		// File at least ends with FBX, if we can open the MF then that's enough for me
		std::fstream f(importname);
		if (!f.is_open())
		{
			std::cout << "Error: Couldn't locate file: " << importname;
			return EXIT_SUCCESS;
		}
		// If the file could, open close the file so the program can continue
		f.close();
	}
	// If the second argument wasn't specified use the name of the file
	if (argc == 2)
	{
		outputname = importname.substr(0, importname.find(".fbx"));
		outputname = outputname.append(".wobj");
	}
	else
	{
		// If the file already has a .wobj extension in the filepath then I don't need to add the extension
		// Otherwise I do
		outputname = argv[2];
		if (outputname.find(".wobj") == std::string::npos)
			outputname.append(".wobj");
	}

	LoadAndWriteMesh(simpleMesh, importname.c_str(), outputname.c_str());

	return EXIT_SUCCESS;
}

void LoadAndWriteMesh(SimpleMesh& simplemesh, const char* inputname, const char* outputname)
{
	std::cout << inputname << "\n" << outputname << "\n";

	// FBX SHIT
	// Initialize the SDK manager. This object handles all our memory management.
	FbxManager* lSdkManager = FbxManager::Create();

	// Create the IO settings object.
	FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);

	// Create an importer using the SDK manager.
	FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "Bruh");

	// Use the first argument as the filename for the importer.
	if (!lImporter->Initialize(inputname, -1, lSdkManager->GetIOSettings())) {
		printf("Call to FbxImporter::Initialize() failed.\n");
		printf("Error returned: %s\n\n", lImporter->GetStatus().GetErrorString());
		exit(-1);
	}

	// Create a new scene so that it can be populated by the imported file.
	FbxScene* lScene = FbxScene::Create(lSdkManager, "myScene");

	// Import the contents of the file into the scene.
	lImporter->Import(lScene);

	// The file is imported; so get rid of the importer.
	lImporter->Destroy();

	// Process the scene and build DirectX Arrays
	std::string filename;
	ProcessFbxMesh(lScene->GetRootNode(), filename);


	// Fix filename
	std::string folder = g_asset_directory;
	int start = filename.find_last_of("\\") + 1;
	int end = filename.find_last_of(".");

	filename = filename.substr(start, end - start);
	filename = folder.append(filename).append((std::string)(".dds"));
	// Fix filename
	// FBX SHIT

	WriteMesh(filename, outputname);
}

void WriteMesh(std::string texturename, const char* outputname)
{
	std::ofstream file(outputname, std::ios::trunc | std::ios::binary | std::ios::out);

	// Fill out Header
	header = { 0 };
	header.indexcount = simpleMesh.indicesList.size();
	header.vertexcount = simpleMesh.vertexList.size();
	header.indexstart = sizeof(MeshHeader);
	header.vertexstart = header.indexstart + simpleMesh.indicesList.size() * 4;
	strcpy_s(header.texturename, texturename.c_str());

	// Write the header
	file.write((const char*)&header, sizeof(MeshHeader));
	// Write the Indices
	file.write((const char*)simpleMesh.indicesList.data(), simpleMesh.indicesList.size() * (size_t)sizeof(int));
	// Write the Vertices
	file.write((const char*)simpleMesh.vertexList.data(), simpleMesh.vertexList.size() * (size_t)sizeof(SimpleVertex));

	file.close();
}

void ProcessFbxMesh(FbxNode* Node, std::string& filename)
{
	//FBX Mesh stuff
	int childrenCount = Node->GetChildCount();

	std::cout << "\nName:" << Node->GetName();

	for (int i = 0; i < childrenCount; i++)
	{
		FbxNode* childNode = Node->GetChild(i);
		FbxMesh* mesh = childNode->GetMesh();
		if (mesh != NULL)
		{
			std::cout << "\nMesh:" << childNode->GetName();

			// Get index count from mesh
			int numVertices = mesh->GetControlPointsCount();
			std::cout << "\nVertex Count:" << numVertices;

			// Resize the vertex vector to size of this mesh
			simpleMesh.vertexList.resize(numVertices);

			// Texture shit
			int materialcount = childNode->GetMaterialCount();
			FbxSurfaceMaterial* material = childNode->GetMaterial(0);

			if (material != NULL)
			{
				FbxProperty diffuseTexture = material->FindProperty(FbxSurfaceMaterial::sDiffuse);

				int textureCount = diffuseTexture.GetSrcObjectCount<FbxTexture>();
				FbxFileTexture* texture = FbxCast<FbxFileTexture>(diffuseTexture.GetSrcObject<FbxFileTexture>(0));

				if (texture)
					filename = texture->GetRelativeFileName();
				else filename = "default_texture";
			}
			// Texture shit


			// UV SHIT
			std::vector<FLOAT2> tempuv;
			GrabUvs(mesh, tempuv);
			// UV SHIT

			//================= Process Vertices ===============
			for (int j = 0; j < numVertices; j++)
			{
				FbxVector4 vert = mesh->GetControlPointAt(j);
				
				simpleMesh.vertexList[j].Pos.x = (float)vert.mData[0];
				simpleMesh.vertexList[j].Pos.y = (float)vert.mData[1];
				simpleMesh.vertexList[j].Pos.z = (float)vert.mData[2];
			}

			int numIndices = mesh->GetPolygonVertexCount();
			std::cout << "\nIndice Count:" << numIndices;

			// No need to allocate int array, FBX does for us
			int* indices = mesh->GetPolygonVertices();

			// Fill indiceList
			simpleMesh.indicesList.resize(numIndices);
			memcpy(simpleMesh.indicesList.data(), indices, numIndices * sizeof(int));



			//================= DON'T KNOW WHAT THE FUCK THIS IS ===============
			// Get the Normals array from the mesh
			FbxArray<FbxVector4> normalsVec;
			mesh->GetPolygonVertexNormals(normalsVec);
			std::cout << "\nNormalVec Count:" << normalsVec.Size();

			// Declare a new array for the second vertex array
			// Note the size is numIndices not numVertices
			std::vector<SimpleVertex> vertexListExpanded;
			vertexListExpanded.resize(numIndices);

			// align (expand) vertex array and set the normals
			SimpleVertex simp;
			for (int j = 0; j < numIndices; j++)
			{
				// Insert position
				simp.Pos = simpleMesh.vertexList[simpleMesh.indicesList[j]].Pos;
				// Insert normal
				simp.Normal.x = static_cast<float>(normalsVec[j].mData[0]);
				simp.Normal.y = static_cast<float>(normalsVec[j].mData[1]);
				simp.Normal.z = static_cast<float>(normalsVec[j].mData[2]);

				// UV SHIT
				simp.Tex.x = tempuv[j].x;
				simp.Tex.y = tempuv[j].y;
				// UV SHIT

				vertexListExpanded[j] = simp;
			}

			// make new indices to match the new vertex(2) array
			std::vector<int> indicesList;
			indicesList.resize(numIndices);

			for (int j = 0; j < numIndices; j++)
			{
				indicesList[j] = j;
			}

			{
				// copy working data to the global SimpleMesh
				simpleMesh.indicesList.assign(indicesList.begin(), indicesList.end());
				simpleMesh.vertexList.assign(vertexListExpanded.begin(), vertexListExpanded.end());

				Compactify();

				// print out some stats
				std::cout << "\nindex count BEFORE/AFTER compaction " << numIndices;
				std::cout << "\nvertex count ORIGINAL (FBX source): " << numVertices;
				std::cout << "\nvertex count AFTER expansion: " << numIndices;
				std::cout << "\nvertex count AFTER compaction: " << simpleMesh.vertexList.size();
				std::cout << "\nSize reduction: " << ((numVertices - simpleMesh.vertexList.size()) / (float)numVertices) * 100.00f << "%";
				std::cout << "\nor " << (simpleMesh.vertexList.size() / (float)numVertices) << " of the expanded size";
			}
			//================= DON'T KNOW WHAT THE FUCK THIS IS ===============
		}
	}
}

void GrabUvs(FbxMesh* mesh, std::vector<FLOAT2>& tempuv)
{
	//================= UV ===============
			//get all UV set names
	FbxStringList lUVSetNameList;
	mesh->GetUVSetNames(lUVSetNameList);

	//iterating over all uv sets
	for (int lUVSetIndex = 0; lUVSetIndex < lUVSetNameList.GetCount(); lUVSetIndex++)
	{
		//get lUVSetIndex-th uv set
		const char* lUVSetName = lUVSetNameList.GetStringAt(lUVSetIndex);
		const FbxGeometryElementUV* lUVElement = mesh->GetElementUV(lUVSetName);

		if (!lUVElement)
			continue;

		// only support mapping mode eByPolygonVertex and eByControlPoint
		if (lUVElement->GetMappingMode() != FbxGeometryElement::eByPolygonVertex &&
			lUVElement->GetMappingMode() != FbxGeometryElement::eByControlPoint)
			return;

		//index array, where holds the index referenced to the uv data
		const bool lUseIndex = lUVElement->GetReferenceMode() != FbxGeometryElement::eDirect;
		const int lIndexCount = (lUseIndex) ? lUVElement->GetIndexArray().GetCount() : 0;

		//iterating through the data by polygon
		const int lPolyCount = mesh->GetPolygonCount();

		if (lUVElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
		{
			for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
			{
				// build the max index array that we need to pass into MakePoly
				const int lPolySize = mesh->GetPolygonSize(lPolyIndex);
				for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
				{
					FbxVector2 lUVValue;

					//get the index of the current vertex in control points array
					int lPolyVertIndex = mesh->GetPolygonVertex(lPolyIndex, lVertIndex);

					//the UV index depends on the reference mode
					int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyVertIndex) : lPolyVertIndex;

					lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

					//User TODO:
					//Print out the value of UV(lUVValue) or log it to a file
					std::cout << "UV: (" << lUVValue.mData[0] << ", " << lUVValue.mData[1] << ")" << std::endl;
					tempuv.push_back(FLOAT2{ static_cast<float>(lUVValue.mData[0]), static_cast<float>(lUVValue.mData[1]) });
				}
			}
		}
		else if (lUVElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
		{
			int lPolyIndexCounter = 0;
			for (int lPolyIndex = 0; lPolyIndex < lPolyCount; ++lPolyIndex)
			{
				// build the max index array that we need to pass into MakePoly
				const int lPolySize = mesh->GetPolygonSize(lPolyIndex);
				for (int lVertIndex = 0; lVertIndex < lPolySize; ++lVertIndex)
				{
					if (lPolyIndexCounter < lIndexCount)
					{
						FbxVector2 lUVValue;

						//the UV index depends on the reference mode
						int lUVIndex = lUseIndex ? lUVElement->GetIndexArray().GetAt(lPolyIndexCounter) : lPolyIndexCounter;

						lUVValue = lUVElement->GetDirectArray().GetAt(lUVIndex);

						//User TODO:
						//Print out the value of UV(lUVValue) or log it to a file
						tempuv.push_back(FLOAT2{ static_cast<float>(lUVValue.mData[0]), static_cast<float>(lUVValue.mData[1]) });

						lPolyIndexCounter++;
					}
				}
			}
		}
	}
	//================= UV ===============
}

void Compactify()
{
	// Re index the motherfucker
	// New vert vector
	std::vector<SimpleVertex> newshit;

	// Tolerance for floating point precision
	float EPSILON = 0.00001f;

	for (int i = 0; i < simpleMesh.indicesList.size(); i++)
	{
		int hit = -1;
		// Check if the vertex and normal is already in another of the unique values
		for (int j = 0; j < newshit.size(); j++)
		{
			// If they are the same, set hit to matching index, else iterate
			if (
				fabs(simpleMesh.vertexList[i].Pos.x - newshit[j].Pos.x) < EPSILON &&
				fabs(simpleMesh.vertexList[i].Pos.y - newshit[j].Pos.y) < EPSILON &&
				fabs(simpleMesh.vertexList[i].Pos.z - newshit[j].Pos.z) < EPSILON &&
				fabs(simpleMesh.vertexList[i].Normal.x - newshit[j].Normal.x) < EPSILON &&
				fabs(simpleMesh.vertexList[i].Normal.y - newshit[j].Normal.y) < EPSILON &&
				fabs(simpleMesh.vertexList[i].Normal.z - newshit[j].Normal.z) < EPSILON &&
				fabs(simpleMesh.vertexList[i].Tex.x - newshit[j].Tex.x) < EPSILON &&
				fabs(simpleMesh.vertexList[i].Tex.y - newshit[j].Tex.y) < EPSILON)
			{
				hit = j;
				break;
			}
		}
		// If hit == -1 then the vertex is unique, add it
		if (hit == -1)
		{
			simpleMesh.indicesList[i] = newshit.size();
			newshit.push_back(simpleMesh.vertexList[i]);
		}
		// If hit != -1 then it's a dupe, don't add it
		else
		{
			// Don't add the vert, but add the updated index
			simpleMesh.indicesList[i] = hit;
		}
	}

	simpleMesh.vertexList.assign(newshit.begin(), newshit.end());
}