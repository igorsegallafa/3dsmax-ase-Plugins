//************************************************************************** 
//* Asciiexp.cpp	- Ascii File Exporter
//* 
//* By Christer Janson
//* Kinetix Development
//*
//* January 20, 1997 CCJ Initial coding
//*
//* This module contains the DLL startup functions
//*
//* Copyright (c) 1997, All Rights Reserved. 
//***************************************************************************
#include "stdafx.h"
#include "asciiexp.h"

HINSTANCE hInstance;
int controlsInit = FALSE;

static BOOL showPrompts;
static BOOL exportSelected;

// Class ID. These must be unique and randomly generated!!
// If you use this as a sample project, this is the first thing
// you should change!
#define ASCIIEXP_CLASS_ID	Class_ID(0x85548e0b, 0x4a26450c)

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved) 
{
	hInstance = hinstDLL;

	// Initialize the custom controls. This should be done only once.
	if (!controlsInit) {
		controlsInit = TRUE;
#if MAX_API_NUM < 38 //MAX_API_NUM_R140
		InitCustomControls(hInstance);
#endif
		InitCommonControls();
	}
	
	return (TRUE);
}

__declspec( dllexport ) const TCHAR* LibDescription() 
{
	return GetString(IDS_LIBDESCRIPTION);
}

/// MUST CHANGE THIS NUMBER WHEN ADD NEW CLASS 
__declspec( dllexport ) int LibNumberClasses() 
{
	return 1;
}

__declspec( dllexport ) ClassDesc* LibClassDesc(int i) 
{
	switch(i) {
	case 0: return GetAsciiExpDesc();
	default: return 0;
	}
}

__declspec( dllexport ) ULONG LibVersion() 
{
	return VERSION_3DSMAX;
}

__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}

class AsciiExpClassDesc:public ClassDesc {
public:
	int				IsPublic() { return 1; }
	void*			Create(BOOL loading = FALSE) { return new AsciiExp; } 
	const TCHAR*	ClassName() { return GetString(IDS_ASCIIEXP); }
	SClass_ID		SuperClassID() { return SCENE_EXPORT_CLASS_ID; } 
	Class_ID		ClassID() { return ASCIIEXP_CLASS_ID; }
	const TCHAR*	Category() { return GetString(IDS_CATEGORY); }
};

static AsciiExpClassDesc AsciiExpDesc;

ClassDesc* GetAsciiExpDesc()
{
	return &AsciiExpDesc;
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;

	return NULL;
}

AsciiExp::AsciiExp()
{
	// These are the default values that will be active when 
	// the exporter is ran the first time.
	// After the first session these options are sticky.
	bIncludeMesh = TRUE;
	bIncludeAnim = TRUE;
	bIncludeMtl =  TRUE;
	bIncludeMeshAnim =  FALSE;
	bIncludeCamLightAnim = FALSE;
	bIncludeIKJoints = FALSE;
	bIncludePhysique = TRUE;
	bIncludePhysiqueAsSkin = TRUE;
	bGameMode = FALSE;
	bBlendWeight = FALSE;
	bBakeObjects = FALSE;
	bLightningMap = FALSE;
	bIncludeNormals  =  FALSE;
	bIncludeTextureCoords = FALSE;
	bIncludeVertexColors = FALSE;
	bIncludeObjGeom = TRUE;
	bIncludeObjShape = TRUE;
	bIncludeObjCamera = TRUE;
	bIncludeObjLight = TRUE;
	bIncludeObjHelper = TRUE;
	bAlwaysSample = FALSE;
	nKeyFrameStep = 5;
	nMeshFrameStep = 5;
	nPrecision = 4;
	nStaticFrame = 0;
}

AsciiExp::~AsciiExp()
{
}

// Added to Export Physique Data From Skin info
// Find Skin Modifier in Node
Modifier* AsciiExp::FindSkinModifier(INode *pNode)
{
	// Get object from node. Abort if no object.
	Object* pObj = pNode->GetObjectRef();

	if (!pObj) return NULL;

	// Is derived object ?
	while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject* pDerObj = static_cast<IDerivedObject*>(pObj);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < pDerObj->NumModifiers())
		{
			// Get current modifier.
			Modifier* mod = pDerObj->GetModifier(ModStackIndex);

			// Is this Skin ?
			if (mod->ClassID() == SKIN_CLASSID )
			{
				// Yes -> Exit.
				return mod;
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
		pObj = pDerObj->GetObjRef();
	}

	// Not found.
	return NULL;
}

// Added to Export Physique Data.
// Find Physique Modifier in Node
Modifier* AsciiExp::FindPhysiqueModifier(INode *pNode)
{
	// Get object from node. Abort if no object.
	Object* ObjectPtr = pNode->GetObjectRef();

	if (!ObjectPtr) return NULL;

	// Is derived object ?
	while (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID && ObjectPtr)
	{
		// Yes -> Cast.
		IDerivedObject *DerivedObjectPtr = (IDerivedObject *)(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);

			// Is this Physique ?
			if (ModifierPtr->ClassID() == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B))
			{
				ModContext * mc = DerivedObjectPtr->GetModContext(ModStackIndex);

				// Yes -> Exit.
				return ModifierPtr;
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
		ObjectPtr = DerivedObjectPtr->GetObjRef();
	}

	// Not found.
	return NULL;
}

