#pragma once

class INode;

class Phys
{
public:
	Phys( INode * _node, int _numVertex );
	~Phys();

	void AddBone( size_t numVertex, std::string boneName, float fWeight = 1.0f );

	INode * GetNode() { return node; }

	std::vector<std::vector<std::pair<std::string, float>>> GetBonesNames() { return bonesNames; }

protected:
	INode * node;
	int numVertex;

	std::vector<std::vector<std::pair<std::string, float>>> bonesNames;
};