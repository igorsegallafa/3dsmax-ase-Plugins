//************************************************************************** 
//* import.cpp	- Ascii File Importer
//* 
//* NOTE: This is unsupported material - not supported by Kinetix.
//*
//* By Christer Janson
//* Kinetix Development
//*
//* March 19, 1997 CCJ Initial coding
//*
//* This module contains the main parser and object creation functions
//*
//* Copyright (c) 1997, All Rights Reserved. 
//************************************************************************** 

/*********************************************************************************
REVISION LOG ENTRY
Revision By: rjc
Revised on 4/16/2001 7:19:52 PM
Comments: 16 apr 2001  - rjc : Making changes and upgrades for R4
*********************************************************************************/


/*********************************************************************************
REVISION LOG ENTRY
Revision By: rjc
Revised on 06/29/2000 11:18:37 AM
Comments:  rjc 29 jun 2000 :  Making changes and upgrades for R3.1
*********************************************************************************/
#include "stdafx.h"
#include "asciiimp.h"

#define PTIMPORTER_SCALE_BEZIER

#ifdef _DEBUG
TCHAR prev1Token[512];
TCHAR prev2Token[512];
TCHAR prev3Token[512];
#endif

//************************************************************************** 
//
//	GroupManager
//
//************************************************************************** 

// Start a new group with the given name
BOOL GroupManager::BeginGroup(TCHAR* groupName)
{
	INodeTab* group = new INodeTab;
	nodeTabTab.Append(1, &group, 5);
	groupNameTab.AddName(groupName);
	
	return TRUE;
}

// We're done adding nodes to the current group, create the group
// and delete the nodelist for this group
BOOL GroupManager::EndGroup(Interface* i)
{
	INodeTab* group;
	group = nodeTabTab[nodeTabTab.Count()-1];
	
	INode* gnode = i->GroupNodes(group, &MSTR( groupNameTab[groupNameTab.Count() - 1] ), FALSE);
	
	groupNameTab.Delete(groupNameTab.Count()-1, 1);
	nodeTabTab.Delete(nodeTabTab.Count()-1, 1);
	delete group;
	
	// If rIsRecording() is true here, it means that we are in a nested group.
	if (IsRecording()) {
		// Support nested groups - add the current groupNode to the parent group
		AddNode(gnode);
	}
	
	return TRUE;
}

// Query if we have an open active group.
BOOL GroupManager::IsRecording()
{
	return nodeTabTab.Count() > 0;
}

// Add a node to to current group.
BOOL GroupManager::AddNode(INode* node)
{
	INodeTab* group;
	group = nodeTabTab[nodeTabTab.Count()-1];
	group->Append(1, &node, 25);
	
	return TRUE;
}

//************************************************************************** 
//
//	Parser functions
//
//************************************************************************** 


// Check if char is a white space
BOOL AsciiImp::IsWhite(TCHAR c)
{
	if (c < 33) 
	{
		return TRUE;
	}
	
	return FALSE;
}

// Do a string comparison
BOOL AsciiImp::Compare(const TCHAR* token, const TCHAR* id)
{
	if (!token)
		return FALSE;
	
	return !_tcscmp(token, id);
}

// Get a token from the file
TCHAR* AsciiImp::GetToken()
{
	static TCHAR str[512];
	TCHAR* p;
	int ch;
	BOOL quoted = FALSE;
	
	// Turn on to debug previous tokens
#ifdef _DEBUG
	_tcscpy(prev3Token, prev2Token);
	_tcscpy(prev2Token, prev1Token);
	_tcscpy(prev1Token, str);
#endif
	
	p = str;
	
	// Skip white space
	while (IsWhite(ch = _fgettc(pStream)) && !feof(pStream));
	
	// Are we at end of file?
	if (feof(pStream)) 
	{
		return NULL;
	}
	
	if (ch == '\"') 
	{
		quoted = TRUE;
	}
	
	// Read everything that is not white space into the token buffer
	do 
	{
		*p = ch;
		p++;
		ch = _fgettc(pStream);
		if (ch == '\"') 
		{
			quoted = FALSE;
		}
	} while ((quoted || !IsWhite(ch)) && !feof(pStream));
	
	*p = '\0';
	
	if (str[0] == '\0')
		return NULL;
	
	return str;
}

// Skip to the end of this block.
BOOL AsciiImp::SkipBlock()
{
	TCHAR* token;
	int level = 1;
	
	do 
	{
		token = GetToken();
		if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
	} while (level > 0);
	
	return TRUE;
}

// Get a Point3 from the file
Point3 AsciiImp::GetPoint3()
{
	TCHAR* token;
	Point3 p;
	
	token = GetToken();
	p.x = (float)_wtof(token);
	token = GetToken();
	p.y = (float)_wtof(token);
	token = GetToken();
	p.z = (float)_wtof(token);
	
	return p;
}

// Get a float from the file
float AsciiImp::GetFloat()
{
	TCHAR* token;
	float f;
	
	token = GetToken();
	f = (float)_wtof(token);
	
	return f;
}

// Get an integer from the file
int AsciiImp::GetInt()
{
	TCHAR* token;
	int i;
	
	token = GetToken();
	i = _wtoi(token);
	
	return i;
}

// Get a string from the file and strip
// leading and trailing quotes.
TSTR AsciiImp::GetString()
{
	static TSTR string;
	
	string = GetToken();
	string.remove(0, 1);
	string.remove(string.length()-1);
	
	return string;
}

// Get a full Matrix3 from the file
Matrix3 AsciiImp::GetNodeTM(TSTR& name, DWORD& iFlags)
{
	Matrix3 mat;
	TCHAR* token;
	
	iFlags = 0;
	
	//Removed from resource.h a never implemented checkbox?
	//#define IDC_INVERSE_INHERIT             1018
	
	do 
	{
		token = GetToken();
		if (Compare(token, ID_TM_ROW0)) 
		{
			mat.SetRow(0, GetPoint3());
		}
		else if (Compare(token, ID_TM_ROW1)) 
		{
			mat.SetRow(1, GetPoint3());
		}
		else if (Compare(token, ID_TM_ROW2)) 
		{
			mat.SetRow(2, GetPoint3());
		}
		else if (Compare(token, ID_TM_ROW3)) 
		{
			mat.SetRow(3, GetPoint3());
		}
		else if (Compare(token, ID_NODE_NAME)) 
		{
			name = GetString();
		}
		else if (Compare(token, ID_INHERIT_POS)) 
		{
			if( !ignoreInherit ) //New option: Ignore Inherit from import
			{
				Point3 tmp = GetPoint3();
				iFlags = iFlags | (tmp.x ? INHERIT_POS_X : 0);
				iFlags = iFlags | (tmp.y ? INHERIT_POS_Y : 0);
				iFlags = iFlags | (tmp.z ? INHERIT_POS_Z : 0);
			}
			else
				GetPoint3();
		}
		else if (Compare(token, ID_INHERIT_ROT)) 
		{
			if( !ignoreInherit ) //New option: Ignore Inherit from import
			{
				Point3 tmp = GetPoint3();
				iFlags = iFlags | (tmp.x ? INHERIT_ROT_X : 0);
				iFlags = iFlags | (tmp.y ? INHERIT_ROT_Y : 0);
				iFlags = iFlags | (tmp.z ? INHERIT_ROT_Z : 0);
			}
			else
				GetPoint3();
		}
		else if (Compare(token, ID_INHERIT_SCL)) 
		{
			if( !ignoreInherit ) //New option: Ignore Inherit from import
			{
				Point3 tmp = GetPoint3();
				iFlags = iFlags | (tmp.x ? INHERIT_SCL_X : 0);
				iFlags = iFlags | (tmp.y ? INHERIT_SCL_Y : 0);
				iFlags = iFlags | (tmp.z ? INHERIT_SCL_Z : 0);
			}
			else
				GetPoint3();
		}
	} while (!Compare(token, L"}"));
	
	return mat;
}

//***************************************************************************
//
// Other utility functions
//
//***************************************************************************

INode* AsciiImp::GetNodeByName(const TCHAR* name)
{
	for (int i=nodeTab.Count()-1; i>=0; i--) 
	{
		if (Compare(name, nodeTab[i]->GetName())) 
		{
			return nodeTab[i];
		}
	}
	return NULL;
}

BOOL AsciiImp::RecordNode(INode* node)
{
	nodeTab.Append(1, &node, 50);
	
	if (groupMgr.IsRecording()) 
	{
		groupMgr.AddNode(node);
	}
	
	return TRUE;
}

//***************************************************************************
//
// Scene Import
//
//***************************************************************************

// Get scene parameters from the file
BOOL AsciiImp::ImportSceneParams()
{
	TCHAR* token;
	int level = 0;
	Interval range;
	
	do 
	{
		token = GetToken();
		if (Compare(token, ID_FIRSTFRAME)) 
		{
			range.SetStart(GetInt());
		}
		else if (Compare(token, ID_LASTFRAME)) 
		{
			range.SetEnd(GetInt());
		}
		else if (Compare(token, ID_FRAMESPEED)) 
		{
			SetFrameRate(GetInt());
		}
		else if (Compare(token, ID_TICKSPERFRAME)) 
		{
			SetTicksPerFrame(GetInt());
			range.SetStart(range.Start() * GetTicksPerFrame());
			range.SetEnd(range.End() * GetTicksPerFrame());
			ip->SetAnimRange(range);
		}
		else if (Compare(token, ID_STATICBGCOLOR)) 
		{
			impip->SetBackGround(0, GetPoint3());
		}
		else if (Compare(token, ID_STATICAMBIENT)) 
		{
			impip->SetAmbient(0, GetPoint3());
		}
		else if (Compare(token, ID_ENVMAP)) 
		{
			float amount;
			Texmap* env = GetTexture(amount);
			if (env) 
			{
				impip->SetEnvironmentMap(env);
				impip->SetUseMap(TRUE);
			}
		}
		else if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		
	} while (level > 0);
	
	return TRUE;
}

//***************************************************************************
//
// TM Animation import
//
//***************************************************************************

typedef enum { kStd, kBez, kTCB } eValType;

struct PosKeeper {
	TimeValue	t;
	Point3		val;
	eValType	type;
	Point3		inTan;
	Point3		outTan;
	float		tens;
	float		cont;
	float		bias;
	float		easeIn;
	float		easeOut;
	DWORD		flags;
};

struct RotKeeper {
	TimeValue	t;
	Point3		axis;
	float		val;
	eValType	type;
	float		tens;
	float		cont;
	float		bias;
	float		easeIn;
	float		easeOut;
	DWORD		flags;
};

struct ScaleKeeper {
	TimeValue	t;
	Point3		comp;
	Point3		axis;
	float		val;
	eValType	type;
	Point3		inTan;
	Point3		outTan;
	float		tens;
	float		cont;
	float		bias;
	float		easeIn;
	float		easeOut;
	DWORD		flags;
};