void AsciiExp::ExportPhysiqueDataFromSkin(INode *pNode, Modifier *pMod, int indentLevel)
{
	//**************************************************************************
	//Get the data from the skin interface
	//**************************************************************************
	//By Sandurr

	TSTR indent;
	indent = GetIndent(indentLevel+1);

	ISkin *skin = (ISkin *) pMod->GetInterface(I_SKIN);
	if (!skin){
		ERRORBOX("ISkin null");
		return;
	}
		
	ISkinContextData *skinMC = skin->GetContextInterface(pNode);
	if(!skinMC){
		ERRORBOX("ISkinContextData null");
		return;
	}

	ISkinImportData * skinImp = (ISkinImportData*)pMod->GetInterface(I_SKINIMPORTDATA);
	if(!skinImp) {
		ERRORBOX("ISkinImportData null");
		return;
	}

	fwprintf(pStream,L"%s%s {\n",indent.data(),  ID_PHYSIQUE);
	fwprintf(pStream,L"%s\t%s %d\n",indent.data(), ID_PHYSIQUE_NUM_VERTASSINE, skinMC->GetNumPoints());

	bool ignoreblended = false;
	for(int i=0;i<skinMC->GetNumPoints();i++)
	{
		Tab<INode*> BoneImp;
		Tab<float> WeightImp;
		
		for(int j = 0 ; j < skinMC->GetNumAssignedBones(i) ; j++)
		{
			INode* bone = skin->GetBone(skinMC->GetAssignedBone(i,j));
			float weight = skinMC->GetBoneWeight(i,j);

			BoneImp.Append(1,&bone);
			WeightImp.Append(1,&weight);
		}

		INode * pChosenBone = NULL;

		if( BoneImp.Count() == 1 )
		{
			pChosenBone = BoneImp[0];
		}
		else
		{
			if( !ignoreblended )
			{
				if( MessageBoxA( 0, "The Skin is blended! The exporter does not support blended vertices.\n\nDo you want to let the exporter choose the most convenient bone, based on the weight, for each vertex?\n\nNote: the exporter will edit bone weights of the Skin", "Skin is Blended", MB_YESNO | MB_ICONEXCLAMATION ) == IDYES )
				{
					//We want to automatically make the bone weights non blended rigid
					ignoreblended = true;
				}
				else
				{
					ERRORBOX( "Can't continue with Skin Export" );
					break;
				}
			}

			INode * biggestBone = NULL;
			float biggestWeight = -1.0f;
			for( int k = 0; k < BoneImp.Count(); k++ )
			{
				float weight = WeightImp[k];

				if( weight > biggestWeight )
				{
					biggestBone = BoneImp[k];
					biggestWeight = weight;
					continue;
				}
			}

			Tab<INode*> newBone;
			Tab<float> newWeight;

			if( biggestBone )
			{
				biggestWeight = 1.0f;

				newBone.Append(1,&biggestBone);
				newWeight.Append(1,&biggestWeight);
			}

			if( newBone.Count() == 1 )
			{
				pChosenBone = newBone[0];

				if( !skinImp->AddWeights( pNode, i, newBone, newWeight ) )
				{
					ERRORBOX( "AddWeights failed at %d (%s,%0.2f)", i, newBone[0]->GetName(), newWeight[0] );
				}
			}
		}

		if( pChosenBone )
		{
			//Just export it
			fwprintf(pStream,L"%s\t%s %d\t\"%s\"\n",indent.data(),ID_PHYSIQUE_NOBLEND_RIGID, i, pChosenBone->GetName());
		}
		else
		{
			ERRORBOX( "Error pChosenBone is NULL" );
			break;
		}

		//Blended Rigid is impossible
		/*
		else 
		{
			//Error, can't do blended
			ERRORBOX( "Vertex # %d is blended [Bones: %d]. Physique does not support blended vertices!\n\nMake the blended vertices non blended and try again!", i, BoneImp.Count() );
			break;

			vertImp = phyContImp->SetVertexInterface(i,RIGID_BLENDED_TYPE);
			if(vertImp == NULL)
				return false;
			IPhyBlendedRigidVertexImport * blendedrigidImp = (IPhyBlendedRigidVertexImport*)vertImp;

			BOOL FirstTime = TRUE;
			for(int j=0;j<BoneImp.Count();j++)
			{
				//need the flag "FirstTime" as physique adds subsequent waits to the first
				blendedrigidImp->SetWeightedNode(BoneImp[j],WeightImp[j],FirstTime);
				FirstTime = FALSE;
			}
		}
		*/
	}

	fwprintf(pStream,L"%s}\n",indent.data());
}
void AsciiExp::ExportPhysiqueDataFromSkinNew( INode * pNode, Modifier * pMod, int indentLevel )
{
    TSTR indent;
    indent = GetIndent( indentLevel + 1 );

    ISkin * skin = (ISkin *)pMod->GetInterface( I_SKIN );

    ISkinContextData * skin_data = (ISkinContextData *)skin->GetContextInterface( pNode );

    Object * obj = pNode->EvalWorldState( ip->GetTime() ).obj;

    TriObject * tri = (TriObject *)obj->ConvertToType( ip->GetTime(), Class_ID( TRIOBJ_CLASS_ID, 0 ) );
    Mesh * mesh = &tri->GetMesh();

    int numverts = mesh->numVerts;

    fwprintf( pStream, L"%s%s {\n", indent.data(), ID_PHYSIQUE );
    fwprintf( pStream, L"%s\t%s %d\n", indent.data(), ID_PHYSIQUE_NUM_VERTASSINE, skin_data->GetNumPoints() );

    for ( int i = 0; i < numverts; i++ )
    {
        float totalWeight = 0.0f, weight = 0.0f;
        TSTR nodeName;

        int numWeights = skin_data->GetNumAssignedBones( i );

        fwprintf( pStream, L"%s\t%s %d {\n", indent.data(), ID_PHYSIQUE_BLEND_RIGID, i );
        fwprintf( pStream, L"%s\t\t%s %d\n", indent.data(), ID_PHYSIQUE_NUM_NODASSINE, numWeights );
        fwprintf( pStream, L"%s\t\t%s {\n", indent.data(), ID_PHYSIQUE_NODE_LIST );


		struct BoneBiggestWeight
		{
			int iIndex;
			std::wstring strBoneName;
			float fWeight;


			BoneBiggestWeight( int idx, std::wstring strname, float fw )
			{
				strBoneName = strname;
				fWeight = fw;
				iIndex = idx;
			};

			BoneBiggestWeight() {};
			~BoneBiggestWeight() {};
		};

		std::vector<BoneBiggestWeight> vWeights;

        for ( int j = 0; j < numWeights; j++ )
        {
			int iBoneIndex = skin_data->GetAssignedBone( i, j );
            INode * pBone = skin->GetBone( iBoneIndex );
            weight = skin_data->GetBoneWeight( i, j );
            if ( weight == 0.000000 ) continue;
            nodeName = pBone->GetName();

			vWeights.push_back( BoneBiggestWeight( iBoneIndex, std::wstring(nodeName), weight ) );

			//fwprintf( pStream, L"%s\t\t\t%s %d\t%0.6f\t\"%s\"\n", indent.data(), ID_PHYSIQUE_NODE, j, weight, nodeName );
        }

		/*
        std::sort( vWeights.begin(), vWeights.end(),
                   []( const BoneBiggestWeight & a, const BoneBiggestWeight & b ) -> bool
        {
            return a.fWeight > b.fWeight;
        } );
		*/
		for ( size_t j = 0; j < vWeights.size(); j++ )
			fwprintf( pStream, L"%s\t\t\t%s %d\t%0.6f\t\"%s\"\n", indent.data(), ID_PHYSIQUE_NODE, j, vWeights[j].fWeight, vWeights[j].strBoneName.c_str() );

        fwprintf( pStream, L"%s\t\t}\n", indent.data() );
        fwprintf( pStream, L"%s\t}\n", indent.data() );
    }
	fwprintf( pStream, L"\t}\n" );
    pMod->ReleaseInterface( I_SKIN, skin );
}



