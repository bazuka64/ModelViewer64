#pragma once
#include "config.h"
#include <MMDFormats/Vmd.h>

using namespace vmd;

class MMDAnimation
{
public:
	struct BoneFrame
	{
		int frame;
		glm::vec3 position;
		glm::quat rotation;
	};
	std::map<std::string, std::vector<BoneFrame>> boneMap;
	std::map<std::string, std::vector<VmdFaceFrame*>> faceMap;
	std::unique_ptr<VmdMotion> motion;

	MMDAnimation(const char* path)
	{
		motion = VmdMotion::LoadFromFile(path);
		if (!motion)throw;

		// bone animation
		for (VmdBoneFrame& bone_frame : motion->bone_frames)
		{
			if (boneMap.find(bone_frame.name) == boneMap.end())
			{
				boneMap[bone_frame.name] = std::vector<BoneFrame>();
			}

			BoneFrame bf;
			bf.frame = bone_frame.frame;
			bf.position = glm::vec3(
				bone_frame.position[0],
				bone_frame.position[1],
				-bone_frame.position[2]
			);
			bf.rotation = glm::quat(
				bone_frame.orientation[3],
				-bone_frame.orientation[0],
				-bone_frame.orientation[1],
				bone_frame.orientation[2]
			);

			boneMap[bone_frame.name].push_back(bf);
		}

		for (auto& pair : boneMap)
		{
			auto& bone_frames = pair.second;
			std::sort(bone_frames.begin(), bone_frames.end(),
				[](BoneFrame& a, BoneFrame& b)
				{
					return a.frame < b.frame;
				}
			);
		}

		// face animation
		for (VmdFaceFrame& face_frame : motion->face_frames)
		{
			if (faceMap.find(face_frame.face_name) == faceMap.end())
			{
				faceMap[face_frame.face_name] = std::vector<VmdFaceFrame*>();
			}

			faceMap[face_frame.face_name].push_back(&face_frame);
		}

		for (auto& pair : faceMap)
		{
			auto& face_frames = pair.second;
			std::sort(face_frames.begin(), face_frames.end(),
				[](VmdFaceFrame* a, VmdFaceFrame* b)
				{
					return a->frame < b->frame;
				}
			);
		}
	}
};