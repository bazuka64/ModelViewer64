#pragma once
#include "config.h"
#include "Shader.h"
#include "Model.h"
#include "Texture.h"

class SkeletalModel : public Model
{
public:
	Assimp::Importer importer;
	const aiScene* scene;

	struct Vertex
	{
		float position[3];
		float uv[2];
		float normal[3];
		int boneIDs[4];
		float weights[4];
	};

	struct Mesh
	{
		GLuint vao;
		unsigned int index_count;
		aiMesh* aiMesh;
	};
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;

	struct Bone
	{
		int id;
		aiBone* aiBone;
	};
	std::map<std::string, Bone> bones;
	std::vector<aiMatrix4x4> FinalTransform;

	aiAnimation* animation;
	float AnimationTime;

	SkeletalModel(std::string path, Shader* shader) 
		: Model(path, shader)
	{
		scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_GenBoundingBoxes);
		if (!scene)
		{
			print(importer.GetErrorString());
			throw;
		}

		//animation = scene->mAnimations[0];
		animation = scene->mAnimations[scene->mNumAnimations-1];

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
				vertices[j].uv[0] = aiMesh->mTextureCoords[0][j].x;
				vertices[j].uv[1] = aiMesh->mTextureCoords[0][j].y;
				vertices[j].normal[0] = aiMesh->mNormals[j].x;
				vertices[j].normal[1] = aiMesh->mNormals[j].y;
				vertices[j].normal[2] = aiMesh->mNormals[j].z;
			}

			for (int j = 0; j < aiMesh->mNumBones; j++)
			{
				aiBone* aiBone = aiMesh->mBones[j];
				std::string name = aiBone->mName.C_Str();

				if (bones.find(name) == bones.end())
				{
					Bone bone;
					bone.id = bones.size();
					bone.aiBone = aiBone;
					bones[name] = bone;
				}

				for (int k = 0; k < aiBone->mNumWeights; k++)
				{
					aiVertexWeight& vw = aiBone->mWeights[k];
					for (int l = 0; l < 4; l++)
					{
						if (vertices[vw.mVertexId].weights[l] == 0)
						{
							vertices[vw.mVertexId].boneIDs[l] = bones[name].id;
							vertices[vw.mVertexId].weights[l] = vw.mWeight;
							break;
						}
					}
				}
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

			glEnableVertexAttribArray(3);
			glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));

			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 4, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, weights));

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

		if (bones.size() > 200)throw;
		FinalTransform.resize(bones.size());

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
				std::string filename = boostPath.string();

				// hardcode for Pokedex 3D Pro
				if (filename[0] == 'E' && filename[1] == 'f')
					continue;

				textures[i].Load(dir + filename);
			}
		}
	}

	void Draw(float dt)
	{
		AnimationTime += dt * animation->mTicksPerSecond;
		AnimationTime = fmod(AnimationTime, animation->mDuration);

		ProcessNode(scene->mRootNode, aiMatrix4x4());

		glUseProgram(shader->program);

		glUniformMatrix4fv(shader->UniformLocations["FinalTransform"], FinalTransform.size(), true, (float*)FinalTransform.data());

		glUniformMatrix4fv(shader->UniformLocations["model"], 1, false, (float*)&transform.mat);

		for (Mesh& mesh : meshes)
		{
			glBindVertexArray(mesh.vao);

			Texture& texture = textures[mesh.aiMesh->mMaterialIndex];
			if (texture.id == 0)continue;

			glBindTexture(GL_TEXTURE_2D, texture.id);

			glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, 0);

			glBindTexture(GL_TEXTURE_2D, 0);
			glBindVertexArray(0);
		}

		glUseProgram(0);

		DrawAABB();
	}

private:
	void ProcessNode(aiNode* node, const aiMatrix4x4& ParentTransform)
	{
		aiMatrix4x4 NodeTransform = node->mTransformation;

		aiNodeAnim* NodeAnim = NULL;
		for (int i = 0; i < animation->mNumChannels; i++)
		{
			aiNodeAnim* channel = animation->mChannels[i];
			if (channel->mNodeName == node->mName)
			{
				NodeAnim = channel;
				break;
			}
		}
		if (NodeAnim)
			TransRotScale(NodeAnim, NodeTransform);

		aiMatrix4x4 GlobalTransform = ParentTransform * NodeTransform;

		if (bones.find(node->mName.C_Str()) != bones.end())
		{
			Bone& bone = bones[node->mName.C_Str()];
			FinalTransform[bone.id] = GlobalTransform * bone.aiBone->mOffsetMatrix;
		}

		for (int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], GlobalTransform);
		}
	}

	void TransRotScale(aiNodeAnim* NodeAnim, aiMatrix4x4& NodeTransform)
	{
		int i;
		aiVector3D trans;
		aiQuaternion rot;
		aiVector3D scale;

		// translation
		for (i = 0; i < NodeAnim->mNumPositionKeys; i++)
		{
			if (NodeAnim->mPositionKeys[i].mTime > AnimationTime)
				break;
		}
		if (i == NodeAnim->mNumPositionKeys)
		{
			aiVectorKey& key = NodeAnim->mPositionKeys[i-1];
			trans = key.mValue;
		}
		else
		{
			aiVectorKey& key0 = NodeAnim->mPositionKeys[i - 1];
			aiVectorKey& key1 = NodeAnim->mPositionKeys[i];
			float factor = (AnimationTime - key0.mTime) / (key1.mTime - key0.mTime);
			trans = key0.mValue + (key1.mValue - key0.mValue) * factor;
		}

		// rotation
		for (i = 0; i < NodeAnim->mNumRotationKeys; i++)
		{
			if (NodeAnim->mRotationKeys[i].mTime > AnimationTime)
				break;
		}
		if (i == NodeAnim->mNumRotationKeys)
		{
			aiQuatKey& key = NodeAnim->mRotationKeys[i-1];
			rot = key.mValue;
		}
		else
		{
			aiQuatKey& key0 = NodeAnim->mRotationKeys[i - 1];
			aiQuatKey& key1 = NodeAnim->mRotationKeys[i];
			float factor = (AnimationTime - key0.mTime) / (key1.mTime - key0.mTime);
			aiQuaternion::Interpolate(rot, key0.mValue, key1.mValue, factor);
		}
		
		// scale
		for (i = 0; i < NodeAnim->mNumScalingKeys; i++)
		{
			if (NodeAnim->mScalingKeys[i].mTime > AnimationTime)
				break;
		}
		if (i == NodeAnim->mNumScalingKeys)
		{
			aiVectorKey& key = NodeAnim->mScalingKeys[i - 1];
			scale = key.mValue;
		}
		else
		{
			aiVectorKey& key0 = NodeAnim->mScalingKeys[i - 1];
			aiVectorKey& key1 = NodeAnim->mScalingKeys[i];
			float factor = (AnimationTime - key0.mTime) / (key1.mTime - key0.mTime);
			scale = key0.mValue + (key1.mValue - key0.mValue) * factor;
		}

		NodeTransform = aiMatrix4x4(scale, rot, trans);
	}
};