void AsciiExp::ExportPhysiqueData(INode *pNode, Modifier *pMod, int indentLevel)
{
	//**************************************************************************
	//Get the data from the physique interface
	//**************************************************************************
	//By Sandurr

	TSTR indent;
	indent = GetIndent(indentLevel+1);

	//get a pointer to the export interface
	IPhysiqueExport *phyExport = (IPhysiqueExport *)pMod->GetInterface(I_PHYEXPORT);

	//get the physique version number.  
	//If the version number is > 30 you may have floating bones
	int ver = phyExport->Version();

	//get a pointer to the export context interface
	IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(pNode);

	//convert to rigid for time independent vertex assignment
	mcExport->ConvertToRigid(true);

	//allow blending to export multi-link assignments
	mcExport->AllowBlending(true);

	//these are the different types of rigid vertices supported
	IPhyRigidVertex *r_vtx;
	IPhyBlendedRigidVertex *rb_vtx;

	//get the vertex count from the export interface
	int numverts = mcExport->GetNumberVertices();

	// write header and number of vertex count
	fwprintf(pStream,L"%s%s {\n",indent.data(),  ID_PHYSIQUE);
	fwprintf(pStream,L"%s\t%s %d\n",indent.data(), ID_PHYSIQUE_NUM_VERTASSINE, numverts);
	
	bool ignoreblended = false;
	bool exit = false;
	//gather the vertex-link assignment data
	for (int i = 0; i<numverts; i++)
	{
		//Get the hierarchial vertex interface for this vertex
		IPhyVertexExport* vi = mcExport->GetVertexInterface(i);//, HIERARCHIAL_VERTEX);
		if (vi)
		{
			//check the vertex type and process accordingly 
			int type = vi->GetVertexType();
			switch (type)
			{
				//we have a vertex assigned to more than one link
				case RIGID_BLENDED_TYPE: 
				{
					if( !ignoreblended )
					{
						if( MessageBoxA( 0, "The Physique is blended! The exporter does not support blended vertices.\n\nDo you want to let the exporter choose the most convenient bone, based on the weight, for each vertex?", "Physique is Blended", MB_YESNO | MB_ICONEXCLAMATION ) == IDYES )
						{
							//We want to choose the most convenient bone based on the highest weight
							ignoreblended = true;
						}
						else
						{
							exit = true;
							ERRORBOX( "Can't continue with Physique Export" );
							break;
						}
					}

					//type-cast the pNode to the proper class  
					rb_vtx = (IPhyBlendedRigidVertex*)vi;
					int numNode = rb_vtx->GetNumberNodes();

					if (numNode <= 0)
					{
						ERRORBOX( "Error Vertex # %d has blended rigid but no Nodes? Must exit", i );
						exit = true;
						break;
					}

					TSTR nodeName;
					float highestWeight = -1.0f;
					//iterate through all links this vertex is assigned to
					for (int x = 0; x<numNode; x++)
					{
						//Get the pNode and its name for export
						TSTR name = rb_vtx->GetNode(x)->GetName();
						//Get the weight
						float weight = rb_vtx->GetWeight(x);;

						if (weight < 0.0f)
						{
							ERRORBOX( "Error Node %s has negative weight of %0.2f\n\nMust exit", name, weight );
							exit = true;
							break;
						}

						if (weight > highestWeight)
						{
							highestWeight = weight;
							nodeName = name;
						}
					}

					if (exit) 
						break;
					
					fwprintf(pStream,L"%s\t%s %d\t\"%s\"\n",indent.data(),ID_PHYSIQUE_NOBLEND_RIGID, i, nodeName.data());
				}
				break;
				case RIGID_TYPE:
				{
					//type-cast the pNode to the proper class
					r_vtx = (IPhyRigidVertex*)vi;
    
					//get the pNode and its name for export
					INode* pBone = r_vtx->GetNode();
					TSTR nodeName = pBone->GetName();

					//Export it!
					fwprintf(pStream,L"%s\t%s %d\t\"%s\"\n",indent.data(),ID_PHYSIQUE_NOBLEND_RIGID, i, nodeName.data());
				}
				break;
			}
			//release the vertex interface
			mcExport->ReleaseVertexInterface(vi);
		}

		if( exit )
			break;
	}

	//release the context interface
	phyExport->ReleaseContextInterface(mcExport);

	//Release the physique interface
	pMod->ReleaseInterface(I_PHYINTERFACE, phyExport);
	
	fwprintf(pStream,L"%s}\n",indent.data());
}