BOOL AsciiImp::ImportTMAnimation()
{
	TCHAR* token;
	int level = 0;
	INode* node = NULL;
	IKeyControl* posCont = NULL;
	IKeyControl* rotCont = NULL;
	IKeyControl* scaleCont = NULL;

	static bool bLinearPosition = false;
	static bool bLinearScale = false;

	Tab<PosKeeper*> posTable;
	Tab<RotKeeper*> rotTable;
	Tab<ScaleKeeper*> scaleTable;
	
	do 
	{
		token = GetToken();
		if (Compare(token, ID_NODE_NAME)) 
		{
			node = GetNodeByName(GetString().data());
		}
		/*********************************************************************/
		//
		// Position controllers
		//
		/*********************************************************************/
		else if (Compare(token, ID_CONTROL_POS_TCB)) 
		{
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_POSITION_CLASS_ID, Class_ID(TCBINTERP_POSITION_CLASS_ID, 0));
				node->GetTMController()->SetPositionController(c);
				if (c) 
				{
					posCont = GetKeyControlInterface(c);
				}
			}
		}
		else if (Compare(token, ID_CONTROL_POS_BEZIER)) 
		{
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_POSITION_CLASS_ID, Class_ID(HYBRIDINTERP_POSITION_CLASS_ID, 0));
				node->GetTMController()->SetPositionController(c);
				if (c) 
				{
					posCont = GetKeyControlInterface(c);
				}
			}
		}
		else if (Compare(token, ID_CONTROL_POS_LINEAR)) 
		{
			bLinearPosition = true;

			if (node) 
			{				
				Control* c = (Control*)CreateInstance(CTRL_POSITION_CLASS_ID, Class_ID(LININTERP_POSITION_CLASS_ID, 0));
				node->GetTMController()->SetPositionController(c);
				if (c) 
				{
					posCont = GetKeyControlInterface(c);
				}
			}
		}
		// Create a Bezier position controller for sampled keys
		else if (Compare(token, ID_POS_TRACK)) 
		{
			bLinearPosition = gameMode ? true : false;
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_POSITION_CLASS_ID, Class_ID( gameMode ? LININTERP_POSITION_CLASS_ID : HYBRIDINTERP_POSITION_CLASS_ID, 0));
				node->GetTMController()->SetPositionController(c);
				if (c) 
				{
					posCont = GetKeyControlInterface(c);
				}
			}
		}
		/*********************************************************************/
		//
		// Rotation controllers
		//
		/*********************************************************************/
		else if (Compare(token, ID_CONTROL_ROT_TCB)) 
		{
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(TCBINTERP_ROTATION_CLASS_ID, 0));
				node->GetTMController()->SetRotationController(c);
				if (c) 
				{
					rotCont = GetKeyControlInterface(c);
				}
			}
		}
		else if (Compare(token, ID_CONTROL_ROT_BEZIER)) 
		{
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID, 0));
				node->GetTMController()->SetRotationController(c);
				if (c) 
				{
					rotCont = GetKeyControlInterface(c);
				}
			}
		}
		else if (Compare(token, ID_CONTROL_ROT_LINEAR)) 
		{
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(LININTERP_ROTATION_CLASS_ID, 0));
				node->GetTMController()->SetRotationController(c);
				if (c) 
				{
					rotCont = GetKeyControlInterface(c);
				}
			}
		}
		// Create a TCB rotation controller for sampled keys
		else if (Compare(token, ID_ROT_TRACK)) 
		{
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(TCBINTERP_ROTATION_CLASS_ID, 0));
				node->GetTMController()->SetRotationController(c);
				if (c) 
				{
					rotCont = GetKeyControlInterface(c);
				}
			}
		}
		/*********************************************************************/
		//
		// Scale controllers
		//
		/*********************************************************************/
		else if (Compare(token, ID_CONTROL_SCALE_TCB)) 
		{
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_SCALE_CLASS_ID, Class_ID(TCBINTERP_SCALE_CLASS_ID, 0));
				node->GetTMController()->SetScaleController(c);
				if (c) 
				{
					scaleCont = GetKeyControlInterface(c);
				}
			}
		}
		else if (Compare(token, ID_CONTROL_SCALE_BEZIER)) 
		{
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_SCALE_CLASS_ID, Class_ID(HYBRIDINTERP_SCALE_CLASS_ID, 0));
				node->GetTMController()->SetScaleController(c);
				if (c) 
				{
					scaleCont = GetKeyControlInterface(c);
				}
			}
		}
		else if (Compare(token, ID_CONTROL_SCALE_LINEAR)) 
		{
			bLinearScale = true;

			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_SCALE_CLASS_ID, Class_ID(LININTERP_SCALE_CLASS_ID, 0));
				node->GetTMController()->SetScaleController(c);
				if (c) 
				{
					scaleCont = GetKeyControlInterface(c);
				}
			}
		}
		// Create a Bezier scale controller for sampled keys
		else if (Compare(token, ID_SCALE_TRACK)) 
		{
			bLinearScale = gameMode ? true : false;
			if (node) 
			{
				Control* c = (Control*)CreateInstance(CTRL_SCALE_CLASS_ID, Class_ID( gameMode ? LININTERP_SCALE_CLASS_ID : HYBRIDINTERP_SCALE_CLASS_ID, 0));
				node->GetTMController()->SetScaleController(c);
				if (c) 
				{
					scaleCont = GetKeyControlInterface(c);
				}
			}
		}
		/*********************************************************************/
		//
		// Controller keys
		//
		/*********************************************************************/
		else if (Compare(token, ID_POS_SAMPLE) || Compare(token, ID_POS_KEY)) 
		{
			PosKeeper* p = new PosKeeper;
			p->t = GetInt();
			p->val = GetPoint3();
			
			// Bezier tangents
			p->inTan = p->val;
			p->outTan = p->val;
			p->flags = 0;
			
			p->type = bLinearPosition ? kStd : kBez;
			
			posTable.Append(1, &p, 5);
		}
		else if (Compare(token, ID_BEZIER_POS_KEY)) 
		{
			PosKeeper* p = new PosKeeper;
			p->t = GetInt();
			p->val = GetPoint3();
			
			// Bezier tangents
			p->inTan = GetPoint3();
			p->outTan = GetPoint3();
			p->flags = GetInt();
			
			p->type = kBez;
			
			posTable.Append(1, &p, 5);
		}
		else if (Compare(token, ID_TCB_POS_KEY)) 
		{
			PosKeeper* p = new PosKeeper;
			p->t = GetInt();
			p->val = GetPoint3();
			
			p->tens = GetFloat();
			p->cont = GetFloat();
			p->bias = GetFloat();
			p->easeIn = GetFloat();
			p->easeOut = GetFloat();
			p->type = kTCB;
			
			posTable.Append(1, &p, 5);
		}
		else if (Compare(token, ID_ROT_KEY) || Compare(token, ID_ROT_SAMPLE)) 
		{
			RotKeeper* r = new RotKeeper;
			r->t = GetInt();
			r->axis = GetPoint3();
			r->val = GetFloat();
			
			r->tens = 0.0f;
			r->cont = 0.0f;
			r->bias = 0.0f;
			r->easeIn = 0.0f;
			r->easeOut = 0.0f;
			r->type = kTCB;
			
			rotTable.Append(1, &r, 5);
		}
		else if (Compare(token, ID_TCB_ROT_KEY)) 
		{
			RotKeeper* r = new RotKeeper;
			r->t = GetInt();
			r->axis = GetPoint3();
			r->val = GetFloat();
			
			r->tens = GetFloat();
			r->cont = GetFloat();
			r->bias = GetFloat();
			r->easeIn = GetFloat();
			r->easeOut = GetFloat();
			r->type = kTCB;
			
			rotTable.Append(1, &r, 5);
		}
		else if (Compare(token, ID_SCALE_SAMPLE) || Compare(token, ID_SCALE_KEY)) 
		{
#ifdef PTIMPORTER_SCALE_BEZIER
			ScaleKeeper * s = new ScaleKeeper;
			s->t = GetInt();
			s->comp = GetPoint3();
			s->axis = Point3( 0.0f, 0.0f, 0.0f );
			s->val = 0.0f;

			s->tens = 0.0f;
			s->cont = 0.0f;
			s->bias = 0.0f;
			s->easeIn = 0.0f;
			s->easeOut = 0.0f;

			s->type = bLinearScale ? kStd : kBez;
#else
			ScaleKeeper* s = new ScaleKeeper;
			s->t = GetInt();
			s->comp = GetPoint3();
			s->axis = GetPoint3();
			s->val = GetFloat();
			
			s->tens = 0.0f;
			s->cont = 0.0f;
			s->bias = 0.0f;
			s->easeIn = 0.0f;
			s->easeOut = 0.0f;
			
			s->type = kTCB;
#endif
			scaleTable.Append(1, &s, 5);
		}
		else if (Compare(token, ID_TCB_SCALE_KEY)) 
		{
#ifdef PTIMPORTER_SCALE_BEZIER
			ScaleKeeper * s = new ScaleKeeper;
			s->t = GetInt();
			s->comp = GetPoint3();
			s->axis = Point3( 0.0f, 0.0f, 0.0f );
			s->val = 0.0f;

			s->tens = 0.0f;
			s->cont = 0.0f;
			s->bias = 0.0f;
			s->easeIn = 0.0f;
			s->easeOut = 0.0f;

			s->type = kBez;
#else
			ScaleKeeper* s = new ScaleKeeper;
			s->t = GetInt();
			s->comp = GetPoint3();
			s->axis = GetPoint3();
			s->val = GetFloat();
			
			s->tens = GetFloat();
			s->cont = GetFloat();
			s->bias = GetFloat();
			s->easeIn = GetFloat();
			s->easeOut = GetFloat();
			
			s->type = kTCB;
#endif		
			scaleTable.Append(1, &s, 5);

		}
		else if (Compare(token, ID_BEZIER_SCALE_KEY)) 
		{
#ifdef PTIMPORTER_SCALE_BEZIER
			ScaleKeeper * s = new ScaleKeeper;
			s->t = GetInt();
			s->comp = GetPoint3();
			s->axis = Point3( 0.0f, 0.0f, 0.0f );
			s->val = 0.0f;

			s->tens = 0.0f;
			s->cont = 0.0f;
			s->bias = 0.0f;
			s->easeIn = 0.0f;
			s->easeOut = 0.0f;

			s->type = kBez;
#else
			ScaleKeeper* s = new ScaleKeeper;
			s->t = GetInt();
			s->comp = GetPoint3();
			s->axis = GetPoint3();
			s->val = GetFloat();
			
			s->inTan = GetPoint3();
			s->outTan = GetPoint3();
			s->flags = GetInt();
			
			s->type = kBez;
#endif			
			scaleTable.Append(1, &s, 5);
		}
		else if (Compare(token, L"{"))
			level++;
		else if ( Compare( token, L"}" ) )
		{
			level--;

			if ( bLinearScale )
				bLinearScale = false;

			if ( bLinearPosition )
				bLinearPosition = false;
		}
	} while (level > 0);
	
	
	if (posCont) 
	{
		posCont->SetNumKeys(posTable.Count());
		for (int i=0; i<posTable.Count(); i++) 
		{
			PosKeeper* p = posTable[i];
			if (p->type == kBez) 
			{
				IBezPoint3Key posKey;
				posKey.time = p->t;
				posKey.val = p->val;
				posKey.intan = p->inTan;
				posKey.outtan = p->outTan;
				posKey.flags = p->flags;

				//Game Mode?
				if ( gameMode )
				{
					//Set Key Tangents to Linear
					SetInTanType( posKey.flags, BEZKEY_LINEAR );
					SetOutTanType( posKey.flags, BEZKEY_LINEAR );
				}

				posCont->SetKey(i, &posKey);
			}
			else if (p->type == kTCB) 
			{
				ITCBPoint3Key posKey;
				posKey.time = p->t;
				posKey.val = p->val;
				posKey.tens = p->tens;
				posKey.cont = p->cont;
				posKey.bias = p->bias;
				posKey.easeIn = p->easeIn;
				posKey.easeOut = p->easeOut;

				//Game Mode?
				if ( gameMode )
				{
					//Set Key Tangents to Linear
					SetInTanType( posKey.flags, BEZKEY_LINEAR );
					SetOutTanType( posKey.flags, BEZKEY_LINEAR );
				}

				posCont->SetKey(i, &posKey);
			}
			else if (p->type == kStd) 
			{
				ILinPoint3Key posKey;
				posKey.time = p->t;
				posKey.val = p->val;
				posCont->SetKey(i, &posKey);
			}
			delete posTable[i];
		}
		posTable.ZeroCount();
		posTable.Shrink();
		posCont->SortKeys();
	}
	
	if (rotCont) 
	{
		rotCont->SetNumKeys(rotTable.Count());
		for (int i=0; i<rotTable.Count(); i++) 
		{
			RotKeeper* r = rotTable[i];
			if (r->type == kTCB) {
				ITCBRotKey rotKey;
				rotKey.time = r->t;
				rotKey.val = AngAxis(r->axis, r->val);
				rotKey.tens = r->tens;
				rotKey.cont = r->cont;
				rotKey.bias = r->bias;
				rotKey.easeIn = r->easeIn;
				rotKey.easeOut = r->easeOut;
				rotCont->SetKey(i, &rotKey);
			}
			else if (r->type == kStd) 
			{
				IBezQuatKey rotKey;
				rotKey.time = r->t;
				rotKey.val = Quat(AngAxis(r->axis, r->val));
				rotCont->SetKey(i, &rotKey);
			}
			
			delete rotTable[i];
		}
		rotTable.ZeroCount();
		rotTable.Shrink();
		rotCont->SortKeys();
	}
	
	if (scaleCont) 
	{
		scaleCont->SetNumKeys(scaleTable.Count());
		for (int i=0; i<scaleTable.Count(); i++) 
		{
			ScaleKeeper* s = scaleTable[i];
			if (s->type == kTCB) 
			{
				ITCBScaleKey scaleKey;
				scaleKey.time = s->t;
				scaleKey.val = ScaleValue(s->comp, Quat(AngAxis(s->axis, s->val)));
				scaleKey.tens = s->tens;
				scaleKey.cont = s->cont;
				scaleKey.bias = s->bias;
				scaleKey.easeIn = s->easeIn;
				scaleKey.easeOut = s->easeOut;
				scaleCont->SetKey(i, &scaleKey);
			}
			if (s->type == kBez) 
			{
				IBezScaleKey scaleKey;
				scaleKey.time = s->t;
				scaleKey.val = ScaleValue(s->comp, Quat(AngAxis(s->axis, s->val)));
				scaleKey.intan = s->inTan;
				scaleKey.outtan = s->outTan;
				scaleKey.flags = s->flags;

				//Game Mode?
				if ( gameMode )
				{
					//Set Key Tangents to Linear
					SetInTanType( scaleKey.flags, BEZKEY_LINEAR );
					SetOutTanType( scaleKey.flags, BEZKEY_LINEAR );
				}

				scaleCont->SetKey(i, &scaleKey);
			}
			if (s->type == kStd) 
			{
				ILinScaleKey scaleKey;
				scaleKey.time = s->t;
				scaleKey.val = ScaleValue(s->comp, Quat(AngAxis(s->axis, s->val)));
				scaleCont->SetKey(i, &scaleKey);
			}
			delete scaleTable[i];
		}
		scaleTable.ZeroCount();
		scaleTable.Shrink();
		scaleCont->SortKeys();
	}
	
	/* Doesn't compute interpolation
	if (rotCont) {
	for (int i=0; i<rotTable.Count(); i++) {
	RotKeeper* r = rotTable[i];
	ITCBRotKey* rotKey = new ITCBRotKey;
	rotKey->time = r->t;
	rotKey->val = AngAxis(r->axis, r->val);
	rotKey->tens = 0.0f;
	rotKey->cont = 0.0f;
	rotKey->bias = 0.0f;
	rotCont->AppendKey(rotKey);
	delete rotTable[i];
	}
	rotTable.ZeroCount();
	rotTable.Shrink();
	rotCont->SortKeys();
	}
	*/
	
	
	// Arrange the keys and recompute tangents
	if (posCont)
		posCont->SortKeys();
	
	if (scaleCont)
		scaleCont->SortKeys();
	
	return TRUE;
}

