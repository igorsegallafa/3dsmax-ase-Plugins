#pragma once

class INode;

class Phys
{
public:
	Phys( INode * _node, int _numVertex );
	~Phys();

	void AddBone( size_t numVertex, string boneName );

	INode * GetNode() { return node; }

	vector<string> GetBonesNames() { return bonesNames; }

protected:
	INode * node;
	int numVertex;

	vector<string> bonesNames;
};