int AsciiExp::ExtCount()
{
	return 1;
}

const TCHAR * AsciiExp::Ext(int n)
{
	switch(n) {
	case 0:
		// This cause a static string buffer overwrite
		// return GetString(IDS_EXTENSION1);
		return _T("ASE");
	}
	return _T("");
}

const TCHAR * AsciiExp::LongDesc()
{
	return GetString(IDS_LONGDESC);
}

const TCHAR * AsciiExp::ShortDesc()
{
	return GetString(IDS_SHORTDESC);
}

const TCHAR * AsciiExp::AuthorName() 
{
	return _T("Christer Janson");
}

const TCHAR * AsciiExp::CopyrightMessage() 
{
	return GetString(IDS_COPYRIGHT);
}

const TCHAR * AsciiExp::OtherMessage1() 
{
	return _T("");
}

const TCHAR * AsciiExp::OtherMessage2() 
{
	return _T("");
}

unsigned int AsciiExp::Version()
{
	return 100;
}

static INT_PTR CALLBACK AboutBoxDlgProc(HWND hWnd, UINT msg, 
	WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		CenterWindow(hWnd, GetParent(hWnd)); 
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			EndDialog(hWnd, 1);
			break;
		}
		break;
		default:
			return FALSE;
	}
	return TRUE;
}       

void AsciiExp::ShowAbout(HWND hWnd)
{
}


