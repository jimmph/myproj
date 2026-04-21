#include "myFace.h"
#include "myvector3d.h"
#include "myHalfedge.h"
#include "myVertex.h"
#include <GL/glew.h>

myFace::myFace(void)
{
	adjacent_halfedge = NULL;
	normal = new myVector3D(1.0, 1.0, 1.0);
}

myFace::~myFace(void)
{
	if (normal) delete normal;
}

void myFace::computeNormal()
{
	myHalfedge* e = adjacent_halfedge;

	myPoint3D* a = e->source->point;
	myPoint3D* b = e->next->source->point;
	myPoint3D* c = e->next->next->source->point;

	normal->setNormal(a, b, c);
}
