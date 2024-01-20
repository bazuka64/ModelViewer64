#pragma once
#include "config.h"
#include "Shader.h"
#include "Model.h"

class StaticModel : public Model
{
public:
	Assimp::Importer importer;
	const aiScene* scene;

	struct Vertex
	{
		float position[3];
		float uv[2];
		float normal[3];
	};

	struct Mesh
	{
		GLuint vao;
		unsigned int index_count;
		aiMesh* aiMesh;
	};
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;

	StaticModel(std::string path, Shader* shader)
		:Model(path, shader)
	{
		scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_GenBoundingBoxes);
		if (!scene)
		{
			print(importer.GetErrorString());
			throw;
		}

		glm::vec3 max(0);
		glm::vec3 min(0);

		meshes.resize(scene->mNumMeshes);
		for (int i = 0; i < scene->mNumMeshes; i++)
		{
			aiMesh* aiMesh = scene->mMeshes[i];
			Mesh& mesh = meshes[i];
			mesh.aiMesh = aiMesh;

			std::vector<Vertex> vertices(aiMesh->mNumVertices);
			for (int j = 0; j < aiMesh->mNumVertices; j++)
			{
				vertices[j].position[0] = aiMesh->mVertices[j].x;
				vertices[j].position[1] = aiMesh->mVertices[j].y;
				vertices[j].position[2] = aiMesh->mVertices[j].z;
				if (aiMesh->mTextureCoords[0])
				{
					vertices[j].uv[0] = aiMesh->mTextureCoords[0][j].x;
					vertices[j].uv[1] = aiMesh->mTextureCoords[0][j].y;
				}
				vertices[j].normal[0] = aiMesh->mNormals[j].x;
				vertices[j].normal[1] = aiMesh->mNormals[j].y;
				vertices[j].normal[2] = aiMesh->mNormals[j].z;
			}

			mesh.index_count = aiMesh->mNumFaces * 3;
			std::vector<unsigned int> indices(aiMesh->mNumFaces * 3);
			for (int j = 0; j < aiMesh->mNumFaces; j++)
			{
				indices[j * 3 + 0] = aiMesh->mFaces[j].mIndices[0];
				indices[j * 3 + 1] = aiMesh->mFaces[j].mIndices[1];
				indices[j * 3 + 2] = aiMesh->mFaces[j].mIndices[2];
			}

			glGenVertexArrays(1, &mesh.vao);
			glBindVertexArray(mesh.vao);

			GLuint vbo;
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, position));

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, uv));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal));

			GLuint ibo;
			glGenBuffers(1, &ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

			glBindVertexArray(0);

			// scale
			max = glm::max(max, glm::vec3(
				aiMesh->mAABB.mMax.x,
				aiMesh->mAABB.mMax.y,
				aiMesh->mAABB.mMax.z
			));
			min = glm::min(min, glm::vec3(
				aiMesh->mAABB.mMin.x,
				aiMesh->mAABB.mMin.y,
				aiMesh->mAABB.mMin.z
			));
		}

		glm::vec3 size = max - min;
		MaxSize = glm::max(size.x, glm::max(size.y, size.z));
		localMin = min;
		localMax = max;

		int lastPos = path.find_last_of("/\\");
		std::string dir = path.substr(0, lastPos + 1);

		textures.resize(scene->mNumMaterials);
		for (int i = 0; i < scene->mNumMaterials; i++)
		{
			aiMaterial* material = scene->mMaterials[i];
			if (material->GetTextureCount(aiTextureType_DIFFUSE))
			{
				aiString aiPath;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &aiPath);
				boost::filesystem::path boostPath = aiPath.C_Str();
				if (boostPath.is_absolute())
				{
					boostPath = boostPath.filename();
				}
				textures[i].Load(dir + boostPath.string());
			}
		}
	}

	void Draw()
	{
		glUseProgram(shader->program);

		glUniformMatrix4fv(shader->UniformLocations["model"], 1, false, (float*)&transform.mat);

		for (Mesh& mesh : meshes)
		{
			glBindVertexArray(mesh.vao);
			glBindTexture(GL_TEXTURE_2D, textures[mesh.aiMesh->mMaterialIndex].id);

			glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, 0);

			glBindTexture(GL_TEXTURE_2D, 0);
			glBindVertexArray(0);
		}

		glUseProgram(0);

		DrawAABB();
	}
};