//Added by Sandurr
//Reads each line of physique assignment and stores it in a temporary map
//The map is then handled at the end of the full import of the file, at FinalizeImport
BOOL AsciiImp::ImportPhysique(INode *pNode)
{
	//We receive the vertex num and and bone node name and put it into a list for later
	//used later in FinalizeVertex

	TCHAR* token;
	int level = 0;

	Phys * p = NULL;
	
	do 
	{
		token = GetToken();
		if (Compare(token, ID_PHYSIQUE_NUM_VERTASSINE)) 
		{
			int numVertex = GetInt();

			p = new Phys( pNode, numVertex );

			physList.push_back( p );
		}
		if (Compare(token, ID_PHYSIQUE_NOBLEND_RIGID))
		{
			int vertex = GetInt();
			WStr boneName = GetString();

			if( p )
			{
				p->AddBone( vertex, boneName.ToCStr().data() );
			}
		}
		else if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
	} while (level > 0);

	return TRUE;
}

//Added by Sandurr..
//To import physique block from ase.. can go into Skin (preferred) or Physique (rather not)
void AsciiImp::FinalizePhysique()
{
	if( physList.size() <= 0 )
		return;

	//From the list compiled before we can now build the physique data

	//Physique modifier creation is possible but adding vertices to it is not
	//So, we're going to convert Physique right away to the new Skin system of 3DS!!!

	//Create the modifiers
	Modifier *skinmodglobal = (Modifier*)ip->CreateInstance(OSM_CLASS_ID,SKIN_CLASSID);
	Modifier *phymodglobal = (Modifier*)ip->CreateInstance(OSM_CLASS_ID,Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B));

	if( !skinmodglobal || !phymodglobal )
	{
		ERRORBOX( "skinmodglobal or phymodglobal NULL" );
		return;
	}

	//Don't necessarily need Bip01 when we are going for Skin.. so only running this when going for Physique!
	if( !phys2skin )
	{
		INode * root = GetNodeByName(L"Bip01");
					
		if( !root )
		{
			ERRORBOX( "Could not find node Bip01. Must exist to use Physique!" );
			return;
		}

		//Use root node
		IPhysiqueImport * phyImp = (IPhysiqueImport*)phymodglobal->GetInterface(I_PHYIMPORT);
		phyImp->AttachRootNode(root,0);
		phyImp->InitializePhysique(root,0);
	}

	//Loop through each GEOMOBJECT that has Physique block...
	for( UINT i = 0; i < physList.size(); i++ )
	{
		ip->ForceCompleteRedraw(0);

		Phys * p = physList[i];

		if( p )
		{
			INode * pNode = p->GetNode();

			if(pNode)
			{
				Object * obj = pNode->GetObjectRef();
				IDerivedObject * der_obj = (IDerivedObject*)obj;

				if( obj->SuperClassID() != GEN_DERIVOB_CLASS_ID )
				{
					der_obj = CreateDerivedObject();
					der_obj->TransferReferences(obj);
					der_obj->ReferenceObject(obj);
				}

				//If user selected physique to skin then we do that!
				if( phys2skin )
				{
					Modifier * mod = (Modifier*)ip->CreateInstance(OSM_CLASS_ID,SKIN_CLASSID);

					//Try to convert Physique directly to Skin

					//Add skin modifier to object
					der_obj->SetAFlag( A_LOCK_TARGET );
					der_obj->AddModifier( mod, NULL, 0 );
					der_obj->ClearAFlag( A_LOCK_TARGET );

					//Skin interface
					ISkin *skin = (ISkin*)mod->GetInterface(I_SKIN);
					if( !skin )
					{
						ERRORBOX( "ISkin null" );
						return;
					}
				
					//Create skin import interface
					ISkinImportData * skinImp = (ISkinImportData*)mod->GetInterface(I_SKINIMPORTDATA);

					if( !skinImp )
					{
						ERRORBOX( "skinImp NULL" );
						break;
					}

					//Make the bones tab and clear it first
					INodeTab bones;
					bones.SetCount( 0 );
					
					//Loop through each vertex
					std::vector<std::string> bonesNames = p->GetBonesNames();
					for( UINT j = 0; j < bonesNames.size(); j++ )
					{
						int numVertex = j;
						std::string boneName = bonesNames[j];

						//Now we can get the bone node since we already gone through each geomobject..
						INode * pBoneNode = GetNodeByName( GetWC( boneName.c_str() ));
						if(pBoneNode)
						{
							bool exists = false;

							for( int z = 0; z < bones.Count(); z++ )
							{
								if( bones[z] == pBoneNode )
									exists = true;
							}

							if( !exists )
								bones.Append( 1, &pBoneNode );
						}
						else
							ERRORBOX( "Phys 2 Skin ERROR [1]: Bone %s not found!", boneName.c_str() );
					}

					for( int i = 0; i < bones.Count(); i++ )
					{						
						if( !skinImp->AddBoneEx( bones[i], TRUE ) )
							ERRORBOX( "AddBoneEx failed for bone %s", bones[i]->GetName() );
						
						Matrix3 initTM;

						skin->GetBoneInitTM( bones[i], initTM );

						if( !skinImp->SetBoneTm( bones[i], initTM, initTM ) )
							ERRORBOX( "SetBoneTm failed for bone %s", bones[i]->GetName() );
					}

					pNode->EvalWorldState( GetCOREInterface()->GetTime() );
					
					bonesNames = p->GetBonesNames();
					for( UINT j = 0; j < bonesNames.size(); j++ )
					{
						der_obj->Eval( 0, 0 );
						
						int numVertex = j;
						std::string boneName = bonesNames[j];
						
						//Now we can get the bone node since we already gone through each geomobject..
						INode * pBoneNode = GetNodeByName( GetWC( boneName.c_str()) );
						if(pBoneNode)
						{
							Tab<INode*> boneImp;
							Tab<float> weightImp;

							boneImp.Append( 1, &pBoneNode );
							float fWeight = 1.0f;
							weightImp.Append( 1, &fWeight );

							if( !skinImp->AddWeights( pNode, numVertex, boneImp, weightImp ) )
								ERRORBOX( "AddWeights failed for vertex # %d [Node: %s]", numVertex, pNode->GetName() );
						}
						else
							ERRORBOX( "Phys 2 Skin ERROR [2]: Bone %s not found!", boneName.c_str() );
					}
				}
				else
				{
					//Try Physique... might fail because 3ds max doesnt support it anymore, prefers skin!

					//Add physique modifier to object
					Matrix3 modmatrix(TRUE);
					ModContext * mc = new ModContext(&modmatrix, NULL, NULL);
					der_obj->AddModifier( phymodglobal, mc, 0 );

					//Contexts
					IPhysiqueImport * phyImp = (IPhysiqueImport*)phymodglobal->GetInterface(I_PHYIMPORT);
					ip->ForceCompleteRedraw();
					IPhyContextImport *phyContImp = (IPhyContextImport*)phyImp->GetContextInterface(pNode);

					pNode->EvalWorldState( GetCOREInterface()->GetTime() );

					//Go through each vertex assignment
					std::vector<std::string> bonesNames = p->GetBonesNames();
					for( UINT j = 0; j < bonesNames.size(); j++ )
					{
						der_obj->Eval( 0, 0 );
						
						int numVertex = j;
						std::string boneName = bonesNames[numVertex];

						//Now we can get the bone node since we already gone through each geomobject..
						INode * pBoneNode = GetNodeByName(GetWC(boneName.c_str()));
						if(pBoneNode)
						{
							IPhyVertexImport * vertImp = phyContImp->SetVertexInterface(numVertex,RIGID_NON_BLENDED_TYPE);

							if( !vertImp )
							{
								ERRORBOX( "[%d] vertImp NULL.. Physique vertex assignment might look distorted", numVertex );
								break;
							}

							IPhyRigidVertexImport * rigidImp = (IPhyRigidVertexImport*)vertImp;
							rigidImp->SetNode(pBoneNode);	//only one bone so safe to do
							
							phyContImp->ReleaseVertexInterface(vertImp);
						}
						else
							ERRORBOX( "Physique ERROR: Bone %s not found!", boneName.c_str() );
					}

					phyImp->ReleaseContextInterface(phyContImp);
					phymodglobal->ReleaseInterface(I_PHYIMPORT,phyImp);
				}
			}

			delete p;
		}
	}
	physList.clear();
}