// Dialog proc
static INT_PTR CALLBACK ExportDlgProc(HWND hWnd, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	Interval animRange;
	ISpinnerControl  *spin;

	AsciiExp *exp = (AsciiExp*)GetWindowLongPtr(hWnd,GWLP_USERDATA); 
	switch (msg) {
	case WM_INITDIALOG:
		exp = (AsciiExp*)lParam;
		SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam); 
		CenterWindow(hWnd, GetParent(hWnd)); 
		CheckDlgButton(hWnd, IDC_MESHDATA, exp->GetIncludeMesh()); 
		CheckDlgButton(hWnd, IDC_ANIMKEYS, exp->GetIncludeAnim()); 
		CheckDlgButton(hWnd, IDC_MATERIAL, exp->GetIncludeMtl());
		CheckDlgButton(hWnd, IDC_MESHANIM, exp->GetIncludeMeshAnim()); 
		CheckDlgButton(hWnd, IDC_CAMLIGHTANIM, exp->GetIncludeCamLightAnim()); 
#ifndef DESIGN_VER
		CheckDlgButton(hWnd, IDC_IKJOINTS, exp->GetIncludeIKJoints()); 
#endif // !DESIGN_VER
		CheckDlgButton(hWnd, IDC_PHYSIQUE, exp->GetIncludePhysique()); 
		CheckDlgButton( hWnd, IDC_PHYSIQUEASSKIN, exp->GetIncludePhysiqueAsSkin() );
		CheckDlgButton( hWnd, IDC_GAMEMODE, exp->GetIsGameMode() );
		CheckDlgButton( hWnd, IDC_BLENDING_PHYSIQUE, exp->GetIsBlendWeight() );
		CheckDlgButton( hWnd, IDC_BAKE_OBJS, exp->GetBakeObjects() );
		CheckDlgButton(hWnd, IDC_NORMALS,  exp->GetIncludeNormals());
		CheckDlgButton(hWnd, IDC_TEXCOORDS,exp->GetIncludeTextureCoords()); 
		CheckDlgButton(hWnd, IDC_VERTEXCOLORS,exp->GetIncludeVertexColors()); 
		CheckDlgButton(hWnd, IDC_OBJ_GEOM,exp->GetIncludeObjGeom()); 
		CheckDlgButton(hWnd, IDC_OBJ_SHAPE,exp->GetIncludeObjShape()); 
		CheckDlgButton(hWnd, IDC_OBJ_CAMERA,exp->GetIncludeObjCamera()); 
		CheckDlgButton(hWnd, IDC_OBJ_LIGHT,exp->GetIncludeObjLight()); 
		CheckDlgButton(hWnd, IDC_OBJ_HELPER,exp->GetIncludeObjHelper());

		// Setup Combo Box
		{
			TCHAR menu[3][12] = { TEXT( "Undefined" ), TEXT( "Static Mesh" ), TEXT( "Terrain" ) };

			for( int i = 0; i < 3; i++ )
				SendMessage( GetDlgItem(hWnd, IDC_COMBO1), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)menu[i] );

			SendMessage( GetDlgItem(hWnd, IDC_COMBO1), CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
		}

		CheckRadioButton(hWnd, IDC_RADIO_USEKEYS, IDC_RADIO_SAMPLE, 
			exp->GetAlwaysSample() ? IDC_RADIO_SAMPLE : IDC_RADIO_USEKEYS);
		
		// Setup the spinner controls for the controller key sample rate 
		spin = GetISpinner(GetDlgItem(hWnd, IDC_CONT_STEP_SPIN)); 
		spin->LinkToEdit(GetDlgItem(hWnd,IDC_CONT_STEP), EDITTYPE_INT ); 
		spin->SetLimits(1, 100, TRUE); 
		spin->SetScale(1.0f);
		spin->SetValue(exp->GetKeyFrameStep() ,FALSE);
		ReleaseISpinner(spin);
		
		// Setup the spinner controls for the mesh definition sample rate 
		spin = GetISpinner(GetDlgItem(hWnd, IDC_MESH_STEP_SPIN)); 
		spin->LinkToEdit(GetDlgItem(hWnd,IDC_MESH_STEP), EDITTYPE_INT ); 
		spin->SetLimits(1, 100, TRUE); 
		spin->SetScale(1.0f);
		spin->SetValue(exp->GetMeshFrameStep() ,FALSE);
		ReleaseISpinner(spin);

		// Setup the spinner controls for the floating point precision 
		spin = GetISpinner(GetDlgItem(hWnd, IDC_PREC_SPIN)); 
		spin->LinkToEdit(GetDlgItem(hWnd,IDC_PREC), EDITTYPE_INT ); 
		spin->SetLimits(1, 10, TRUE); 
		spin->SetScale(1.0f);
		spin->SetValue(exp->GetPrecision() ,FALSE);
		ReleaseISpinner(spin);

		// Setup the spinner control for the static frame#
		// We take the frame 0 as the default value
		animRange = exp->GetInterface()->GetAnimRange();
		spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN)); 
		spin->LinkToEdit(GetDlgItem(hWnd,IDC_STATIC_FRAME), EDITTYPE_INT ); 
		spin->SetLimits(animRange.Start() / GetTicksPerFrame(), animRange.End() / GetTicksPerFrame(), TRUE); 
		spin->SetScale(1.0f);
		spin->SetValue(0, FALSE);
		ReleaseISpinner(spin);

		// Enable / disable mesh options
		EnableWindow(GetDlgItem(hWnd, IDC_NORMALS), exp->GetIncludeMesh());
		EnableWindow(GetDlgItem(hWnd, IDC_TEXCOORDS), exp->GetIncludeMesh());
		EnableWindow(GetDlgItem(hWnd, IDC_VERTEXCOLORS), exp->GetIncludeMesh());
		break;

	case CC_SPINNER_CHANGE:
		spin = (ISpinnerControl*)lParam; 
		break;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
			case CBN_SELCHANGE:
			{
				int ItemIndex = SendMessage( (HWND)lParam, (UINT)CB_GETCURSEL,(WPARAM)0, (LPARAM)0 );

				//Static Mesh
				if( ItemIndex == 1 )
				{
					CheckDlgButton( hWnd, IDC_MESHDATA, TRUE );
					CheckDlgButton( hWnd, IDC_ANIMKEYS, FALSE );
					CheckDlgButton( hWnd, IDC_MATERIAL, TRUE );
					CheckDlgButton( hWnd, IDC_MESHANIM, FALSE );
					CheckDlgButton( hWnd, IDC_CAMLIGHTANIM, FALSE );
#ifndef DESIGN_VER
					CheckDlgButton( hWnd, IDC_IKJOINTS, FALSE );
#endif // !DESIGN_VER
					CheckDlgButton( hWnd, IDC_PHYSIQUE, FALSE );
					CheckDlgButton( hWnd, IDC_PHYSIQUEASSKIN, FALSE );
					CheckDlgButton( hWnd, IDC_GAMEMODE, FALSE );
					CheckDlgButton( hWnd, IDC_BLENDING_PHYSIQUE, FALSE );
					CheckDlgButton( hWnd, IDC_BAKE_OBJS, FALSE );
					CheckDlgButton( hWnd, IDC_NORMALS, FALSE );
					CheckDlgButton( hWnd, IDC_TEXCOORDS, TRUE );
					CheckDlgButton( hWnd, IDC_VERTEXCOLORS, FALSE );
					CheckDlgButton( hWnd, IDC_OBJ_GEOM, TRUE );
					CheckDlgButton( hWnd, IDC_OBJ_SHAPE, FALSE );
					CheckDlgButton( hWnd, IDC_OBJ_CAMERA, FALSE );
					CheckDlgButton( hWnd, IDC_OBJ_LIGHT, FALSE );
					CheckDlgButton( hWnd, IDC_OBJ_HELPER, FALSE );
				}
				//Terrain
				else if( ItemIndex == 2 )
				{
					CheckDlgButton( hWnd, IDC_MESHDATA, TRUE );
					CheckDlgButton( hWnd, IDC_ANIMKEYS, FALSE );
					CheckDlgButton( hWnd, IDC_MATERIAL, TRUE );
					CheckDlgButton( hWnd, IDC_MESHANIM, FALSE );
					CheckDlgButton( hWnd, IDC_CAMLIGHTANIM, FALSE );
#ifndef DESIGN_VER
					CheckDlgButton( hWnd, IDC_IKJOINTS, FALSE );
#endif // !DESIGN_VER
					CheckDlgButton( hWnd, IDC_PHYSIQUE, FALSE );
					CheckDlgButton( hWnd, IDC_PHYSIQUEASSKIN, FALSE );
                    CheckDlgButton( hWnd, IDC_GAMEMODE, FALSE );
                    CheckDlgButton( hWnd, IDC_BLENDING_PHYSIQUE, FALSE );
					CheckDlgButton( hWnd, IDC_BAKE_OBJS, FALSE );
					CheckDlgButton( hWnd, IDC_NORMALS, FALSE );
					CheckDlgButton( hWnd, IDC_TEXCOORDS, TRUE );
					CheckDlgButton( hWnd, IDC_VERTEXCOLORS, TRUE );
					CheckDlgButton( hWnd, IDC_OBJ_GEOM, TRUE );
					CheckDlgButton( hWnd, IDC_OBJ_SHAPE, FALSE );
					CheckDlgButton( hWnd, IDC_OBJ_CAMERA, FALSE );
					CheckDlgButton( hWnd, IDC_OBJ_LIGHT, TRUE );
					CheckDlgButton( hWnd, IDC_OBJ_HELPER, FALSE );
				}
			}
			break;
		}

		switch (LOWORD(wParam)) {
		case IDC_MESHDATA:
			// Enable / disable mesh options
			EnableWindow(GetDlgItem(hWnd, IDC_NORMALS), IsDlgButtonChecked(hWnd,
				IDC_MESHDATA));
			EnableWindow(GetDlgItem(hWnd, IDC_TEXCOORDS), IsDlgButtonChecked(hWnd,
				IDC_MESHDATA));
			EnableWindow(GetDlgItem(hWnd, IDC_VERTEXCOLORS), IsDlgButtonChecked(hWnd,
				IDC_MESHDATA));
			break;
		case IDOK:
			exp->SetIncludeMesh(IsDlgButtonChecked(hWnd, IDC_MESHDATA)); 
			exp->SetIncludeAnim(IsDlgButtonChecked(hWnd, IDC_ANIMKEYS)); 
			exp->SetIncludeMtl(IsDlgButtonChecked(hWnd, IDC_MATERIAL)); 
			exp->SetIncludeMeshAnim(IsDlgButtonChecked(hWnd, IDC_MESHANIM)); 
			exp->SetIncludeCamLightAnim(IsDlgButtonChecked(hWnd, IDC_CAMLIGHTANIM)); 
#ifndef DESIGN_VER
			exp->SetIncludeIKJoints(IsDlgButtonChecked(hWnd, IDC_IKJOINTS)); 
#endif // !DESIGN_VER
			exp->SetIncludePhysique(IsDlgButtonChecked(hWnd, IDC_PHYSIQUE)); 
			exp->SetIncludePhysiqueAsSkin( IsDlgButtonChecked( hWnd, IDC_PHYSIQUEASSKIN ) );
            exp->SetGameMode( IsDlgButtonChecked( hWnd, IDC_GAMEMODE ) );
            exp->SetBlendWeight( IsDlgButtonChecked( hWnd, IDC_BLENDING_PHYSIQUE ) );
			exp->SetBakeObjects( IsDlgButtonChecked( hWnd, IDC_BAKE_OBJS ) );
			exp->SetIncludeNormals(IsDlgButtonChecked(hWnd, IDC_NORMALS));
			exp->SetIncludeTextureCoords(IsDlgButtonChecked(hWnd, IDC_TEXCOORDS)); 
			exp->SetIncludeVertexColors(IsDlgButtonChecked(hWnd, IDC_VERTEXCOLORS)); 
			exp->SetIncludeObjGeom(IsDlgButtonChecked(hWnd, IDC_OBJ_GEOM)); 
			exp->SetIncludeObjShape(IsDlgButtonChecked(hWnd, IDC_OBJ_SHAPE)); 
			exp->SetIncludeObjCamera(IsDlgButtonChecked(hWnd, IDC_OBJ_CAMERA)); 
			exp->SetIncludeObjLight(IsDlgButtonChecked(hWnd, IDC_OBJ_LIGHT)); 
			exp->SetIncludeObjHelper(IsDlgButtonChecked(hWnd, IDC_OBJ_HELPER));
			exp->SetAlwaysSample(IsDlgButtonChecked(hWnd, IDC_RADIO_SAMPLE));
			exp->SetLightningMap( IsDlgButtonChecked( hWnd, IDC_LIGHTNINGMAP ) );

			spin = GetISpinner(GetDlgItem(hWnd, IDC_CONT_STEP_SPIN)); 
			exp->SetKeyFrameStep(spin->GetIVal()); 
			ReleaseISpinner(spin);

			spin = GetISpinner(GetDlgItem(hWnd, IDC_MESH_STEP_SPIN)); 
			exp->SetMeshFrameStep(spin->GetIVal());
			ReleaseISpinner(spin);

			spin = GetISpinner(GetDlgItem(hWnd, IDC_PREC_SPIN)); 
			exp->SetPrecision(spin->GetIVal());
			ReleaseISpinner(spin);
		
			spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN)); 
			exp->SetStaticFrame(spin->GetIVal() * GetTicksPerFrame());
			ReleaseISpinner(spin);
			
			EndDialog(hWnd, 1);
			break;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			break;
		}
		break;
		default:
			return FALSE;
	}
	return TRUE;
}       

