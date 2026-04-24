#include "myMesh.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <GL/glew.h>
#include "myvector3d.h"

using namespace std;

myMesh::myMesh(void)
{
	/**** TODO ****/
}


myMesh::~myMesh(void)
{
	/**** TODO ****/
}

void myMesh::clear()
{
	for (unsigned int i = 0; i < vertices.size(); i++) if (vertices[i]) delete vertices[i];
	for (unsigned int i = 0; i < halfedges.size(); i++) if (halfedges[i]) delete halfedges[i];
	for (unsigned int i = 0; i < faces.size(); i++) if (faces[i]) delete faces[i];

	vector<myVertex *> empty_vertices;    vertices.swap(empty_vertices);
	vector<myHalfedge *> empty_halfedges; halfedges.swap(empty_halfedges);
	vector<myFace *> empty_faces;         faces.swap(empty_faces);
}

void myMesh::checkMesh()
{
	vector<myHalfedge *>::iterator it;
	for (it = halfedges.begin(); it != halfedges.end(); it++)
	{
		if ((*it)->twin == NULL)
			break;
	}
	if (it != halfedges.end())
		cout << "Error! Not all edges have their twins!\n";
	else cout << "Each edge has a twin!\n";
}


bool myMesh::readFile(std::string filename)
{
	string s, t, u;
	vector<int> faceids;
	myHalfedge **hedges;

	ifstream fin(filename);
	if (!fin.is_open()) {
		cout << "Unable to open file!\n";
		return false;
	}
	name = filename;

	map<pair<int, int>, myHalfedge *> twin_map;
	map<pair<int, int>, myHalfedge *>::iterator it;

	while (getline(fin, s))
	{
		stringstream myline(s);
		myline >> t;
		if (t == "g") {}
		else if (t == "v")
		{
			float x, y, z;
			myline >> x >> y >> z;
			cout << "v " << x << " " << y << " " << z << endl;
			myPoint3D* p = new myPoint3D(x, y, z);
			myVertex* v = new myVertex;
			v->point = p;
			vertices.push_back(v);
		}
		else if (t == "mtllib") {}
		else if (t == "usemtl") {}
		else if (t == "s") {}
		else if (t == "f")
		{
			faceids.clear();
			while (myline >> u)
				faceids.push_back(atoi((u.substr(0, u.find("/"))).c_str()) - 1);
			if (faceids.size() < 3)
				continue;

			hedges = new myHalfedge * [faceids.size()];
			for (unsigned int i = 0; i < faceids.size(); i++)
				hedges[i] = new myHalfedge();

			myFace* f = new myFace();
			f->adjacent_halfedge = hedges[0];

			for (unsigned int i = 0; i < faceids.size(); i++)
			{
				int iplusone = (i + 1) % faceids.size();
				int iminusone = (i - 1 + faceids.size()) % faceids.size();

				hedges[i]->next = hedges[iplusone];
				hedges[i]->prev = hedges[iminusone];
				hedges[i]->source = vertices[faceids[i]];
				hedges[i]->adjacent_face = f;

				vertices[faceids[i]]->originof = hedges[i];

				pair<int, int> twin_edge = make_pair(faceids[iplusone], faceids[i]);
				it = twin_map.find(twin_edge);
				if (it != twin_map.end()) {
					hedges[i]->twin = it->second;
					it->second->twin = hedges[i];
				}
				twin_map[make_pair(faceids[i], faceids[iplusone])] = hedges[i];

				halfedges.push_back(hedges[i]);
			}

			delete[] hedges;
			faces.push_back(f);
		}
	}

	checkMesh();
	normalize();

	return true;
}


void myMesh::computeNormals()
{
	for (unsigned int i = 0; i < faces.size(); i++)
		faces[i]->computeNormal();

	for (unsigned int i = 0; i < vertices.size(); i++)
		vertices[i]->computeNormal();
}

