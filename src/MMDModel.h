#pragma once
#include "config.h"
#include <MMDFormats/Pmx.h>
#include <MMDFormats/EncodingHelper.h>
#include "Shader.h"
#include "Texture.h"
#include "MMDAnimation.h"
#include "Transform.h"
#include "Model.h"

using namespace pmx;

class MMDModel : public Model
{
public:
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
		PmxMaterial* material;
		int index_offset;
	};
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;

	struct Bone
	{
		PmxBone* pmxBone;
		std::string name;
		int id;
		Bone* parent;
		std::vector<Bone*> children;
		glm::vec3 position; // initial pos
		glm::vec3 target_position; // for debug
		glm::mat4 InverseBindPose;
		glm::mat4 ParentOffset;

		// dynamic change
		glm::mat4 GlobalTransform;
		glm::mat4 LocalTransform;
		glm::vec3 trans;
		glm::quat rot;
		int lastFrame;
	};
	std::vector<Bone> bones;
	std::vector<Bone*> ikBones;
	std::vector<glm::mat4> FinalTransform;
	GLuint ubo;

	MMDAnimation* animation;
	float animFrame;

	struct RigidBody
	{
		PmxRigidBody* pmxBody;
		btRigidBody* btBody;
		Bone* bone;
		glm::mat4 fromBone;
		glm::mat4 fromBody;
	};
	std::vector<RigidBody> bodies;
	btDiscreteDynamicsWorld* world;

	struct Morph
	{
		PmxMorph* pmxMorph;
		std::string name;
		int lastFrame;
	};
	std::vector<Morph> morphs;
	std::vector<glm::vec3> morphPos;
	GLuint morphPosBuf;

	MMDModel(std::string path, Shader* shader);
	void Draw(float dt, bool EnableAnimation, bool EnablePhysics, bool DebugDraw);
	void Reset();

private:
	void BuildBuffers();
	void MaterialInit(std::string path);
	void BoneInit();
	void WorldInit();
	void RigidBodyInit();
	void JointInit();
	void MorphInit();

	void ProcessAnimation();
	void GrantParent();
	void ProcessIK();
	void ProcessPhysics(float dt);
	void ProcessMorph();
	void SubProcessMorph(PmxMorph& pmxMorph, float weight);
	void UpdateGlobalTransform(Bone* bone);
	void DrawDebug();
};