//***************************************************************************
//
// GeomObject Import
//
//***************************************************************************

// Get a list of vertices
BOOL AsciiImp::GetVertexList(TriObject* tri)
{
	BOOL done = FALSE;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_VERTEX)) 
		{
			int vxNo = GetInt();
			Point3 vx = GetPoint3();
			tri->mesh.setVert(vxNo, vx);
		}
	} while (!done);
	
	return TRUE;
}

// Convert a string with comma separated smoothing groups into a DWORD
DWORD AsciiImp::GetSmoothingGroups(TCHAR* smStr)
{
	wchar_t * token;
	DWORD smGroup = 0;
	
	token = _tcstok(smStr, L",");
	while(token) 
	{
		smGroup = smGroup + (1<<(_wtoi(token)-1));
		token = _tcstok(NULL, L",");
	}
	
	return smGroup;
}

BOOL TMNegParity(Matrix3 &m)
{
	return (DotProd(CrossProd(m.GetRow(0),m.GetRow(1)),m.GetRow(2))<0.0)?1:0;
}

// Get a list of faces
BOOL AsciiImp::GetFaceList(TriObject* tri, Matrix3& tm)
{
	BOOL negScale = TMNegParity(tm);
	int vx1, vx2, vx3;
	
	// Order of the vertices. Get 'em counter clockwise if the objects is
	// negatively scaled.
	if (negScale) {
		vx1 = 2;
		vx2 = 1;
		vx3 = 0;
	}
	else {
		vx1 = 0;
		vx2 = 1;
		vx3 = 2;
	}

	BOOL done = FALSE;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_FACE)) 
		{
			int faceNo = GetInt();
			Face* f = &tri->mesh.faces[faceNo];
			
			// Get vertex assignment
			GetToken(); // Junk
			f->v[vx1] = GetInt();
			GetToken(); // Junk
			f->v[vx2] = GetInt();
			GetToken(); // Junk
			f->v[vx3] = GetInt();
			
			// Get face visibility
			GetToken(); // Junk
			f->setEdgeVis(vx1, GetInt());
			token = GetToken(); // Junk
			f->setEdgeVis(vx2, GetInt());
			token = GetToken(); // Junk
			f->setEdgeVis(vx3, GetInt());
			
			// Get face smoothing group
			GetToken(); // Junk
			token = GetToken();
			if (!Compare(token, ID_MESH_MTLID)) 
			{
				f->setSmGroup(GetSmoothingGroups(token));
				GetToken(); // Junk (MESH_MTL_ID)
			}
			f->setMatID((MtlID)GetInt());
		}
	} while (!done);
	
	return TRUE;
}

// Get a list of texture vertices
BOOL AsciiImp::GetTVertexList(TriObject* tri)
{
	BOOL done = FALSE;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_TVERT)) 
		{
			int vxNum = GetInt();
			Point3 vx = GetPoint3();
			tri->mesh.setTVert(vxNum, vx);
		}
	} while (!done);
	
	return TRUE;
}

// Get a list of texture faces
BOOL AsciiImp::GetTFaceList(TriObject* tri, Matrix3& tm)
{
	BOOL negScale = TMNegParity(tm);
	int vx1, vx2, vx3;
	
	// Order of the vertices. Get 'em counter clockwise if the objects is
	// negatively scaled.
	if (negScale) {
		vx1 = 2;
		vx2 = 1;
		vx3 = 0;
	}
	else {
		vx1 = 0;
		vx2 = 1;
		vx3 = 2;
	}

	BOOL done = FALSE;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_TFACE)) 
		{
			int faceNum = GetInt();
			Point3 tf = GetPoint3();
			tri->mesh.tvFace[faceNum].t[vx1] = (int)tf.x;
			tri->mesh.tvFace[faceNum].t[vx2] = (int)tf.y;
			tri->mesh.tvFace[faceNum].t[vx3] = (int)tf.z;
		}
	} while (!done);
	
	return TRUE;
}

// Get a list of texture vertices in Mapping Channel
BOOL AsciiImp::GetTVertexListMap(TriObject* tri, int mp)
{
	BOOL done = FALSE;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_TVERT)) 
		{
			int vxNum = GetInt();
			Point3 vx = GetPoint3();
			tri->mesh.setMapVert(mp, vxNum, vx);
		}
	} while (!done);
	
	return TRUE;
}

// Get a list of texture faces in Mapping Channel
BOOL AsciiImp::GetTFaceListMap(TriObject* tri, int mp, Matrix3& tm)
{
	BOOL negScale = TMNegParity(tm);
	int vx1, vx2, vx3;
	
	// Order of the vertices. Get 'em counter clockwise if the objects is
	// negatively scaled.
	if (negScale) {
		vx1 = 2;
		vx2 = 1;
		vx3 = 0;
	}
	else {
		vx1 = 0;
		vx2 = 1;
		vx3 = 2;
	}

	BOOL done = FALSE;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_TFACE)) 
		{
			int faceNum = GetInt();
			Point3 tf = GetPoint3();
			tri->mesh.mapFaces(mp)[faceNum].t[vx1] = (int)tf.x;
			tri->mesh.mapFaces(mp)[faceNum].t[vx2] = (int)tf.y;
			tri->mesh.mapFaces(mp)[faceNum].t[vx3] = (int)tf.z;
		}
	} while (!done);
	
	return TRUE;
}

// Get a Mapping Channel (implemented by Sandurr)
BOOL AsciiImp::GetMappingChannel(TriObject* tri, Matrix3& tm)
{
	BOOL done = FALSE;
	TCHAR* token;

	int mp = GetInt();

	tri->mesh.setMapSupport(mp);
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_NUMTVERTEX)) 
		{
			tri->mesh.setNumMapVerts(mp, GetInt());
		}
		else if (Compare(token, ID_MESH_NUMTVFACES)) 
		{
			tri->mesh.setNumMapFaces(mp, GetInt());
		}
		else if (Compare(token, ID_MESH_TVERTLIST)) 
		{
			GetTVertexListMap(tri, mp);
		}
		else if (Compare(token, ID_MESH_TFACELIST)) 
		{
			GetTFaceListMap(tri, mp, tm);
		}
	} while (!done);
	
	return TRUE;
}

// Get a list of vertex colors
BOOL AsciiImp::GetCVertexList(TriObject* tri)
{
	BOOL done = FALSE;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_VERTCOL)) 
		{
			int vxNum = GetInt();
			Point3 vx = GetPoint3();
			tri->mesh.vertCol[vxNum] = vx;
		}
	} while (!done);
	
	return TRUE;
}

// Get a list of "vertex color faces"
BOOL AsciiImp::GetCFaceList(TriObject* tri, Matrix3& tm)
{
	BOOL negScale = TMNegParity(tm);
	int vx1, vx2, vx3;
	
	// Order of the vertices. Get 'em counter clockwise if the objects is
	// negatively scaled.
	if (negScale) {
		vx1 = 2;
		vx2 = 1;
		vx3 = 0;
	}
	else {
		vx1 = 0;
		vx2 = 1;
		vx3 = 2;
	}

	BOOL done = FALSE;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_CFACE)) 
		{
			int faceNum = GetInt();
			Point3 tf = GetPoint3();
			tri->mesh.vcFace[faceNum].t[vx1] = (int)tf.x;
			tri->mesh.vcFace[faceNum].t[vx2] = (int)tf.y;
			tri->mesh.vcFace[faceNum].t[vx3] = (int)tf.z;
		}
	} while (!done);
	
	return TRUE;
}