void myMesh::normalize()
{
	if (vertices.size() < 1) return;

	int tmpxmin = 0, tmpymin = 0, tmpzmin = 0, tmpxmax = 0, tmpymax = 0, tmpzmax = 0;

	for (unsigned int i = 0; i < vertices.size(); i++) {
		if (vertices[i]->point->X < vertices[tmpxmin]->point->X) tmpxmin = i;
		if (vertices[i]->point->X > vertices[tmpxmax]->point->X) tmpxmax = i;

		if (vertices[i]->point->Y < vertices[tmpymin]->point->Y) tmpymin = i;
		if (vertices[i]->point->Y > vertices[tmpymax]->point->Y) tmpymax = i;

		if (vertices[i]->point->Z < vertices[tmpzmin]->point->Z) tmpzmin = i;
		if (vertices[i]->point->Z > vertices[tmpzmax]->point->Z) tmpzmax = i;
	}

	double xmin = vertices[tmpxmin]->point->X, xmax = vertices[tmpxmax]->point->X,
		ymin = vertices[tmpymin]->point->Y, ymax = vertices[tmpymax]->point->Y,
		zmin = vertices[tmpzmin]->point->Z, zmax = vertices[tmpzmax]->point->Z;

	double scale = (xmax - xmin) > (ymax - ymin) ? (xmax - xmin) : (ymax - ymin);
	scale = scale > (zmax - zmin) ? scale : (zmax - zmin);

	for (unsigned int i = 0; i < vertices.size(); i++) {
		vertices[i]->point->X -= (xmax + xmin) / 2;
		vertices[i]->point->Y -= (ymax + ymin) / 2;
		vertices[i]->point->Z -= (zmax + zmin) / 2;

		vertices[i]->point->X /= scale;
		vertices[i]->point->Y /= scale;
		vertices[i]->point->Z /= scale;
	}
}


void myMesh::splitFaceTRIS(myFace *f, myPoint3D *p)
{
	// Collect halfedges around the face
	std::vector<myHalfedge *> hedges;
	myHalfedge *start = f->adjacent_halfedge;
	myHalfedge *current = start;
	do {
		hedges.push_back(current);
		current = current->next;
	} while (current != start);

	int n = hedges.size();

	// Create the new center vertex
	myVertex *v = new myVertex();
	v->point = new myPoint3D(p->X, p->Y, p->Z);
	vertices.push_back(v);

	// Create new faces (reuse f for the first triangle)
	std::vector<myFace *> newfaces(n);
	newfaces[0] = f;
	for (int i = 1; i < n; i++) {
		newfaces[i] = new myFace();
		faces.push_back(newfaces[i]);
	}

	// Create new halfedges: a[i] (from next vertex to v) and b[i] (from v to source of hedge[i])
	std::vector<myHalfedge *> a(n), b(n);
	for (int i = 0; i < n; i++) {
		a[i] = new myHalfedge();
		b[i] = new myHalfedge();
		halfedges.push_back(a[i]);
		halfedges.push_back(b[i]);
	}

	// Set up each triangle: hedges[i] -> a[i] -> b[i]
	for (int i = 0; i < n; i++) {
		a[i]->source = hedges[(i + 1) % n]->source;
		b[i]->source = v;

		hedges[i]->next = a[i];
		a[i]->next = b[i];
		b[i]->next = hedges[i];

		hedges[i]->prev = b[i];
		a[i]->prev = hedges[i];
		b[i]->prev = a[i];

		hedges[i]->adjacent_face = newfaces[i];
		a[i]->adjacent_face = newfaces[i];
		b[i]->adjacent_face = newfaces[i];

		newfaces[i]->adjacent_halfedge = hedges[i];
	}

	// Set twin relationships for internal edges
	// a[i] goes from source_{i+1} to v, b[(i+1)%n] goes from v to source_{i+1}
	for (int i = 0; i < n; i++) {
		a[i]->twin = b[(i + 1) % n];
		b[(i + 1) % n]->twin = a[i];
	}

	// Set the new vertex's originof
	v->originof = b[0];
}

void myMesh::splitEdge(myHalfedge *e1, myPoint3D *p)
{

	/**** TODO ****/
}

void myMesh::splitFaceQUADS(myFace *f, myPoint3D *p)
{
	/**** TODO ****/
}


void myMesh::subdivisionCatmullClark()
{
	/**** TODO ****/
}

static myVector3D cross3D(const myVector3D& a, const myVector3D& b) {
	return myVector3D(
		a.dY * b.dZ - a.dZ * b.dY,
		a.dZ * b.dX - a.dX * b.dZ,
		a.dX * b.dY - a.dY * b.dX
	);
}

static double dot3D(const myVector3D& a, const myVector3D& b) {
	return a.dX * b.dX + a.dY * b.dY + a.dZ * b.dZ;
}

