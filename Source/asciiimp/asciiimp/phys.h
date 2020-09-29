#pragma once

class INode;

class Phys
{
public:
	Phys( INode * _node, int _numVertex );
	~Phys();

	void AddBone( size_t numVertex, string boneName, float fWeight = 1.0f );

	INode * GetNode() { return node; }

	std::vector<std::vector<std::pair<string, float>>> GetBonesNames() { return bonesNames; }

protected:
	INode * node;
	int numVertex;

	std::vector<std::vector<std::pair<string, float>>> bonesNames;
};