// Dummy function for progress bar
DWORD WINAPI fn(LPVOID arg)
{
	return(0);
}

void BakeObjectTransformation()
{
	std::wstring strText = _T( R"PROG(

fn OrgControl PTSel=
(
	fStart = animationRange.start
	fEnd = animationRange.end
	MyP = point()
	MyP.name = "masterJoe"
	CNT=copy $masterJoe
	CNT.wireColor=[255,0,0]
	--PTSel.name = CNT.name + "_Baked"
	--CNT.name = PTSel.name + "_Baked"
	---------------------------------------------------------------
	 for i = fStart to fEnd do  
     ( 
          animate on 
          ( 
               at time i 
               ( 
                  CNT.transform = PTSel.transform  				 
			    )
			)
		)
		 PTSel.parent = undefined
		 PTSel.name = PTSel.name
		 PTSel.pos.controller = bezier_position ()
		 PTSel.pos.controller = Position_XYZ ()
		 PTSel.rotation.controller = tcb_rotation ()
		 PTSel.rotation.controller = Euler_XYZ ()
		 PTSel.scale.controller = bezier_scale ()
		 PTSel.scale.controller = ScaleXYZ ()
		 
		 for w = fStart to fEnd do   
     ( 
		
          animate on 
          ( 
               at time w 
               ( 
                  PTSel.transform = CNT.transform  				 
			    )
			)
		)	
	delete MyP	
	delete CNT
)

allObjects = $*

nodes = for o in allObjects where (matchPattern o.name pattern:("Bip01*")) collect ( o )

max create mode
for x in nodes do (OrgControl x)
		

)PROG" );

	ExecuteMAXScriptScript( strText.c_str() );
}

