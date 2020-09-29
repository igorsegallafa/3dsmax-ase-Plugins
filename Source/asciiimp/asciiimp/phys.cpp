#include "stdafx.h"
#include "phys.h"

Phys::Phys( INode * _node, int _numVertex )
{
	node = _node;
	numVertex = _numVertex;

	bonesNames.resize( numVertex );
}

Phys::~Phys()
{
}

void Phys::AddBone( size_t numVertex, string boneName, float fWeight )
{
	size_t size = bonesNames.size();

	if( numVertex < 0 || numVertex >= size )
		return;

	bonesNames[numVertex] = make_pair( boneName, fWeight );
}