// Get face & vertex normals
BOOL AsciiImp::GetMeshNormals(TriObject* tri)
{
	BOOL done = FALSE;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_MESH_CFACE)) 
		{
			int faceNum = GetInt();
			Point3 tf = GetPoint3();
			tri->mesh.vcFace[faceNum].t[0] = (int)tf.x;
			tri->mesh.vcFace[faceNum].t[1] = (int)tf.y;
			tri->mesh.vcFace[faceNum].t[2] = (int)tf.z;
		}
	} while (!done);
	
	return TRUE;
}

// Get a full mesh definition and stuff it into the TriObject
BOOL AsciiImp::GetMesh(TriObject* tri, Matrix3& tm)
{
	int level = 0;
	TCHAR* token;
	
	do {
		token = GetToken();
		if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		else if (Compare(token, ID_MESH_NUMVERTEX)) 
		{
			tri->mesh.setNumVerts(GetInt());
		}
		else if (Compare(token, ID_MESH_NUMFACES)) 
		{
			tri->mesh.setNumFaces(GetInt());
		}
		else if (Compare(token, ID_MESH_NUMTVERTEX)) 
		{
			tri->mesh.setNumTVerts(GetInt());
		}
		else if (Compare(token, ID_MESH_NUMTVFACES)) 
		{
			tri->mesh.setNumTVFaces(GetInt());
		}
		else if (Compare(token, ID_MESH_NUMCVERTEX)) 
		{
			tri->mesh.setNumVertCol(GetInt());
		}
		else if (Compare(token, ID_MESH_NUMCVFACES)) 
		{
			tri->mesh.setNumVCFaces(GetInt());
		}
		else if (Compare(token, ID_MESH_VERTEX_LIST)) 
		{
			GetVertexList(tri);
		}
		else if (Compare(token, ID_MESH_NORMALS)) 
		{
			GetMeshNormals(tri);
		}
		else if (Compare(token, ID_MESH_FACE_LIST)) 
		{
			GetFaceList(tri, tm);
		}
		else if (Compare(token, ID_MESH_TVERTLIST)) 
		{
			GetTVertexList(tri);
		}
		else if (Compare(token, ID_MESH_TFACELIST)) 
		{
			GetTFaceList(tri, tm);
		}
		else if (Compare(token, ID_MESH_MAPPINGCHANNEL))
		{
			//Added by Sandurr
			GetMappingChannel(tri, tm);
		}
		else if (Compare(token, ID_MESH_CVERTLIST)) 
		{
			GetCVertexList(tri);
		}
		else if (Compare(token, ID_MESH_CFACELIST)) 
		{
			GetCFaceList(tri, tm);
		}
		else if (Compare(token, ID_TIMEVALUE)) 
		{
			GetInt();
		}
	} while (level != 0);
	
	return TRUE;
}

// Get a GeomObject from the file
BOOL AsciiImp::ImportGeomObject()
{
	TCHAR*	token;
	Matrix3 nodeTM(1);
	TSTR	nodeName;
	int 	level = 0;
	DWORD	iFlags = 0;
	
	INode* parentNode = NULL;
	TriObject* tri = CreateNewTriObject();
	
#ifdef USE_IMPNODES
	ImpNode* node = impip->CreateNode();
	node->Reference(tri);
	impip->AddNodeToScene(node);
	INode* realINode = node->GetINode();
#else
	INode* realINode = ip->CreateObjectNode(tri);
#endif
	
	RecordNode(realINode);
	
	do {
		token = GetToken();
		if (Compare(token, ID_NODE_TM)) 
		{
			TSTR tmp;
			nodeTM = GetNodeTM(tmp, iFlags);
		}
		else if (Compare(token, ID_NODE_NAME)) 
		{
			nodeName = GetString().data();
#ifdef USE_IMPNODES
			node->SetName(nodeName);
#else
			realINode->SetName(nodeName);
#endif
		}
		else if (Compare(token, ID_NODE_PARENT)) 
		{
			parentNode = GetNodeByName(GetString().data());
			if (parentNode && parentNode != realINode) 
			{
				parentNode->AttachChild(realINode, 1);
			}
		}
		else if (Compare(token, ID_MESH)) 
		{
			GetMesh(tri, nodeTM);
			// The mesh has its vertices in world space. 
			// Transform them to object space
			Matrix3 invNodeTM = Inverse(nodeTM);
			for (int f=0; f<tri->mesh.numVerts; f++) 
			{
				tri->mesh.verts[f] = invNodeTM * tri->mesh.verts[f];
			}
		}
		else if (Compare(token, ID_MESH_ANIMATION)) 
		{
			// Can't import.
			// Unless, of course, we go to some effort and create morph targets
			GetToken(); 	// Skip leading "{"
			SkipBlock();	// Skip until matching "}"
		}
		else if (Compare(token, ID_MATERIAL_REF)) 
		{
			int mtlID = GetInt();
			
			if (mtlID > mtlTab.Count()) 
			{
				// Error
			}
			else {
				Mtl* mtl = mtlTab[mtlID];
				if (mtl) {
					realINode->SetMtl(mtl);
				}
			}
		}
		else if (Compare(token, ID_WIRECOLOR)) 
		{
			Color col = GetPoint3();
			realINode->SetWireColor(RGB(col.r * 255.0f, col.g * 255.0f, col.b * 255.0f));
		}
		else if (Compare(token, ID_PROP_MOTIONBLUR)) 
		{
			int motBlur = GetInt();
			realINode->SetMotBlur(motBlur);
		}
		else if (Compare(token, ID_PROP_CASTSHADOW)) 
		{
			int castShadows = GetInt();
			realINode->SetCastShadows(castShadows);
		}
		else if (Compare(token, ID_PROP_RECVSHADOW)) 
		{
			int recvShadows = GetInt();
			realINode->SetRcvShadows(recvShadows);
		}
		else if (Compare(token, ID_TM_ANIMATION)) 
		{
			ImportTMAnimation();
		}
		else if (Compare(token, ID_PHYSIQUE)) 
		{
			ImportPhysique(realINode);
		}
		else if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
	} while (level > 0);
	
#ifdef USE_IMPNODES
	node->SetTransform(0, nodeTM);
#else
	realINode->SetNodeTM(0, nodeTM);
#endif
	
	Control* tmCont = realINode->GetTMController();
	if (tmCont) 
	{
		tmCont->SetInheritanceFlags(~iFlags, TRUE);
	}
	
	tri->mesh.InvalidateTopologyCache();
	tri->mesh.InvalidateGeomCache();
	tri->mesh.buildNormals();
	
	return TRUE;
}

//***************************************************************************
//
// Camera Import
//
//***************************************************************************

// Get camera settings
TimeValue AsciiImp::GetCameraSettings(GenCamera* cam)
{
	BOOL done = FALSE;
	TCHAR* token;
	TimeValue time;
	
	cam->SetManualClip(0); // default
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_TIMEVALUE)) 
		{
			time = (TimeValue)GetInt();
		}
		else if (Compare(token, ID_CAMERA_HITHER)) 
		{
			cam->SetManualClip(1);
			cam->SetClipDist(time, CAM_HITHER_CLIP, GetFloat());
		}
		else if (Compare(token, ID_CAMERA_YON)) 
		{
			cam->SetClipDist(time, CAM_YON_CLIP, GetFloat());
		}
		else if (Compare(token, ID_CAMERA_NEAR)) 
		{
			cam->SetEnvRange(time, ENV_NEAR_RANGE, GetFloat());
		}
		else if (Compare(token, ID_CAMERA_FAR)) 
		{
			cam->SetEnvRange(time, ENV_FAR_RANGE, GetFloat());
		}
		else if (Compare(token, ID_CAMERA_FOV)) 
		{
			cam->SetFOV(time, GetFloat());
		}
		else if (Compare(token, ID_CAMERA_TDIST)) 
		{
			cam->SetTDist(time, GetFloat());
		}
	} while (!done);
	
	return time;
}

// Get a Camera from the file
BOOL AsciiImp::ImportCamera()
{
	TCHAR* token;
	Matrix3 nodeTM(1);
	BOOL gotNodeTM = FALSE;
	BOOL hasTarget = FALSE;
	Matrix3 targetTM(1);
	TSTR nodeName, targetName;
	int level = 0;
	DWORD	iFlags = INHERIT_ALL;
	DWORD	targetIFlags = INHERIT_ALL;
	
	GenCamera* cam = NULL;
	
	do {
		token = GetToken();
		
		// Bug out if something goes terribly wrong
		if (*token == '\0')
			return FALSE;
		
		// We can see that we have a target if we get two node TM's.
		if (Compare(token, ID_NODE_TM)) 
		{
			if (!gotNodeTM) {
				TSTR tmp;
				nodeTM = GetNodeTM(tmp, iFlags);
				gotNodeTM = TRUE;
			}
			else { // Target objects's TM
				targetTM = GetNodeTM(targetName, targetIFlags);
				hasTarget = TRUE;
			}
		}
		else if (Compare(token, ID_NODE_NAME)) 
		{
			nodeName = GetString();
		}
		else if (Compare(token, ID_TM_ANIMATION)) 
		{
			ImportTMAnimation();
		}
		else if (Compare(token, ID_CAMERA_SETTINGS)) 
		{
			if (hasTarget) 
			{
				cam = impip->CreateCameraObject(TARGETED_CAMERA);
			}
			else 
			{
				cam = impip->CreateCameraObject(FREE_CAMERA);
			}
			GetCameraSettings(cam);
		}
		else if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		
	} while (level > 0);
	
#ifdef USE_IMPNODES
	ImpNode* node = impip->CreateNode();
	ImpNode* targetNode;
	node->Reference(cam);
	node->SetName(nodeName);
	impip->AddNodeToScene(node);
	
	INode* realINode = node->GetINode();
#else
	INode* realINode = ip->CreateObjectNode(cam);
	realINode->SetName(nodeName);
#endif
	
	RecordNode(realINode);
	
	if (hasTarget) 
	{
#ifdef USE_IMPNODES
		targetNode = impip->CreateNode();
		Object* target = impip->CreateTargetObject();
		targetNode->Reference(target);
		targetNode->SetName(targetName);
		impip->AddNodeToScene(targetNode);
		impip->BindToTarget(node, targetNode);
		targetNode->SetTransform(0, targetTM);
		realINode = targetNode->GetINode();
		RecordNode(realINode);
#else
		Object* target = (Object*)ip->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(TARGET_CLASS_ID, 0x00));
		INode* targetNode = ip->CreateObjectNode(target);
		ip->BindToTarget(realINode, targetNode);
		targetNode->SetName(targetName);
		targetNode->SetNodeTM(0, targetTM);
		RecordNode(targetNode);
#endif
	}
	
#ifdef USE_IMPNODES
	node->SetTransform(0, nodeTM);
#else
	realINode->SetNodeTM(0, nodeTM);
#endif
	
	cam->Enable(TRUE);
	
	return TRUE;
}


//***************************************************************************
//
// Light Import
//
//***************************************************************************