// Start the exporter!
// This is the real entrypoint to the exporter. After the user has selected
// the filename (and he's prompted for overwrite etc.) this method is called.
int AsciiExp::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options) 
{
	// Set a global prompt display switch
	showPrompts = suppressPrompts ? FALSE : TRUE;
	exportSelected = (options & SCENE_EXPORT_SELECTED) ? TRUE : FALSE;

	// Grab the interface pointer.
	ip = i;

	// Get the options the user selected the last time
	ReadConfig();

	if(showPrompts) {
		// Prompt the user with our dialogbox, and get all the options.
		if (!DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ASCIIEXPORT_DLG),
			ip->GetMAXHWnd(), ExportDlgProc, (LPARAM)this)) {
			return 1;
			}
		}

	if ( GetBakeObjects() )
		BakeObjectTransformation();
	
	swprintf(szFmtStr, L"%%4.%df", nPrecision);

	// Open the stream
	pStream = _tfopen(name,_T("wt"));
	if (!pStream) {
		return 0;
	}
	
	// Startup the progress bar.
	ip->ProgressStart(GetString(IDS_PROGRESS_MSG), TRUE, fn, NULL);

	// Get a total node count by traversing the scene
	// We don't really need to do this, but it doesn't take long, and
	// it is nice to have an accurate progress bar.
	nTotalNodeCount = 0;
	nCurNode = 0;
	PreProcess(ip->GetRootNode(), nTotalNodeCount);
	
	// First we write out a file header with global information. 
	ExportGlobalInfo();

	// Export list of material definitions
	ExportMaterialList();

	int numChildren = ip->GetRootNode()->NumberOfChildren();
	
	// Call our node enumerator.
	// The nodeEnum function will recurse into itself and 
	// export each object found in the scene.
	
	for (int idx=0; idx<numChildren; idx++) {
		if (ip->GetCancel())
			break;
		nodeEnum(ip->GetRootNode()->GetChildNode(idx), 0);
	}

	// We're done. Finish the progress bar.
	ip->ProgressEnd();

	// Close the stream
	fclose(pStream);

	// Write the current options to be used next time around.
	WriteConfig();

	return 1;
}

BOOL AsciiExp::SupportsOptions(int ext, DWORD options) {
	assert(ext == 0);	// We only support one extension
	return(options == SCENE_EXPORT_SELECTED) ? TRUE : FALSE;
	}

// This method is the main object exporter.
// It is called once of every node in the scene. The objects are
// exported as they are encoutered.

// Before recursing into the children of a node, we will export it.
// The benefit of this is that a nodes parent is always before the
// children in the resulting file. This is desired since a child's
// transformation matrix is optionally relative to the parent.

BOOL AsciiExp::nodeEnum(INode* node, int indentLevel) 
{
	if(exportSelected && node->Selected() == FALSE)
		return TREE_CONTINUE;

	nCurNode++;
	ip->ProgressUpdate((int)((float)nCurNode/nTotalNodeCount*100.0f)); 

	// Stop recursing if the user pressed Cancel 
	if (ip->GetCancel())
		return FALSE;
	
	TSTR indent = GetIndent(indentLevel);
	
	// If this node is a group head, all children are 
	// members of this group. The node will be a dummy node and the node name
	// is the actualy group name.
	if (node->IsGroupHead()) {
		fwprintf(pStream,L"%s%s \"%s\" {\n", indent.data(), ID_GROUP, FixupName(node->GetName())); 
		indentLevel++;
	}
	
	// Only export if exporting everything or it's selected
	if(!exportSelected || node->Selected()) {

		// The ObjectState is a 'thing' that flows down the pipeline containing
		// all information about the object. By calling EvalWorldState() we tell
		// max to eveluate the object at end of the pipeline.
		ObjectState os = node->EvalWorldState(0); 

		// The obj member of ObjectState is the actual object we will export.
		if (os.obj) {

			// We look at the super class ID to determine the type of the object.
			switch(os.obj->SuperClassID()) {
			case GEOMOBJECT_CLASS_ID: 
				if (GetIncludeObjGeom()) ExportGeomObject(node, indentLevel); 
				break;
			case CAMERA_CLASS_ID:
				if (GetIncludeObjCamera()) ExportCameraObject(node, indentLevel); 
				break;
			case LIGHT_CLASS_ID:
				if (GetIncludeObjLight()) ExportLightObject(node, indentLevel); 
				break;
			case SHAPE_CLASS_ID:
				if (GetIncludeObjShape()) ExportShapeObject(node, indentLevel); 
				break;
			case HELPER_CLASS_ID:
				if (GetIncludeObjHelper()) ExportHelperObject(node, indentLevel); 
				break;
			}
		}
	}	
	
	// For each child of this node, we recurse into ourselves 
	// until no more children are found.
	for (int c = 0; c < node->NumberOfChildren(); c++) {
		if (!nodeEnum(node->GetChildNode(c), indentLevel))
			return FALSE;
	}
	
	// If thie is true here, it is the end of the group we started above.
	if (node->IsGroupHead()) {
		fwprintf(pStream,L"%s}\n", indent.data());
		indentLevel--;
	}

	return TRUE;
}