static double convexity(myVertex* prev, myVertex* v, myVertex* next, const myVector3D& n) {
	myVector3D a(
		v->point->X - prev->point->X,
		v->point->Y - prev->point->Y,
		v->point->Z - prev->point->Z
	);
	myVector3D b(
		next->point->X - v->point->X,
		next->point->Y - v->point->Y,
		next->point->Z - v->point->Z
	);
	myVector3D c = cross3D(a, b);
	return dot3D(c, n);  // signe relatif à la normale
}

static bool pointInTriangle3D(myVertex* p, myVertex* a, myVertex* b, myVertex* c, const myVector3D& n) {
	myVector3D v0(c->point->X - a->point->X, c->point->Y - a->point->Y, c->point->Z - a->point->Z);
	myVector3D v1(b->point->X - a->point->X, b->point->Y - a->point->Y, b->point->Z - a->point->Z);
	myVector3D v2(p->point->X - a->point->X, p->point->Y - a->point->Y, p->point->Z - a->point->Z);

	double dot00 = dot3D(v0, v0);
	double dot01 = dot3D(v0, v1);
	double dot02 = dot3D(v0, v2);
	double dot11 = dot3D(v1, v1);
	double dot12 = dot3D(v1, v2);

	double denom = dot00 * dot11 - dot01 * dot01;
	if (fabs(denom) < 1e-12) return false;  // triangle dégénéré
	double invDenom = 1.0 / denom;
	double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	return (u >= 0) && (v >= 0) && (u + v <= 1);
}

static myVector3D computeFaceNormal(const std::vector<myVertex *> &verts) {
    myVector3D n(0, 0, 0);
    int size = verts.size();
    for (int i = 0; i < size; i++) {
        myPoint3D *curr = verts[i]->point;
        myPoint3D *next = verts[(i + 1) % size]->point;
        n.dX += (curr->Y - next->Y) * (curr->Z + next->Z);
        n.dY += (curr->Z - next->Z) * (curr->X + next->X);
        n.dZ += (curr->X - next->X) * (curr->Y + next->Y);
    }
    n.normalize();
    return n;
}

// ========== HELPERS Ear Clipping ==========

// Vrai si B est un sommet convexe par rapport à la normale n de la face.
// Principe : cross(AB, BC) dot normal > 0 ? virage dans le bon sens.
static bool isConvexCorner(myVertex* A, myVertex* B, myVertex* C, myVector3D* n)
{
	myVector3D ab = (*B->point) - (*A->point);
	myVector3D bc = (*C->point) - (*B->point);
	myVector3D cr = ab.crossproduct(bc);
	return (cr * (*n)) > 0;
}

// Vrai si le point P se trouve à l'intérieur du triangle (A, B, C).
// Méthode : les trois produits vectoriels doivent pointer du même côté.
static bool pointInsideTriangle(myPoint3D* A, myPoint3D* B, myPoint3D* C, myPoint3D* P)
{
	myVector3D ab = (*B) - (*A);
	myVector3D bc = (*C) - (*B);
	myVector3D ca = (*A) - (*C);

	myVector3D ap = (*P) - (*A);
	myVector3D bp = (*P) - (*B);
	myVector3D cp = (*P) - (*C);

	myVector3D n1 = ab.crossproduct(ap);
	myVector3D n2 = bc.crossproduct(bp);
	myVector3D n3 = ca.crossproduct(cp);

	return (n1 * n2) >= 0 && (n2 * n3) >= 0 && (n1 * n3) >= 0;
}

// Vrai si une oreille (A, B, C) est "coupable" : aucun sommet reflex restant
// du polygone n'est à l'intérieur du triangle ABC.
// Optimisation TD Q1.2 : on ne teste QUE les sommets reflex.
static bool earIsFree(myHalfedge* earStart, myHalfedge* earEnd,
	myPoint3D* A, myPoint3D* B, myPoint3D* C,
	myVector3D* n)
{
	// On parcourt les sommets "entre" l'oreille (ceux qui ne sont pas A, B, C)
	myHalfedge* walker = earEnd->next;
	while (walker != earStart)
	{
		myVertex* prevV = walker->prev->source;
		myVertex* currV = walker->source;
		myVertex* nextV = walker->next->source;

		// Un sommet reflex est un sommet non convexe
		bool reflex = !isConvexCorner(prevV, currV, nextV, n);

		if (reflex && pointInsideTriangle(A, B, C, currV->point))
			return false;

		walker = walker->next;
	}
	return true;
}

