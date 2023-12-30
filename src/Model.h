#pragma once
#include "config.h"
#include <MMDFormats/Pmx.h>
#include <MMDFormats/EncodingHelper.h>
#include "Shader.h"
#include "Texture.h"
#include "Animation.h"

using namespace pmx;

class Model
{
public:
	Shader& shader;
	PmxModel model;
	GLuint vao;
	oguna::EncodingConverter conv;

	struct Vertex
	{
		float position[3];
		float uv[2];
		int boneIDs[4];
		float weights[4];
	};

	struct Mesh
	{
		int index_count;
		int index_offset;
		int texture_index;
	};
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;

	struct Bone
	{
		PmxBone* pmxBone;
		int id;
		std::string name;
		Bone* parent;
		glm::vec3 position;
		glm::mat4 InverseBindPose;
		glm::mat4 ParentOffset;

		glm::mat4 GlobalTransform;
		glm::mat4 LocalTransform;
		int lastFrame;
	};
	std::vector<Bone> bones;
	std::vector<glm::mat4> FinalTransform;
	GLuint ubo;

	Animation* anim;
	float animFrame;

	Model(std::string path, Shader& shader) :shader(shader)
	{
		std::ifstream file(path, std::ios::binary);
		if (file.fail())throw;
		model.Read(&file);

		std::vector<Vertex> vertices(model.vertex_count);
		for (int i = 0; i < model.vertex_count; i++)
		{
			PmxVertex& vertex = model.vertices[i];
			vertices[i].position[0] = vertex.positon[0];
			vertices[i].position[1] = vertex.positon[1];
			vertices[i].position[2] = -vertex.positon[2];
			vertices[i].uv[0] = vertex.uv[0];
			vertices[i].uv[1] = vertex.uv[1];

			switch (vertex.skinning_type)
			{
			case PmxVertexSkinningType::BDEF1:
			{
				auto skinning = (PmxVertexSkinningBDEF1*)vertex.skinning.get();
				vertices[i].boneIDs[0] = skinning->bone_index;
				vertices[i].weights[0] = 1;
			}
			break;
			case PmxVertexSkinningType::BDEF2:
			case PmxVertexSkinningType::SDEF:
			{
				auto skinning = (PmxVertexSkinningBDEF2*)vertex.skinning.get();
				vertices[i].boneIDs[0] = skinning->bone_index1;
				vertices[i].boneIDs[1] = skinning->bone_index2;
				vertices[i].weights[0] = skinning->bone_weight;
				vertices[i].weights[1] = 1 - skinning->bone_weight;
			}
			break;
			case PmxVertexSkinningType::BDEF4:
			{
				auto skinning = (PmxVertexSkinningBDEF4*)vertex.skinning.get();
				vertices[i].boneIDs[0] = skinning->bone_index1;
				vertices[i].boneIDs[1] = skinning->bone_index2;
				vertices[i].boneIDs[2] = skinning->bone_index3;
				vertices[i].boneIDs[3] = skinning->bone_index4;
				vertices[i].weights[0] = skinning->bone_weight1;
				vertices[i].weights[1] = skinning->bone_weight2;
				vertices[i].weights[2] = skinning->bone_weight3;
				vertices[i].weights[3] = skinning->bone_weight4;
			}
			break;
			default:
				throw;
			}
		}

		std::vector<int> indices(model.index_count);
		for (int i = 0; i < model.index_count/3; i++)
		{
			indices[i * 3 + 0] = model.indices[i * 3 + 2];
			indices[i * 3 + 1] = model.indices[i * 3 + 1];
			indices[i * 3 + 2] = model.indices[i * 3 + 0];
		}

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, position));

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, uv));

		glEnableVertexAttribArray(2);
		glVertexAttribIPointer(2, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, weights));

		GLuint ibo;
		glGenBuffers(1, &ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		int offset = 0;
		meshes.resize(model.material_count);
		for (int i = 0; i < model.material_count; i++)
		{
			PmxMaterial& material = model.materials[i];
			meshes[i].index_count = material.index_count;
			meshes[i].index_offset = offset;
			offset += material.index_count;
			meshes[i].texture_index = material.diffuse_texture_index;
		}

		int lastPos = path.find_last_of("/");
		std::string dir = path.substr(0, lastPos + 1);

		textures.resize(model.texture_count);
		for (int i = 0; i < model.texture_count; i++)
		{
			std::wstring wstr = model.textures[i];
			std::string str;
			conv.Utf16ToCp932(wstr.c_str(), wstr.length(), &str);
			textures[i].Load(dir + str);
		}

		glGenBuffers(1, &ubo);
		FinalTransform.resize(model.bone_count);
		bones.resize(model.bone_count);
		for (int i = 0; i < model.bone_count; i++)
		{
			PmxBone& pmxBone = model.bones[i];
			Bone& bone = bones[i];

			bone.pmxBone = &pmxBone;
			bone.id = i;
			conv.Utf16ToCp932(pmxBone.bone_name.c_str(), pmxBone.bone_name.length(), &bone.name);

			bone.position = glm::vec3(
				pmxBone.position[0],
				pmxBone.position[1],
				-pmxBone.position[2]
			);

			bone.InverseBindPose = glm::translate(glm::mat4(1), -bone.position);

			if (pmxBone.parent_index != -1)
			{
				bone.parent = &bones[pmxBone.parent_index];
				bone.ParentOffset = glm::translate(glm::mat4(1), bone.position - bone.parent->position);
			}
			else
			{
				bone.ParentOffset = glm::translate(glm::mat4(1), bone.position);
			}
		}
	}

	void Draw(float dt)
	{
		animFrame += dt * 30;

		for (Bone& bone : bones)
		{
			bone.LocalTransform = bone.ParentOffset;
			if (anim->boneMap.find(bone.name) != anim->boneMap.end())
			{
				std::vector<Animation::BoneFrame>& bone_frames = anim->boneMap[bone.name];

				int i;
				for (i = bone.lastFrame; i < bone_frames.size(); i++)
				{
					if (bone_frames[i].frame > animFrame)
						break;
				}
				bone.lastFrame = i - 1;

				glm::vec3 trans;
				glm::quat rot;
				if (i == bone_frames.size())
				{
					Animation::BoneFrame& bf = bone_frames[i - 1];
					trans = bf.position;
					rot = bf.rotation;
				}
				else
				{
					Animation::BoneFrame& bf0 = bone_frames[i - 1];
					Animation::BoneFrame& bf1 = bone_frames[i];
					float factor = (animFrame - bf0.frame) / (bf1.frame - bf0.frame);
					trans = bf0.position + (bf1.position - bf0.position) * factor;
					rot = glm::slerp(bf0.rotation, bf1.rotation, factor);
				}

				glm::mat4 transMat = glm::translate(glm::mat4(1), trans);
				glm::mat4 rotMat = glm::mat4_cast(rot);
				bone.LocalTransform = bone.ParentOffset * transMat * rotMat;
			}

			if (bone.parent)
				bone.GlobalTransform = bone.parent->GlobalTransform * bone.LocalTransform;
			else
				bone.GlobalTransform = bone.LocalTransform;
		}

		for (Bone& bone : bones)
			FinalTransform[bone.id] = bone.GlobalTransform * bone.InverseBindPose;

		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
		glBufferData(GL_UNIFORM_BUFFER, FinalTransform.size() * sizeof(glm::mat4), FinalTransform.data(), GL_STREAM_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

		glUseProgram(shader.program);
		glBindVertexArray(vao);

		for (Mesh& mesh : meshes)
		{
			if (mesh.texture_index != -1)
				glBindTexture(GL_TEXTURE_2D, textures[mesh.texture_index].id);

			glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, (void*)(mesh.index_offset * sizeof(int)));

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glBindVertexArray(0);
		glUseProgram(0);
	}
};