void AsciiExp::PreProcess(INode* node, int& nodeCount)
{
	nodeCount++;
	
	// Add the nodes material to out material list
	// Null entries are ignored when added...
	mtlList.AddMtl(node->GetMtl());

	// For each child of this node, we recurse into ourselves 
	// and increment the counter until no more children are found.
	for (int c = 0; c < node->NumberOfChildren(); c++) {
		PreProcess(node->GetChildNode(c), nodeCount);
	}
}

/****************************************************************************

 Configuration.
 To make all options "sticky" across sessions, the options are read and
 written to a configuration file every time the exporter is executed.

 ****************************************************************************/

TSTR AsciiExp::GetCfgFilename()
{
	TSTR filename;
	
	filename += ip->GetDir(APP_PLUGCFG_DIR);
	filename += L"\\";
	filename += CFGFILENAME;

	return filename;
}

// NOTE: Update anytime the CFG file changes
#define CFG_VERSION 0x03

BOOL AsciiExp::ReadConfig()
{
	TSTR filename = GetCfgFilename();
	FILE* cfgStream;

	cfgStream = _wfopen(filename, L"rb");
	if (!cfgStream)
		return FALSE;

	// First item is a file version
	int fileVersion = _getw(cfgStream);

	if (fileVersion > CFG_VERSION) {
		// Unknown version
		fclose(cfgStream);
		return FALSE;
	}

	SetIncludeMesh(fgetc(cfgStream));
	SetIncludeAnim(fgetc(cfgStream));
	SetIncludeMtl(fgetc(cfgStream));
	SetIncludeMeshAnim(fgetc(cfgStream));
	SetIncludeCamLightAnim(fgetc(cfgStream));
	SetIncludeIKJoints(fgetc(cfgStream));
	SetIncludePhysique(fgetc(cfgStream));
	SetIncludePhysiqueAsSkin(fgetc(cfgStream));
	SetIncludeNormals(fgetc(cfgStream));
	SetIncludeTextureCoords(fgetc(cfgStream));
	SetIncludeObjGeom(fgetc(cfgStream));
	SetIncludeObjShape(fgetc(cfgStream));
	SetIncludeObjCamera(fgetc(cfgStream));
	SetIncludeObjLight(fgetc(cfgStream));
	SetIncludeObjHelper(fgetc(cfgStream));
	SetAlwaysSample(fgetc(cfgStream));
	SetMeshFrameStep(_getw(cfgStream));
	SetKeyFrameStep(_getw(cfgStream));

	// Added for version 0x02
	if (fileVersion > 0x01) {
		SetIncludeVertexColors(fgetc(cfgStream));
	}

	// Added for version 0x03
	if (fileVersion > 0x02) {
		SetPrecision(_getw(cfgStream));
	}

	fclose(cfgStream);

	return TRUE;
}

void AsciiExp::WriteConfig()
{
	TSTR filename = GetCfgFilename();
	FILE* cfgStream;

	cfgStream = _wfopen(filename, L"wb");
	if (!cfgStream)
		return;

	// Write CFG version
	_putw(CFG_VERSION,				cfgStream);

	fputc(GetIncludeMesh(),			cfgStream);
	fputc(GetIncludeAnim(),			cfgStream);
	fputc(GetIncludeMtl(),			cfgStream);
	fputc(GetIncludeMeshAnim(),		cfgStream);
	fputc(GetIncludeCamLightAnim(),	cfgStream);
	fputc(GetIncludeIKJoints(),		cfgStream);
	fputc(GetIncludePhysique(),		cfgStream);
	fputc(GetIncludePhysiqueAsSkin(),	cfgStream);
	fputc(GetIncludeNormals(),		cfgStream);
	fputc(GetIncludeTextureCoords(),	cfgStream);
	fputc(GetIncludeObjGeom(),		cfgStream);
	fputc(GetIncludeObjShape(),		cfgStream);
	fputc(GetIncludeObjCamera(),	cfgStream);
	fputc(GetIncludeObjLight(),		cfgStream);
	fputc(GetIncludeObjHelper(),	cfgStream);
	fputc(GetAlwaysSample(),		cfgStream);
	_putw(GetMeshFrameStep(),		cfgStream);
	_putw(GetKeyFrameStep(),		cfgStream);
	fputc(GetIncludeVertexColors(),	cfgStream);
	_putw(GetPrecision(),			cfgStream);

	fclose(cfgStream);
}


BOOL MtlKeeper::AddMtl(Mtl* mtl)
{
	if (!mtl) {
		return FALSE;
	}

	int numMtls = mtlTab.Count();
	for (int i=0; i<numMtls; i++) {
		if (mtlTab[i] == mtl) {
			return FALSE;
		}
	}
	mtlTab.Append(1, &mtl, 25);

	return TRUE;
}

int MtlKeeper::GetMtlID(Mtl* mtl)
{
	int numMtls = mtlTab.Count();
	for (int i=0; i<numMtls; i++) {
		if (mtlTab[i] == mtl) {
			return i;
		}
	}
	return -1;
}

int MtlKeeper::Count()
{
	return mtlTab.Count();
}

Mtl* MtlKeeper::GetMtl(int id)
{
	return mtlTab[id];
}
