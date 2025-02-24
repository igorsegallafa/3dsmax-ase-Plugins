#pragma once

class INode;

class Phys
{
public:
	Phys( INode * _node, int _numVertex );
	~Phys();

	void AddBone( size_t numVertex, std::string boneName );

	INode * GetNode() { return node; }

	std::vector<std::string> GetBonesNames() { return bonesNames; }

protected:
	INode * node;
	int numVertex;

	std::vector<std::string> bonesNames;
};