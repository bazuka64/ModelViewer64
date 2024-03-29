#pragma once
#include "config.h"

class DebugDrawer : public btIDebugDraw
{
public:
	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
	{
		glColor3fv((float*)&color);
		glBegin(GL_LINES);
		glVertex3fv((float*)&from);
		glVertex3fv((float*)&to);
		glEnd();
	}

	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {}
	virtual void reportErrorWarning(const char* warningString) {}
	virtual void draw3dText(const btVector3& location, const char* textString) {}
	virtual void setDebugMode(int debugMode) {}

	virtual int getDebugMode() const { 
		return btIDebugDraw::DBG_DrawWireframe; 
	}
};