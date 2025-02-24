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

void Phys::AddBone( size_t numVertex, std::string boneName )
{
	size_t size = bonesNames.size();

	if( numVertex < 0 || numVertex >= size )
		return;

	bonesNames[numVertex] = boneName;
}