// ========== TRIANGULATION ==========

void myMesh::triangulate()
{
	vector<myFace*> originalFaces = faces;
	for (unsigned int i = 0; i < originalFaces.size(); i++)
		triangulate(originalFaces[i]);
}

bool myMesh::triangulate(myFace* f)
{
	// Compter les sommets de la face
	int remaining = 0;
	myHalfedge* walker = f->adjacent_halfedge;
	do {
		remaining++;
		walker = walker->next;
	} while (walker != f->adjacent_halfedge);

	if (remaining < 3) return false;   // face dégénérée
	if (remaining == 3) return false;  // déjà un triangle

	// Normale de la face (utilisée pour tester la convexité)
	f->computeNormal();

	// Trois pointeurs qui forment l'oreille candidate : ear = (a -> b -> c)
	// b est le sommet "candidat" qu'on essaie de couper
	myHalfedge* a = f->adjacent_halfedge;
	myHalfedge* b = a->next;
	myHalfedge* c = b->next;

	int stuckCounter = 0;
	int stuckLimit = remaining * remaining;

	while (remaining > 3)
	{
		myVertex* vA = a->source;
		myVertex* vB = b->source;
		myVertex* vC = c->source;

		bool convex = isConvexCorner(vA, vB, vC, f->normal);
		bool free = convex && earIsFree(a, c, vA->point, vB->point, vC->point, f->normal);

		if (convex && free)
		{
			// ======= Couper l'oreille (vA, vB, vC) =======
			// On crée une arête interne entre vA et vC (diagonale).
			// Elle donne deux halfedges twins :
			//   - cutNew  : va de vC vers vA, appartient à la NOUVELLE face (le triangle coupé)
			//   - cutKeep : va de vA vers vC, reste dans la face f qui continue d'être traitée
			myHalfedge* cutNew = new myHalfedge();
			myHalfedge* cutKeep = new myHalfedge();

			cutNew->source = vC;
			cutKeep->source = vA;
			cutNew->twin = cutKeep;
			cutKeep->twin = cutNew;

			// Nouvelle face pour le triangle (vA, vB, vC)
			myFace* triFace = new myFace();
			triFace->adjacent_halfedge = a;

			// Le triangle est un cycle fermé : a -> b -> cutNew -> a
			a->adjacent_face = triFace;
			b->adjacent_face = triFace;
			cutNew->adjacent_face = triFace;

			myHalfedge* aPrev = a->prev;  // mémoriser avant d'écraser

			a->prev = cutNew;
			cutNew->next = a;
			cutNew->prev = b;
			b->next = cutNew;

			// La face f continue d'exister avec un sommet en moins : on remplace
			// le trajet "aPrev -> a -> b -> c" par "aPrev -> cutKeep -> c"
			cutKeep->adjacent_face = f;
			aPrev->next = cutKeep;
			cutKeep->prev = aPrev;
			cutKeep->next = c;
			c->prev = cutKeep;

			// Maintenir f->adjacent_halfedge valide (a et b ne lui appartiennent plus)
			f->adjacent_halfedge = cutKeep;

			// Si vA ou vC pointait sur un halfedge qu'on a déplacé, réparer
			if (vA->originof == a) vA->originof = cutKeep;
			if (vC->originof == b || vC->originof == c) vC->originof = c;

			// Enregistrer les nouveaux éléments dans le mesh
			faces.push_back(triFace);
			halfedges.push_back(cutNew);
			halfedges.push_back(cutKeep);

			remaining--;

			// Repartir depuis l'arête qui remplace l'ancienne : l'oreille suivante
			// sera (aPrev, cutKeep, c)
			a = aPrev;
			b = cutKeep;
			c = c;  // c reste le même

			stuckCounter = 0;
			continue;
		}

		// Pas d'oreille ici : avancer les trois pointeurs
		a = b;
		b = c;
		c = c->next;

		stuckCounter++;
		if (stuckCounter > stuckLimit)
		{
			cout << "Warning: ear clipping gave up on a face\n";
			return true;
		}
	}

	// Quand remaining == 3, les trois halfedges restants a, b, c forment déjà
	// un triangle correctement lié dans f (on n'a rien à faire de plus).
	// Juste s'assurer que f pointe sur l'un d'eux.
	f->adjacent_halfedge = a;

	return true;
}