TimeValue AsciiImp::GetLightSettings(GenLight* light)
{
	BOOL done = FALSE;
	TCHAR* token;
	TimeValue time;
	
	do {
		token = GetToken();
		if (Compare(token, L"}"))
			done = TRUE;
		else if (Compare(token, ID_TIMEVALUE)) {
			time = (TimeValue)GetInt();
		}
		else if (Compare(token, ID_LIGHT_COLOR)) {
			Point3 col = GetPoint3();
			light->SetRGBColor(time, col);
		}
		else if (Compare(token, ID_LIGHT_INTENS)) {
			light->SetIntensity(time, GetFloat());
		}
		else if (Compare(token, ID_LIGHT_HOTSPOT)) {
			light->SetHotspot(time, GetFloat());
		}
		else if (Compare(token, ID_LIGHT_FALLOFF)) {
			light->SetFallsize(time, GetFloat());
		}
		else if (Compare(token, ID_LIGHT_ATTNSTART)) {
			light->SetUseAtten(1);
			light->SetAtten(time, ATTEN_START, GetFloat());
		}
		else if (Compare(token, ID_LIGHT_ATTNEND)) {
			light->SetAtten(time, ATTEN_END, GetFloat());
		}
		else if (Compare(token, ID_LIGHT_ASPECT)) {
			light->SetAspect(time, GetFloat());
		}
		else if (Compare(token, ID_LIGHT_TDIST)) {
			light->SetTDist(time, GetFloat());
		}
		else if (Compare(token, ID_LIGHT_MAPBIAS)) {
			light->SetMapBias(time, GetFloat());
		}
		else if (Compare(token, ID_LIGHT_MAPRANGE)) {
			light->SetMapRange(time, GetFloat());
		}
		else if (Compare(token, ID_LIGHT_MAPSIZE)) {
			light->SetMapSize(time, GetInt());
		}
		else if (Compare(token, ID_LIGHT_RAYBIAS)) {
			light->SetRayBias(time, GetFloat());
		}
	} while (!done);
	
	return time;
}

BOOL AsciiImp::GetLightExclusions(GenLight* light)
{
	BOOL done = FALSE;
	TCHAR* token;
	
	NameTab el;
	
	do {
		token = GetToken();
		if (Compare(token, L"}")) {
			done = TRUE;
		}
		else if (Compare(token, ID_LIGHT_NUMEXCLUDED)) {
			GetInt(); // Unused
		}
		else if (Compare(token, ID_LIGHT_EXCLINCLUDE)) {
			el.SetFlag(NT_INCLUDE, GetInt());
		}
		else if (Compare(token, ID_LIGHT_EXCL_AFFECT_ILLUM)) {
			el.SetFlag(NT_AFFECT_ILLUM, GetInt());
		}
		else if (Compare(token, ID_LIGHT_EXCL_AFFECT_SHAD)) {
			el.SetFlag(NT_AFFECT_SHADOWCAST, GetInt());
		}
		else if (Compare(token, ID_LIGHT_EXCLUDED)) {
			el.AddName(GetString().data());
		}
	} while (!done);
	
	light->SetExclusionList((ExclList &)el); // 16 apr 2001  - rjc : Please refer to 3ds max R4 sdk help doc
	// Class ExclList : public InterfaceServer
	// Description:
	// This class is available in release 4.0 and later only.
	// This class represents an exclusion list and is a direct parallel to the NameTab, 
	// and converting from using one to the other is fairly simple.
	
	return TRUE;
}

// Get a Light from the file
BOOL AsciiImp::ImportLight()
{
	TCHAR* token;
	Matrix3 nodeTM(1);
	BOOL gotNodeTM = FALSE;
	BOOL hasTarget = FALSE;
	Matrix3 targetTM(1);
	TSTR nodeName, targetName;
	int level = 0;
	DWORD	iFlags = INHERIT_ALL;
	DWORD	targetIFlags = INHERIT_ALL;
	
	GenLight* light = NULL;
	
	do {
		token = GetToken();
		
		// Bug out if something goes terribly wrong
		if (*token == '\0')
			return FALSE;
		
		// We can see that we have a target if we get two node TM's.
		if (Compare(token, ID_NODE_TM)) {
			if (!gotNodeTM) {
				TSTR tmp;
				nodeTM = GetNodeTM(tmp, iFlags);
				gotNodeTM = TRUE;
			}
			else { // Target objects's TM
				targetTM = GetNodeTM(targetName, targetIFlags);
				hasTarget = TRUE;
			}
		}
		else if (Compare(token, ID_NODE_NAME)) {
			nodeName = GetString();
		}
		else if (Compare(token, ID_TM_ANIMATION)) {
			ImportTMAnimation();
		}
		else if (Compare(token, ID_LIGHT_TYPE)) {
			token = GetToken();
			if (Compare(token, _T("Omni"))) {
				light = impip->CreateLightObject(OMNI_LIGHT);
			}
			else if (Compare(token, _T("Target"))) {
				light = impip->CreateLightObject(TSPOT_LIGHT);
			}
			else if (Compare(token, _T("Directional"))) {
				light = impip->CreateLightObject(DIR_LIGHT);
			}
			else if (Compare(token, _T("Free"))) {
				light = impip->CreateLightObject(FSPOT_LIGHT);
			}
		}
		else if (Compare(token, ID_LIGHT_SETTINGS)) {
			GetLightSettings(light);
		}
		else if (Compare(token, ID_LIGHT_SHADOWS)) {
			GetToken();
			if (Compare(token, _T("Off"))) {
				light->SetShadow(0);
			}
			else if (Compare(token, _T("Mapped"))) {
				light->SetShadow(1);
				light->SetShadowType(0);
			}
			else if (Compare(token, _T("Raytraced"))) {
				light->SetShadow(1);
				light->SetShadowType(1);
			}
		}
		else if (Compare(token, ID_LIGHT_USELIGHT)) {
			light->SetUseLight(GetInt());
		}
		else if (Compare(token, ID_LIGHT_USEGLOBAL)) {
			light->SetUseGlobal(GetInt());
		}
		else if (Compare(token, ID_LIGHT_ABSMAPBIAS)) {
			light->SetAbsMapBias(GetInt());
		}
		else if (Compare(token, ID_LIGHT_OVERSHOOT)) {
			light->SetOvershoot(GetInt());
		}
		else if (Compare(token, ID_LIGHT_SPOTSHAPE)) {
			GetToken();
			if (Compare(token, _T("Rect"))) {
				light->SetSpotShape(RECT_LIGHT);
			}
			else {
				light->SetSpotShape(CIRCLE_LIGHT);
			}
		}
		else if (Compare(token, ID_LIGHT_EXCLUSIONLIST)) {
			GetLightExclusions(light);
		}
		else if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		
	} while (level > 0);
	
#ifdef USE_IMPNODES
	ImpNode* node = impip->CreateNode();
	ImpNode* targetNode;
	node->Reference(light);
	node->SetName(nodeName);
	impip->AddNodeToScene(node);
	
	INode* realINode = node->GetINode();
#else
	INode* realINode = ip->CreateObjectNode(light);
	realINode->SetName(nodeName);
	ip->AddLightToScene(realINode);
#endif
	RecordNode(realINode);
	
	if (hasTarget) {
#ifdef USE_IMPNODES
		targetNode = impip->CreateNode();
		Object* target = impip->CreateTargetObject();
		targetNode->Reference(target);
		targetNode->SetName(targetName);
		impip->AddNodeToScene(targetNode);
		impip->BindToTarget(node, targetNode);
		targetNode->SetTransform(0, targetTM);
		realINode = targetNode->GetINode();
		RecordNode(realINode);
#else
		Object* target = (Object*)ip->CreateInstance(GEOMOBJECT_CLASS_ID, Class_ID(TARGET_CLASS_ID, 0x00));
		INode* targetNode = ip->CreateObjectNode(target);
		ip->BindToTarget(realINode, targetNode);
		targetNode->SetName(targetName);
		targetNode->SetNodeTM(0, targetTM);
		RecordNode(targetNode);
#endif
	}
	
#ifdef USE_IMPNODES
	node->SetTransform(0, nodeTM);
#else
	realINode->SetNodeTM(0, nodeTM);
#endif
	light->Enable(TRUE);
	
	return TRUE;
}


//***************************************************************************
//
// Shape Import
//
//***************************************************************************

BOOL AsciiImp::ImportShape()
{
	TCHAR*	token;
	Matrix3 nodeTM(1);
	TSTR	nodeName;
	int 	level = 0;
	Spline3D*	spline = NULL;
	DWORD	iFlags = INHERIT_ALL;
	
	SplineShape* obj = (SplineShape *)ip->CreateInstance(SHAPE_CLASS_ID, splineShapeClassID);
	
	do {
		token = GetToken();
		
		// Bug out if something goes terribly wrong
		if (*token == '\0')
			return FALSE;
		
		if (Compare(token, ID_NODE_TM)) {
			TSTR tmp;
			nodeTM = GetNodeTM(tmp, iFlags);
		}
		else if (Compare(token, ID_NODE_NAME)) {
			nodeName = GetString();
		}
		else if (Compare(token, ID_TM_ANIMATION)) {
			ImportTMAnimation();
		}
		else if (Compare(token, ID_SHAPE_LINE)) {
			GetFloat(); // Junk
			if (spline) {
				// Finish previous spline
				spline->ComputeBezPoints();
			}
			spline = obj->shape.NewSpline();
		}
		else if (Compare(token, ID_SHAPE_CLOSED)) {
			spline->SetClosed(1);
		}
		else if (Compare(token, ID_SHAPE_VERTEX_KNOT)) {
			GetFloat();
			Point3 pt = Inverse(nodeTM) * GetPoint3();
			spline->AddKnot(SplineKnot(KTYPE_CORNER, LTYPE_CURVE, pt, pt, pt));
		}
		else if (Compare(token, ID_SHAPE_VERTEX_INTERP)) {
			// Convert interpolated knots to "real" ones.
			GetFloat();
			Point3 pt = Inverse(nodeTM) * GetPoint3();
			spline->AddKnot(SplineKnot(KTYPE_CORNER, LTYPE_CURVE, pt, pt, pt));
		}
		else if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		
	} while (level > 0);
	
	if (spline) {
		spline->ComputeBezPoints();
	}
	obj->shape.UpdateSels();
	obj->shape.InvalidateGeomCache();
	
#ifdef USE_IMPNODES
	ImpNode* node = impip->CreateNode();
	node->Reference(obj);
	node->SetName(nodeName);
	impip->AddNodeToScene(node);
	node->SetTransform(0, nodeTM);
	INode* realINode = node->GetINode();
#else
	INode* realINode = ip->CreateObjectNode(obj);
	realINode->SetName(nodeName);
	realINode->SetNodeTM(0, nodeTM);
#endif
	
	RecordNode(realINode);
	
	return TRUE;
}

