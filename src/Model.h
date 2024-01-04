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
	bool Debug = false;

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
		std::string name;
		int id;
		Bone* parent;
		std::vector<Bone*> children;
		glm::vec3 position; // initial pos
		glm::vec3 target_position;
		glm::mat4 InverseBindPose;
		glm::mat4 ParentOffset;

		// dynamic change
		glm::mat4 GlobalTransform;
		glm::mat4 LocalTransform;
		int lastFrame;
	};
	std::vector<Bone> bones;
	std::vector<Bone*> ikBones;
	std::vector<glm::mat4> FinalTransform;
	GLuint ubo;

	Animation* anim;
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

	Model(std::string path, Shader& shader) :shader(shader)
	{
		std::ifstream file(path, std::ios::binary);
		if (file.fail())throw;
		model.Read(&file);

		BuildBuffers(path);

		BoneInit();

		PhysicsInit();
	}

	void Draw(float dt)
	{
		animFrame += dt * 30;

		// Update LocalTransform
		ProcessAnimation();

		UpdateGlobalTransform(&bones[0]);

		ProcessIK();

		ProcessPhysics(dt);

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

		if(Debug)
			DrawDebug();
	}

private:

	void DrawDebug()
	{
		extern Camera camera;

		glDisable(GL_DEPTH_TEST);
		glLineWidth(5);

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((float*)&camera.view);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf((float*)&camera.projection);

		glColor3f(0, 1, 0);
		for (Bone& bone : bones)
		{
			if (bone.target_position == glm::vec3(0))continue;

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glMultMatrixf((float*)&FinalTransform[bone.id]);

			glBegin(GL_LINES);
			glVertex3fv((float*) & bone.position);
			glVertex3fv((float*) & bone.target_position);
			glEnd();

			glPopMatrix();
		}

		glColor3f(1, 0, 0);
		for (RigidBody& body : bodies)
		{
			// draw only capsule shape
			if (body.pmxBody->shape != 2)continue;

			btTransform transform;
			body.btBody->getMotionState()->getWorldTransform(transform);
			glm::mat4 mat;
			transform.getOpenGLMatrix((float*)&mat);

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glMultMatrixf((float*)&mat);

			float capsule_height = body.pmxBody->size[1];
			glBegin(GL_LINES);
			glVertex3f(0, capsule_height/2, 0);
			glVertex3f(0, -capsule_height/2, 0);
			glEnd();

			glPopMatrix();
		}

		glEnable(GL_DEPTH_TEST);
	}

	void BuildBuffers(std::string path)
	{
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
		for (int i = 0; i < model.index_count / 3; i++)
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
	}

	void BoneInit()
	{
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
				bone.parent->children.push_back(&bone);
				bone.ParentOffset = glm::translate(glm::mat4(1), bone.position - bone.parent->position);
			}
			else
			{
				bone.ParentOffset = glm::translate(glm::mat4(1), bone.position);
			}

			if (pmxBone.ik_target_bone_index != 0)
				ikBones.push_back(&bone);
		}

		// set bone target pos for debug draw
		for (Bone& bone : bones)
		{
			if (bone.pmxBone->bone_flag & 0x0001)
			{
				// 1:target_index
				if (bone.pmxBone->target_index == -1)continue;

				bone.target_position = bones[bone.pmxBone->target_index].position;
			}
			else
			{
				// 0:offset
				glm::vec3 offset(
					bone.pmxBone->offset[0],
					bone.pmxBone->offset[1],
					-bone.pmxBone->offset[2]
				);
				bone.target_position = bone.position + offset;
			}
		}
	}

	void PhysicsInit()
	{
		// world
		auto configuration = new btDefaultCollisionConfiguration();
		auto dispatcher = new btCollisionDispatcher(configuration);
		auto overlappingPairCache = new btDbvtBroadphase();
		auto solver = new btSequentialImpulseConstraintSolver();
		world = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, configuration);
		world->setGravity(btVector3(0, -10 * 10, 0));

		// rigid body
		bodies.resize(model.rigid_body_count);
		for (int i = 0; i < model.rigid_body_count; i++)
		{
			PmxRigidBody& pmxBody = model.rigid_bodies[i];
			RigidBody& body = bodies[i];

			body.pmxBody = &pmxBody;

			btCollisionShape* shape = NULL;
			if (pmxBody.shape == 0)
				shape = new btSphereShape(pmxBody.size[0]);
			else if (pmxBody.shape == 1)
				shape = new btBoxShape(btVector3(pmxBody.size[0], pmxBody.size[1], pmxBody.size[2]));
			else if (pmxBody.shape == 2)
				shape = new btCapsuleShape(pmxBody.size[0], pmxBody.size[1]);
			else
				throw;

			float mass = 0;
			btVector3 localInertia(0, 0, 0);
			if (pmxBody.physics_calc_type != 0)
			{
				mass = pmxBody.mass;
				shape->calculateLocalInertia(mass, localInertia);
			}

			glm::mat4 bodyTransform = glm::eulerAngleYXZ(
				-pmxBody.orientation[1],
				-pmxBody.orientation[0],
				pmxBody.orientation[2]
			);
			bodyTransform[3] = glm::vec4(
				pmxBody.position[0],
				pmxBody.position[1],
				-pmxBody.position[2],
				1.0
			);

			btTransform transform;
			transform.setFromOpenGLMatrix((float*)&bodyTransform);
			auto motion = new btDefaultMotionState(transform);

			btRigidBody::btRigidBodyConstructionInfo info(mass, motion, shape, localInertia);
			info.m_friction = pmxBody.friction;
			info.m_restitution = pmxBody.repulsion;
			info.m_linearDamping = pmxBody.move_attenuation;
			info.m_angularDamping = pmxBody.rotation_attenuation;
			body.btBody = new btRigidBody(info);

			if (pmxBody.physics_calc_type == 0)
			{
				body.btBody->setCollisionFlags(body.btBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
				body.btBody->setActivationState(DISABLE_DEACTIVATION);
			}

			world->addRigidBody(body.btBody, 1 << pmxBody.group, pmxBody.mask);

			// bone and rigid body positions are a little bit different
			if (pmxBody.target_bone != -1)
			{
				body.bone = &bones[pmxBody.target_bone];

				body.fromBone = body.bone->InverseBindPose * bodyTransform;
				body.fromBody = glm::inverse(body.fromBone);
			}
		}

		// joint
		for (int i = 0; i < model.joint_count; i++)
		{
			PmxJoint& joint = model.joints[i];

			RigidBody& bodyA = bodies[joint.param.rigid_body1];
			RigidBody& bodyB = bodies[joint.param.rigid_body2];

			btTransform transform;
			transform.setOrigin(btVector3(
				joint.param.position[0],
				joint.param.position[1],
				-joint.param.position[2]
			));
			btQuaternion rot;
			rot.setEuler(
				-joint.param.orientaiton[1],
				-joint.param.orientaiton[0],
				joint.param.orientaiton[2]
			);
			transform.setRotation(rot);

			btTransform frameInA = bodyA.btBody->getWorldTransform();
			frameInA = frameInA.inverse() * transform;
			btTransform frameInB = bodyB.btBody->getWorldTransform();
			frameInB = frameInB.inverse() * transform;

			auto constraint = new btGeneric6DofSpringConstraint(*bodyA.btBody, *bodyB.btBody, frameInA, frameInB, true);
			world->addConstraint(constraint);

			constraint->setLinearLowerLimit(btVector3(
				joint.param.move_limitation_min[0],
				joint.param.move_limitation_min[1],
				joint.param.move_limitation_min[2]
			));
			constraint->setLinearUpperLimit(btVector3(
				joint.param.move_limitation_max[0],
				joint.param.move_limitation_max[1],
				joint.param.move_limitation_max[2]
			));
			constraint->setAngularLowerLimit(btVector3(
				joint.param.rotation_limitation_min[0],
				joint.param.rotation_limitation_min[1],
				joint.param.rotation_limitation_min[2]
			));
			constraint->setAngularUpperLimit(btVector3(
				joint.param.rotation_limitation_max[0],
				joint.param.rotation_limitation_max[1],
				joint.param.rotation_limitation_max[2]
			));

			if (joint.param.spring_move_coefficient[0] != 0)
			{
				constraint->enableSpring(0, true);
				constraint->setStiffness(0, joint.param.spring_move_coefficient[0]);
			}
			if (joint.param.spring_move_coefficient[1] != 0)
			{
				constraint->enableSpring(1, true);
				constraint->setStiffness(1, joint.param.spring_move_coefficient[1]);
			}
			if (joint.param.spring_move_coefficient[2] != 0)
			{
				constraint->enableSpring(2, true);
				constraint->setStiffness(2, joint.param.spring_move_coefficient[2]);
			}
			if (joint.param.spring_rotation_coefficient[0] != 0)
			{
				constraint->enableSpring(3, true);
				constraint->setStiffness(3, joint.param.spring_rotation_coefficient[0]);
			}
			if (joint.param.spring_rotation_coefficient[1] != 0)
			{
				constraint->enableSpring(4, true);
				constraint->setStiffness(4, joint.param.spring_rotation_coefficient[1]);
			}
			if (joint.param.spring_rotation_coefficient[2] != 0)
			{
				constraint->enableSpring(5, true);
				constraint->setStiffness(5, joint.param.spring_rotation_coefficient[2]);
			}
		}
	}

	void ProcessAnimation()
	{
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
		}
	}

	void ProcessIK()
	{
		for (Bone* ikBone : ikBones)
		{
			PmxBone* pmxBone = ikBone->pmxBone;
			Bone* targetBone = &bones[pmxBone->ik_target_bone_index];
			glm::vec3 ikPos = ikBone->GlobalTransform[3];

			for (int i = 0; i < pmxBone->ik_loop; i++)
			{
				glm::vec3 targetPos = targetBone->GlobalTransform[3];
				float distance = glm::distance(ikPos, targetPos);
				if (distance < 0.1)break;

				for (int j = 0; j < pmxBone->ik_link_count; j++)
				{
					PmxIkLink& link = pmxBone->ik_links[j];
					Bone* linkBone = &bones[link.link_target];

					glm::vec3 linkPos = linkBone->GlobalTransform[3];
					glm::vec3 targetPos = targetBone->GlobalTransform[3];

					glm::mat4 invLink = glm::inverse(linkBone->GlobalTransform);

					glm::vec3 localIkPos = invLink * glm::vec4(ikPos, 1);
					glm::vec3 localTargetPos = invLink * glm::vec4(targetPos, 1);

					localIkPos = glm::normalize(localIkPos);
					localTargetPos = glm::normalize(localTargetPos);

					float dot = glm::dot(localIkPos, localTargetPos);
					dot = glm::clamp<float>(dot, -1, 1);
					float theta = std::acos(dot);
					if (theta == 0)continue;

					theta = glm::clamp<float>(theta, 0, pmxBone->ik_loop_angle_limit);

					glm::vec3 axis = glm::cross(localTargetPos, localIkPos);
					if (axis == glm::vec3(0))continue;

					glm::quat ikRot = glm::rotate(glm::quat(1, 0, 0, 0), theta, axis);

					glm::quat animRot = glm::quat_cast(linkBone->LocalTransform);

					glm::quat totalRot = animRot * ikRot;

					if (link.angle_lock)
					{
						glm::vec3 euler = glm::eulerAngles(totalRot);

						euler.x = glm::clamp<float>(euler.x, -link.min_radian[0], -link.max_radian[0]);
						euler.y = glm::clamp<float>(euler.y, -link.min_radian[1], -link.max_radian[1]);
						euler.z = glm::clamp<float>(euler.z, link.max_radian[2], link.min_radian[2]);

						totalRot = glm::quat(euler);
					}

					glm::mat4 newLocal = glm::mat4_cast(totalRot);

					newLocal[3] = linkBone->LocalTransform[3];

					linkBone->LocalTransform = newLocal;

					UpdateGlobalTransform(linkBone);
				}
			}
		}
	}

	void ProcessPhysics(float dt)
	{
		for (RigidBody& body : bodies)
		{
			// do kinematic
			if (body.pmxBody->physics_calc_type != 0)continue;
			if (!body.bone)continue;

			glm::mat4 mat = body.bone->GlobalTransform * body.fromBone;
			btTransform transform;
			transform.setFromOpenGLMatrix((float*)&mat);
			body.btBody->getMotionState()->setWorldTransform(transform);
		}

		world->stepSimulation(dt);

		for (RigidBody& body : bodies)
		{
			// do dynamic
			if (body.pmxBody->physics_calc_type == 0)continue;
			if (!body.bone)continue;

			btTransform transform;
			body.btBody->getMotionState()->getWorldTransform(transform);
			glm::mat4 mat;
			transform.getOpenGLMatrix((float*)&mat);
			body.bone->GlobalTransform = mat * body.fromBody;
		}
	}

	void UpdateGlobalTransform(Bone* bone)
	{
		if (bone->parent)
			bone->GlobalTransform = bone->parent->GlobalTransform * bone->LocalTransform;
		else
			bone->GlobalTransform = bone->LocalTransform;

		for (Bone* child : bone->children)
			UpdateGlobalTransform(child);
	}
};