//***************************************************************************
//
// Helper Import
//
//***************************************************************************

BOOL AsciiImp::ImportHelper()
{
	TCHAR*	token;
	Matrix3 nodeTM(1);
	int 	level = 0;
	Point3	boxMin, boxMax;
	DWORD	iFlags = 0;
	INode* parentNode = NULL;
	
	DummyObject* obj = (DummyObject*)CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID, 0));
	
#ifdef USE_IMPNODES
	ImpNode* node = impip->CreateNode();
	node->Reference(obj);
	impip->AddNodeToScene(node);
	
	INode* realINode = node->GetINode();
#else
	INode* realINode = ip->CreateObjectNode(obj);
#endif
	RecordNode(realINode);
	
	do {
		token = GetToken();
		
		// Bug out if something goes terribly wrong
		if (*token == '\0')
			return FALSE;
		
		if (Compare(token, ID_NODE_TM)) {
			TSTR tmp;
			nodeTM = GetNodeTM(tmp, iFlags);
		}
		else if (Compare(token, ID_NODE_PARENT)) {
			parentNode = GetNodeByName(GetString().data());
			if (parentNode && parentNode != realINode) {
				parentNode->AttachChild(realINode, 1);
			}
		}
		else if (Compare(token, ID_NODE_NAME)) {
#ifdef USE_IMPNODES
			node->SetName(GetString().data());
#else
			realINode->SetName(GetString());
#endif
		}
		else if (Compare(token, ID_BOUNDINGBOX_MIN)) {
			boxMin = Inverse(nodeTM) * GetPoint3();
		}
		else if (Compare(token, ID_BOUNDINGBOX_MAX)) {
			boxMax = Inverse(nodeTM) * GetPoint3();
		}
		else if (Compare(token, ID_TM_ANIMATION)) {
			ImportTMAnimation();
		}
		else if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		
	} while (level > 0);
	
	Box3 boundingBox(boxMin, boxMax);
	obj->SetBox(boundingBox);
#ifdef USE_IMPNODES
	node->SetTransform(0, nodeTM);
#else
	realINode->SetNodeTM(0, nodeTM);
#endif
	
	Control* tmCont = realINode->GetTMController();
	if (tmCont) {
		tmCont->SetInheritanceFlags(~iFlags, TRUE);
	}
	
	return TRUE;
}

//***************************************************************************
//
// Material Import
//
//***************************************************************************

BOOL AsciiImp::ImportMaterialList()
{
	TCHAR*	token;
	int 	level = 0;
	int 	numMtls = 0;
	
	do {
		token = GetToken();
		if (Compare(token, ID_MATERIAL)) {
			int mtlNum = GetInt();
			Mtl* mtl = GetMaterial();
			if (mtl) {
				mtlTab.Insert(mtlNum, 1, &mtl);
			}
		}
		else if (Compare(token, ID_MATERIAL_COUNT)) {
			numMtls = GetInt();
		}
		else if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
	} while (level > 0);
	
	return TRUE;
}

Mtl* AsciiImp::GetMaterial()
{
	int level = 0;
	TCHAR* token;
	Mtl* mtl = NULL;
	TSTR name;
	
	do {
		token = GetToken();
		if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		else if (Compare(token, ID_MATNAME)) {
			name = GetString();
		}
		else if (Compare(token, ID_MATCLASS)) {
			TSTR texClass = GetString();
			if (Compare(texClass, _T("Standard"))) {
				GetStandardMtl(mtl);
				level--;
			}
			if (Compare(texClass, _T("Standard (Legacy)"))) {
				GetStandardMtl(mtl);
				level--;
			}
			else if (Compare(texClass, _T("Multi/Sub-Object"))) {
				GetMultiSubObjMtl(mtl);
				level--;
			}
			else if (Compare(texClass, _T("Shell Material"))) {
				GetShellMtl(mtl);
				level--;
			}
			else
			{
				//Error
				ERRORBOX( "Material Class [%s] Not Supported by Importer", texClass );
			}
		}
		else if (Compare(token, ID_AMBIENT)) {
			GetPoint3();
		}
		else if (Compare(token, ID_DIFFUSE)) {
			GetPoint3();
		}
		else if (Compare(token, ID_SPECULAR)) {
			GetPoint3();
		}
		else if (Compare(token, ID_SHINE)) {
			GetFloat();
		}
		else if (Compare(token, ID_SHINE_STRENGTH)) {
			GetFloat();
		}
		else if (Compare(token, ID_TRANSPARENCY)) {
			//Fix in Extended version
			//Parent material doesn't have opacity option so do nothing!
			GetFloat();
		}
		else if (Compare(token, ID_TWOSIDED)) {
			//Fix in Extended version
			//Parent material doesn't have twosided option so do nothing!
		}
		else if (Compare(token, ID_WIRESIZE)) {
			GetFloat();
		}
	} while (level != 0);
	
	if (mtl) {
		mtl->SetName(name);
	}
	
	return mtl;
}

BOOL AsciiImp::GetStandardMtl(Mtl*& mtl)
{
	int level = 1;
	TCHAR* token;
	StdMat* stdMtl;
	
	mtl = stdMtl = NewDefaultStdMat();
	
	do {
		token = GetToken();
		if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		else if (Compare(token, ID_MATNAME)) {
			stdMtl->SetName(GetString());
		}
		else if (Compare(token, ID_AMBIENT)) {
			stdMtl->SetAmbient(GetPoint3(), 0);
		}
		else if (Compare(token, ID_DIFFUSE)) {
			stdMtl->SetDiffuse(GetPoint3(), 0);
		}
		else if (Compare(token, ID_SPECULAR)) {
			stdMtl->SetSpecular(GetPoint3(), 0);
		}
		else if (Compare(token, ID_SHINE)) {
			stdMtl->SetShininess(GetFloat(), 0);
		}
		else if (Compare(token, ID_SHINE_STRENGTH)) {
			stdMtl->SetShinStr(GetFloat(), 0);
		}
		else if (Compare(token, ID_TRANSPARENCY)) {
			//Fix in Extended version
			stdMtl->SetOpacity(1.0f - GetFloat(), 0);
		}
		else if (Compare(token, ID_TWOSIDED)) {
			//Fix in Extended version
			stdMtl->SetTwoSided(TRUE);
		}
		else if (Compare(token, ID_SELFILLUM)) {
			//Fix in Extended version
			stdMtl->SetSelfIllum(GetFloat(), 0);
		}
		else if (Compare(token, ID_WIRESIZE)) {
			stdMtl->SetWireSize(GetFloat(), 0);
		}
		else if (Compare(token, ID_MAP_AMBIENT)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				stdMtl->SetSubTexmap(ID_AM, tex);
				stdMtl->SetTexmapAmt(ID_AM, amount, 0);
				stdMtl->EnableMap(ID_AM, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_DIFFUSE)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				MSTR className;
				tex->GetClassName( className );

				if( autoOpacityMap )
				{
					if( lstrcmpW( className, L"Bitmap" ) == 0 )
					{
						BitmapTex* bitmapTex = (BitmapTex*)tex;
						Bitmap* bitmap = bitmapTex->GetBitmap( 0 );

						bool hasAlphaChannel = false;

						if( bitmap )
						{
							//Check if have alpha channel
							for( int i = 0; i < bitmap->GetBitmapInfo().Width(); i++ )
							{
								if( hasAlphaChannel )
									break;

								for( int j = 0; j < bitmap->GetBitmapInfo().Height(); j++ )
								{
									BMM_Color_64 pixel;
									bitmap->GetPixels( i, j, 1, &pixel );

									if( pixel.a < 255 )
									{
										hasAlphaChannel = true;
										break;
									}
								}
							}

							if( hasAlphaChannel )
							{
								bitmapTex->SetAlphaAsMono( TRUE );
								stdMtl->SetSubTexmap( ID_OP, tex );
								stdMtl->SetTexmapAmt( ID_OP, amount, 0 );
								stdMtl->EnableMap( ID_OP, TRUE );
							}
						}
					}

					tex->ActivateTexDisplay( TRUE );
					tex->SetMtlFlag( MTL_TEX_DISPLAY_ENABLED, TRUE );
				}

				stdMtl->ActivateTexDisplay( TRUE );
				stdMtl->SetMtlFlag( MTL_TEX_DISPLAY_ENABLED, TRUE );
				stdMtl->SetSubTexmap(ID_DI, tex);
				stdMtl->SetTexmapAmt(ID_DI, amount, 0);
				stdMtl->EnableMap(ID_DI, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_SPECULAR)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				stdMtl->SetSubTexmap(ID_SP, tex);
				stdMtl->SetTexmapAmt(ID_SP, amount, 0);
				stdMtl->EnableMap(ID_SP, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_SHINE)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				stdMtl->SetSubTexmap(ID_SH, tex);
				stdMtl->SetTexmapAmt(ID_SH, amount, 0);
				stdMtl->EnableMap(ID_SH, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_SHINESTRENGTH)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				stdMtl->SetSubTexmap(ID_SS, tex);
				stdMtl->SetTexmapAmt(ID_SS, amount, 0);
				stdMtl->EnableMap(ID_SS, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_SELFILLUM)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				BitmapTex* bitmapTex = (BitmapTex*)tex;

				if( bitmapTex->GetUVGen() && forceMappingChannel )
					bitmapTex->GetUVGen()->SetMapChannel( 2 );

				stdMtl->SetSubTexmap(ID_SI, tex);
				stdMtl->SetTexmapAmt(ID_SI, amount, 0);
				stdMtl->EnableMap(ID_SI, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_OPACITY)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				BitmapTex* bitmapTex = (BitmapTex*)tex;

				bitmapTex->SetAlphaAsMono( TRUE );
				stdMtl->SetSubTexmap(ID_OP, tex);
				stdMtl->SetTexmapAmt(ID_OP, amount, 0);
				stdMtl->EnableMap(ID_OP, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_FILTERCOLOR)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				stdMtl->SetSubTexmap(ID_FI, tex);
				stdMtl->SetTexmapAmt(ID_FI, amount, 0);
				stdMtl->EnableMap(ID_FI, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_BUMP)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				stdMtl->SetSubTexmap(ID_BU, tex);
				stdMtl->SetTexmapAmt(ID_BU, amount, 0);
				stdMtl->EnableMap(ID_BU, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_REFLECT)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				stdMtl->SetSubTexmap(ID_RL, tex);
				stdMtl->SetTexmapAmt(ID_RL, amount, 0);
				stdMtl->EnableMap(ID_RL, TRUE);
			}
		}
		else if (Compare(token, ID_MAP_REFRACT)) {
			float amount = 0.0f;
			Texmap* tex = GetTexture(amount);
			if (tex) {
				stdMtl->SetSubTexmap(ID_RR, tex);
				stdMtl->SetTexmapAmt(ID_RR, amount, 0);
				stdMtl->EnableMap(ID_RR, TRUE);
			}
		}
	} while (level != 0);
	
	return TRUE;
}

BOOL AsciiImp::GetMultiSubObjMtl(Mtl*& mtl)
{
	int level = 1;
	TCHAR* token;
	MultiMtl *multiMat;
	
	mtl = multiMat = NewDefaultMultiMtl();
	
	do {
		token = GetToken();
		if (Compare(token, L"{")) {
			level++;
		}
		else if (Compare(token, L"}")) {
			level--;
		}
		else if (Compare(token, ID_NUMSUBMTLS)) {
			multiMat->SetNumSubMtls(GetInt());
		}
		else if (Compare(token, ID_SUBMATERIAL)) {
			int mtlNum = GetInt();
			Mtl* subMtl = GetMaterial();
			if (subMtl) {
				multiMat->SetSubMtl(mtlNum, subMtl);
			}
		}
	} while (level != 0);
	
	return TRUE;
}

//Added by Sandurr
BOOL AsciiImp::GetShellMtl(Mtl*& mtl)
{
	int level = 1;
	TCHAR* token;
	MultiMtl *multiMat;
	
	mtl = multiMat = NewDefaultMultiMtl(); //We can't make BAKE_SHELL_CLASS_ID materials.. so we just use Multi Object..
	
	do {
		token = GetToken();
		if (Compare(token, L"{")) {
			level++;
		}
		else if (Compare(token, L"}")) {
			level--;
		}
		else if (Compare(token, ID_NUMSUBMTLS)) {
			multiMat->SetNumSubMtls(GetInt());
		}
		else if (Compare(token, ID_SUBMATERIAL)) {
			int mtlNum = GetInt();
			Mtl* subMtl = GetMaterial();
			if (subMtl) {
				multiMat->SetSubMtl(mtlNum, subMtl);
			}
		}
	} while (level != 0);
	
	return TRUE;
}


//***************************************************************************
//
// Texture Import
//
//***************************************************************************

Texmap* AsciiImp::GetTexture(float& amount)
{
	int level = 0;
	TCHAR* token;
	Texmap* tex = NULL;
	TSTR name;
	
	do {
		token = GetToken();
		if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		else if (Compare(token, ID_TEXNAME)) {
			name = GetString();
		}
		else if (Compare(token, ID_TEXAMOUNT)) {
			amount = GetFloat();
		}
		else if (Compare(token, ID_TEXCLASS)) {
			TSTR texClass = GetString();
			if (Compare(texClass, _T("Bitmap"))) {
				GetBitmapTexture(tex, amount);
				level--;
			}
			else if (Compare(texClass, _T("Mask"))) {
				GetMaskTexture(tex, amount);
			}
			else if (Compare(texClass, _T("RGB Tint"))) {
				GetRGBTintTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Checker"))) {
				GetCheckerTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Mix"))) {
				GetMixTexture(tex, amount);
				level--;
			}
			else if (Compare(texClass, _T("Marble"))) {
				GetMarbleTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Noise"))) {
				GetNoiseTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Reflect/Refract"))) {
				GetReflectRefractTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Flat Mirror"))) {
				GetFlatMirrorTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Gradient"))) {
				GetGradientTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Composite"))) {
				GetCompositeTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Wood"))) {
				GetWoodTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Dent"))) {
				GetDentTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Cellular"))) {
				GetCellularTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Falloff"))) {
				GetFalloffTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Output"))) {
				GetOutputTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Particle Age"))) {
				GetParticleAgeTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Particle MBlur"))) {
				GetParticleMBlurTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Perlin Marble"))) {
				GetPerlinMarbleTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Planet"))) {
				GetPlanetTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Raytrace"))) {
				GetRaytraceTexture(tex, amount);
			}
			else if (Compare(texClass, _T("RGB Multiply"))) {
				GetRGBMultTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Smoke"))) {
				GetSmokeTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Speckle"))) {
				GetSpeckleTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Splat"))) {
				GetSplatTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Stucco"))) {
				GetStuccoTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Thin Wall Refraction"))) {
				GetThinWallRefractTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Vertex Color"))) {
				GetVertexColorTexture(tex, amount);
			}
			else if (Compare(texClass, _T("Water"))) {
				GetWaterTexture(tex, amount);
			}
		}
	} while (level != 0);
	
	if (tex) {
		tex->SetName(name);
	}
	
	return tex;
}

BOOL AsciiImp::GetMaskTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(MASK_CLASS_ID, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetRGBTintTexture(Texmap*& tex, float& amount)
{
	tex = NewDefaultTintTex();
	return TRUE;
}

BOOL AsciiImp::GetCheckerTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(CHECKER_CLASS_ID, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetMixTexture(Texmap*& tex, float& amount)
{
	tex = NewDefaultMixTex();

	int level = 1;
	TCHAR* token;
	int subNo = 0;
	
	do {
		token = GetToken();
		if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		else if (Compare(token, ID_TEXAMOUNT)) {
			amount = GetFloat();
		}
		else if (Compare(token, ID_MAP_GENERIC)) {
			float tempAmount = 0.0f;
			Texmap* newTex = GetTexture(tempAmount);

			if (newTex) {
				tex->SetSubTexmap(subNo, newTex);
				subNo++;
			}
		}
	} while (level != 0);

	return TRUE;
}

BOOL AsciiImp::GetMarbleTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(MARBLE_CLASS_ID, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetNoiseTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(NOISE_CLASS_ID, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetReflectRefractTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(ACUBIC_CLASS_ID, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetFlatMirrorTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(MIRROR_CLASS_ID, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetGradientTexture(Texmap*& tex, float& amount)
{
	tex = NewDefaultGradTex();
	return TRUE;
}

BOOL AsciiImp::GetCompositeTexture(Texmap*& tex, float& amount)
{
	tex = NewDefaultCompositeTex();
	return TRUE;
}

BOOL AsciiImp::GetWoodTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x0000214, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetDentTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x0000218, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetCellularTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0xc90017a5,0x111940bb));
	return TRUE;
}

BOOL AsciiImp::GetFalloffTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(FALLOFF_CLASS_ID, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetOutputTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(OUTPUT_CLASS_ID, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetParticleAgeTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x8d618ea4, 0x49bbe8cf));
	return TRUE;
}

BOOL AsciiImp::GetParticleMBlurTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x8a746be5, 0x81163ef6));
	return TRUE;
}

BOOL AsciiImp::GetPerlinMarbleTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x23ad0ae9, 0x158d7a88));
	return TRUE;
}

BOOL AsciiImp::GetPlanetTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x46396cf1, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetRaytraceTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x00, 0x00)); // TBD
	return TRUE;
}

BOOL AsciiImp::GetRGBMultTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(RGBMULT_CLASS_ID, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetSmokeTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0xa845e7c, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetSpeckleTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x62c32b8a, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetSplatTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x90b04f9, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetStuccoTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x9312fbe, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetThinWallRefractTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0xd1f5a804, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetVertexColorTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x0934851, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetWaterTexture(Texmap*& tex, float& amount)
{
	tex = (Texmap*)CreateInstance(TEXMAP_CLASS_ID, Class_ID(0x7712634e, 0x00));
	return TRUE;
}

BOOL AsciiImp::GetBitmapTexture(Texmap*& tex, float& amount)
{
	int level = 1;
	TCHAR* token;
	BitmapTex* bmtex;
	
	tex = bmtex = (BitmapTex*)NewDefaultBitmapTex();

	do {
		token = GetToken();
		if (Compare(token, L"{"))
			level++;
		else if (Compare(token, L"}"))
			level--;
		else if (Compare(token, ID_BITMAP)) 
		{
			std::wstring originalTextureFile = GetString().data();

			FILE* pFile = nullptr;
			_wfopen_s( &pFile, originalTextureFile.c_str(), L"rb" );

			if( pFile )
			{
				bmtex->SetMapName( originalTextureFile.c_str() );
				fclose( pFile );
			}
			else
			{
				originalTextureFile = originalTextureFile.substr( originalTextureFile.find_last_of( '\\' ) + 1 );
				bmtex->SetMapName( std::wstring( baseFilePath + originalTextureFile ).c_str() );
			}
		}
		else if (Compare(token, ID_TEXAMOUNT)) {
			amount = GetFloat();
		}
		else if (Compare(token, ID_MAPTYPE)) {
			token = GetToken();
			/* TBD
			if (Compare(token, _T("Explicit")))
			bmtex->GetUVGen()->SetCoordMapping(UVMAP_EXPLICIT);
			else if (Compare(token, _T("Spherical")))
			bmtex->GetUVGen()->SetCoordMapping(UVMAP_SPHERE_ENV);
			else if (Compare(token, _T("Cylindrical")))
			bmtex->GetUVGen()->SetCoordMapping(UVMAP_CYL_ENV);
			else if (Compare(token, _T("Shrinkwrap")))
			bmtex->GetUVGen()->SetCoordMapping(UVMAP_SHRINK_ENV);
			else if (Compare(token, _T("Screen")))
			bmtex->GetUVGen()->SetCoordMapping(UVMAP_SCREEN_ENV);
			*/
		}
		else if (Compare(token, ID_U_OFFSET)) {
			bmtex->GetUVGen()->SetUOffs(GetFloat(), 0);
		}
		else if (Compare(token, ID_V_OFFSET)) {
			bmtex->GetUVGen()->SetVOffs(GetFloat(), 0);
		}
		else if (Compare(token, ID_U_TILING)) {
			bmtex->GetUVGen()->SetUScl(GetFloat(), 0);
		}
		else if (Compare(token, ID_V_TILING)) {
			bmtex->GetUVGen()->SetVScl(GetFloat(), 0);
		}
		else if (Compare(token, ID_ANGLE)) {
			bmtex->GetUVGen()->SetAng(GetFloat(), 0);
		}
		else if (Compare(token, ID_BLUR)) {
			bmtex->GetUVGen()->SetBlur(GetFloat(), 0);
		}
		else if (Compare(token, ID_BLUR_OFFSET)) {
			bmtex->GetUVGen()->SetBlurOffs(GetFloat(), 0);
		}
		else if (Compare(token, ID_NOISE_AMT)) {
			bmtex->GetUVGen()->SetNoiseAmt(GetFloat(), 0);
		}
		else if (Compare(token, ID_NOISE_SIZE)) {
			bmtex->GetUVGen()->SetNoiseSize(GetFloat(), 0);
		}
		else if (Compare(token, ID_NOISE_LEVEL)) {
			bmtex->GetUVGen()->SetNoiseLev(GetInt(), 0);
		}
		else if (Compare(token, ID_NOISE_PHASE)) {
			bmtex->GetUVGen()->SetNoisePhs(GetFloat(), 0);
		}
	} while (level != 0);
	
	return TRUE;
}
