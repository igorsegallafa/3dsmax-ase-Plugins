#include "stdafx.h"
#include "PublishedInterface.h"
#include "modstack.h"

#define VIEW_DEFAULT_WIDTH ((float)400.0)

//SDK level methods
//****************************************************************
Modifier* IPhysiqueInterface::FindPhysiqueModifier (INode* nodePtr, int modIndex)
{
      // Get object from node. Abort if no object.
      if (!nodePtr) return NULL;

      Object* ObjectPtr = nodePtr->GetObjectRef();

      if (!ObjectPtr ) return NULL;
      
      while (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID && ObjectPtr)
      {
            // Yes -> Cast.
            IDerivedObject* DerivedObjectPtr = (IDerivedObject *)(ObjectPtr);                   
                  
            // Iterate over all entries of the modifier stack.
            int modStackIndex = 0, phyModIndex = 0;
            
            while (modStackIndex < DerivedObjectPtr->NumModifiers())
            {
                  // Get current modifier.
                  Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(modStackIndex);

                  // Is this Physique ?
                  if (ModifierPtr->ClassID() == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B))
                  {
                        if (phyModIndex >= modIndex)
                        { 
                              return ModifierPtr;
                        }
                        phyModIndex++;
                  }
                  modStackIndex++;
            }
            ObjectPtr = DerivedObjectPtr->GetObjRef();
      }

      // Not found.
      throw MAXException(L"No physique modifier found");
      return NULL;
}

BOOL IPhysiqueInterface::CheckMatrix(INode* node, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");
      
      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);


      int version = phyExport->Version();
      
      if(version == 30)
      {
      //get the node's initial transformation matrix and store it in a matrix3
            Matrix3 initTM = Matrix3(TRUE);
            int msg = phyExport->GetInitNodeTM(node, initTM);

            //Release the physique interface


            if (msg == NO_MATRIX_SAVED)
                  return false;
      }

      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
      return true;


}

Matrix3 IPhysiqueInterface::GetInitialNodeTM(INode* node, ReferenceTarget* mod)
{

      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");
      
      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get the node's initial transformation matrix and store it in a matrix3
      Matrix3 initTM = Matrix3(TRUE);
      int msg = phyExport->GetInitNodeTM(node, initTM);

      //Release the physique interface
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
      if (msg != MATRIX_RETURNED)
      {
            switch (msg)
            {
                  case NODE_NOT_FOUND:
                        throw MAXException (L"Node not found");
                  case NO_MATRIX_SAVED:
                        throw MAXException(L"No matrix saved");
                  case INVALID_MOD_POINTER:
                        throw MAXException(L"Invalid modifier pointer");
                  default: 
                        break;
            }
      }


      return initTM;
}

Matrix3 IPhysiqueInterface::GetInitialBoneTM(INode* node, int index, ReferenceTarget* mod)
{

      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");
      
      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get the list of bones
      Tab<INode*> bones = GetBoneList(node);
      if (index <0 || index >= bones.Count()) throw MAXException(L"The Bone index is not in the valid range");

      //get the node's initial transformation matrix and store it in a matrix3
      Matrix3 initTM = Matrix3(TRUE);
      int msg = phyExport->GetInitNodeTM(bones[index], initTM);
      //Release the physique interface
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
      
      if (msg != MATRIX_RETURNED)
      {
            switch (msg)
            {
                  case NODE_NOT_FOUND:
                        throw MAXException(L"Node not found");
                  case NO_MATRIX_SAVED:
                        throw MAXException(L"No matrix saved");
                  case INVALID_MOD_POINTER:
                        throw MAXException(L"Invalid modifier pointer");
                  default: 
                        break;
            }
      }
      return initTM;
}

int IPhysiqueInterface::GetBoneCount(INode* node, ReferenceTarget* mod)
{
      //get the list of bones
      Tab<INode*> bones = GetBoneList(node, mod);
      return bones.Count();
}



// This function can be used to gather the list of bones used by the modifier
//****************************************************************************
Tab<INode*> IPhysiqueInterface::GetBoneList(INode* node, ReferenceTarget* mod)
{
      int i = 0, x = 0;
      Tab<INode*> bones;
      INode* bone;

      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get a pointer to the export context interface
      IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(node);

      //convert to rigid for time independent vertex assignment
      mcExport->ConvertToRigid(true);

      //allow blending to export multi-link assignments
      mcExport->AllowBlending(true);

      //These are the different types of vertex classes 
      IPhyBlendedRigidVertex *rb_vtx;
      IPhyRigidVertex *r_vtx;
      IPhyFloatingVertex *f_vtx;

      //get the vertex count from the export interface
      int numverts = mcExport->GetNumberVertices();
      
      //iterate through all vertices and gather the bone list
      for (i = 0; i<numverts; i++) 
      {
            BOOL exists = false;
            
            //get the hierarchial vertex interface
            IPhyVertexExport* vi = mcExport->GetVertexInterface(i);
            if (vi) {

                  //check the vertex type and process accordingly
                  int type = vi->GetVertexType();
                  switch (type) 
                  {
                        //The vertex is rigid, blended vertex.  It's assigned to multiple links
                        case RIGID_BLENDED_TYPE:
                              //type-cast the node to the proper class        
                              rb_vtx = (IPhyBlendedRigidVertex*)vi;
                              
                              //iterate through the bones assigned to this vertex
                              for (x = 0; x<rb_vtx->GetNumberNodes(); x++) 
                              {
                                    exists = false;
                                    //get the node by index
                                    bone = rb_vtx->GetNode(x);
                                    
                                    //check to see if we already have this bone
                                    for (int z=0;z<bones.Count();z++)
                                          if (bone == bones[z]) exists = true;

                                    //if you didn't find a match add it to the list
                                    if (!exists) bones.Append(1, &bone);
                              }
                              break;
                        //The vertex is a rigid vertex and only assigned to one link
                        case RIGID_TYPE:
                              //type-cast the node to the proper calss
                              r_vtx = (IPhyRigidVertex*)vi;
                              
                              //get the node
                              bone = r_vtx->GetNode();

                              //check to see if the bone is already in the list
                              for (x = 0;x<bones.Count();x++)
                              {
                                    if (bone == bones[x]) exists = true;
                              }
                              //if you didn't find a match add it to the list
                              if (!exists) bones.Append(1, &bone);
                              break;

                        case FLOATING_TYPE:
                              f_vtx = (IPhyFloatingVertex*)mcExport->GetVertexInterface(i);
                              for (x = 0; x<f_vtx->GetNumberNodes(); x++)
                              {
                                    exists = false;

                                     //get the node by index
                                    bone = f_vtx->GetNode(x); 
                                    
                                    //check to see if we already have this bone
                                    for (int z=0;z<bones.Count();z++)
                                          if (bone == bones[z]) exists = true;

                                    //if you didn't find a match add it to the list
                                    if (!exists) bones.Append(1, &bone);
                              }
                              break;
      
                        // Shouldn't make it here because we converted to rigid earlier.  
                        // It should be one of the above two types
                        default: break;  
                  }
            }
             
      }

      //release the context interface
      phyExport->ReleaseContextInterface(mcExport);

      //Release the physique interface
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);

      return bones;
}

int IPhysiqueInterface::GetAPIVersion(INode* node, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);
      int version = phyExport->Version();
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);

      return version;
}

void IPhysiqueInterface::SetInitialPose(INode* node, bool set, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      phyExport->SetInitialPose(set);
      ((ReferenceTarget*)mod)->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
}

int IPhysiqueInterface::GetVertexCount(INode* node, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get a pointer to the export context interface
      IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(node);

      int vertCount = mcExport->GetNumberVertices();

      phyExport->ReleaseContextInterface(mcExport);
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);

      return vertCount;
}

int IPhysiqueInterface::GetVertexType(INode* node, int vertIndex, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get a pointer to the export context interface
      IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(node);
      
      IPhyVertexExport* vi = mcExport->GetVertexInterface(vertIndex);
      if (!vi) 
      { 
            phyExport->ReleaseContextInterface(mcExport);
            mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
            throw MAXException(L"No vertex interface found"); 
      } 

      int type = vi->GetVertexType();

      mcExport->ReleaseVertexInterface(vi);
      phyExport->ReleaseContextInterface(mcExport);
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);

      return type;
}

int IPhysiqueInterface::GetVertexBoneCount(INode* node, int vertIndex, bool rigid, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get a pointer to the export context interface
      IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(node);

      if (vertIndex < 0 || vertIndex >= mcExport->GetNumberVertices())
            throw MAXException(L"The vertex index is not in the valid range");

      mcExport->ConvertToRigid(rigid);
      int count = 0;

      IPhyVertexExport* vi = mcExport->GetVertexInterface(vertIndex);
      if (vi) 
      { 
            int type = vi->GetVertexType();
            switch (type)
            {
                  case RIGID_NON_BLENDED_TYPE:
                  case DEFORMABLE_NON_BLENDED_TYPE:
                        count += 1;
                        break;
                  case RIGID_BLENDED_TYPE:
                        count += ((IPhyBlendedRigidVertex*)vi)->GetNumberNodes();
                        break;
                  case DEFORMABLE_BLENDED_TYPE:
                        throw MAXException(L"Not a currently supported type");
                        break;
                  case FLOATING_TYPE:
                        count += ((IPhyFloatingVertex*)vi)->GetNumberNodes();
                        break;
            }

            mcExport->ReleaseVertexInterface(vi);

      }

      phyExport->ReleaseContextInterface(mcExport);
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);

      return count;
}

Tab<INode*> IPhysiqueInterface::GetVertexBones(INode* node, int vertIndex, bool rigid, bool blending, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get a pointer to the export context interface
      IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(node);
      mcExport->ConvertToRigid(rigid);
      mcExport->AllowBlending(blending);
      
      if (vertIndex < 0 || vertIndex >= mcExport->GetNumberVertices())
            throw MAXException(L"The vertex index is not in the valid range");

      INode* bone = NULL;
      Tab<INode*> bones;
      bones.ZeroCount();
      int count = 0, i = 0;

      IPhyVertexExport* vi = mcExport->GetVertexInterface(vertIndex);
      if (vi) 
      { 
            int type = vi->GetVertexType();
            switch (type)
            {
                  case RIGID_NON_BLENDED_TYPE:
                        bone = ((IPhyRigidVertex*)vi)->GetNode();
                        bones.Append(1, &bone);
                        break;
                  case RIGID_BLENDED_TYPE:
                        count = ((IPhyBlendedRigidVertex*)vi)->GetNumberNodes();
                        for (i=0;i<count;i++)
                        {
                              bone = ((IPhyBlendedRigidVertex*)vi)->GetNode(i);
                              bones.Append(1, &bone);
                        }
                        break;
                  case DEFORMABLE_NON_BLENDED_TYPE:
                        bone = ((IPhyDeformableOffsetVertex*)vi)->GetNode();
                        bones.Append(1, &bone);
                        break;
                  case DEFORMABLE_BLENDED_TYPE:
                        throw MAXException(L"Not a currently supported type");
                        break;
                  case FLOATING_TYPE:
                        count += ((IPhyFloatingVertex*)vi)->GetNumberNodes();
                        for (i=0;i<count;i++)
                        {
                              bone = ((IPhyFloatingVertex*)vi)->GetNode(i);
                              bones.Append(1, &bone);
                        }
                        break;
            }

            mcExport->ReleaseVertexInterface(vi);
      }

      phyExport->ReleaseContextInterface(mcExport);
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
      
      return bones;
}

INode* IPhysiqueInterface::GetVertexBone(INode* node, int vertIndex, int boneIndex, bool rigid, bool blending, ReferenceTarget* mod)
{
      Tab<INode*> bones = GetVertexBones(node, vertIndex, rigid, blending, mod);
      if (boneIndex < 0 || boneIndex >= bones.Count())
            throw MAXException(L"The bone index is not in the valid range");

      return bones[boneIndex];
}

Point3 IPhysiqueInterface::GetVertexOffset(INode* node, int vertIndex, int boneIndex, bool rigid, bool blending, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get a pointer to the export context interface
      IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(node);
      mcExport->ConvertToRigid(rigid);
      mcExport->AllowBlending(blending);
      
      if (vertIndex < 0 || vertIndex >= mcExport->GetNumberVertices())
            throw MAXException(L"The vertex index is not in the valid range");

      Point3 offset = Point3(0,0,0);
      Tab<Point3> offsetTab;
      offsetTab.ZeroCount();
      int count = 0, i = 0;

      IPhyVertexExport* vi = mcExport->GetVertexInterface(vertIndex);
      if (vi) 
      { 
            int type = vi->GetVertexType();
            switch (type)
            {
                  case RIGID_NON_BLENDED_TYPE:
                        offset = ((IPhyRigidVertex*)vi)->GetOffsetVector();
                        offsetTab.Append(1, &offset);
                        break;
                  case RIGID_BLENDED_TYPE:
                        count = ((IPhyBlendedRigidVertex*)vi)->GetNumberNodes();
                        for (i=0;i<count;i++)
                        {
                              offset = ((IPhyBlendedRigidVertex*)vi)->GetOffsetVector(i);
                              offsetTab.Append(1, &offset);
                        }
                        break;
                  case DEFORMABLE_NON_BLENDED_TYPE:
                        offset = ((IPhyDeformableOffsetVertex*)vi)->GetOffsetVector();
                        offsetTab.Append(1, &offset);
                        break;
                  case DEFORMABLE_BLENDED_TYPE:
                        throw MAXException(L"Not a currently supported type");
                        break;
                  case FLOATING_TYPE:
                        count += ((IPhyFloatingVertex*)vi)->GetNumberNodes();
                        for (i=0;i<count;i++)
                        {
                              offset = ((IPhyFloatingVertex*)vi)->GetOffsetVector(i);
                              offsetTab.Append(1, &offset);
                        }
                        break;
            }

            mcExport->ReleaseVertexInterface(vi);

      }

      phyExport->ReleaseContextInterface(mcExport);
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);

      if (boneIndex < 0 || boneIndex >= offsetTab.Count())
            throw MAXException(L"The bone index is not in the valid range");
      
      return offsetTab[boneIndex];
}

Point3 IPhysiqueInterface::GetVertexDeformableOffset(INode* node, int vertIndex, ReferenceTarget* mod, TimeValue t)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get a pointer to the export context interface
      IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(node);
      
      if (vertIndex < 0 || vertIndex >= mcExport->GetNumberVertices())
            throw MAXException(L"The vertex index is not in the valid range");

      Point3 offset = Point3(0,0,0);

      IPhyVertexExport* vi = mcExport->GetVertexInterface(vertIndex);
      if (vi) 
      { 
            int type = vi->GetVertexType();
            switch (type)
            {
                  case RIGID_NON_BLENDED_TYPE:
                  case RIGID_BLENDED_TYPE:
                  case FLOATING_TYPE:
                        break;
                  case DEFORMABLE_NON_BLENDED_TYPE:
                        offset = ((IPhyDeformableOffsetVertex*)vi)->GetDeformOffsetVector(t);
                        break;
                  case DEFORMABLE_BLENDED_TYPE:
                        throw MAXException(L"Not a currently supported type");
                        break;
            }
            mcExport->ReleaseVertexInterface(vi);
      }

      phyExport->ReleaseContextInterface(mcExport);
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
      return offset;
}

float IPhysiqueInterface::GetVertexWeight(INode* node, int vertIndex, int boneIndex, bool rigid, bool blending, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      //get a pointer to the export interface
      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);

      //get a pointer to the export context interface
      IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(node);
      mcExport->ConvertToRigid(rigid);
      mcExport->AllowBlending(blending);
      
      if (vertIndex < 0 || vertIndex >= mcExport->GetNumberVertices())
            throw MAXException(L"The vertex index is not in the valid range");

      //Point3 offset = Point3(0,0,0);
      //Tab<Point3> offsetTab;
      //offsetTab.ZeroCount();
      int count = 0, i = 0;
      float normalizedWeight = 0.0f, totalWeight = 0.0f, weight = 0.0f;
      float tempFloat;

      IPhyVertexExport* vi = mcExport->GetVertexInterface(vertIndex);
      if (vi) 
      { 
            int type = vi->GetVertexType();
            switch (type)
            {
                  case RIGID_NON_BLENDED_TYPE:
                  case DEFORMABLE_NON_BLENDED_TYPE:
                        count = 1;
                        normalizedWeight = 1.0f;
                        break;
                  case RIGID_BLENDED_TYPE:
                        count = ((IPhyBlendedRigidVertex*)vi)->GetNumberNodes();
                        for (i=0;i<count;i++)
                        {
                              tempFloat = ((IPhyBlendedRigidVertex*)vi)->GetWeight(i);
                              if (i == boneIndex) normalizedWeight = tempFloat;
                        }
                        break;
                  case DEFORMABLE_BLENDED_TYPE:
                        throw MAXException(L"Not a currently supported type");
                        break;
                  case FLOATING_TYPE:
                        normalizedWeight = 0.0f;
                        for (i=0;i<((IPhyFloatingVertex*)vi)->GetNumberNodes();i++)
                        {
                              tempFloat = ((IPhyFloatingVertex*)vi)->GetWeight(i, weight);
                              if (count + i == boneIndex) normalizedWeight = tempFloat;
                              totalWeight += weight;
                        }
                        count = count+i;

                        if (totalWeight != 0) 
                              normalizedWeight = normalizedWeight/totalWeight;
                        break;
            }

            mcExport->ReleaseVertexInterface(vi);
      }

      phyExport->ReleaseContextInterface(mcExport);
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);

      if (boneIndex < 0 || boneIndex >= count)
            throw MAXException(L"The bone index is not in the valid range");
      
      return normalizedWeight;
}

bool IPhysiqueInterface::SetFigureMode(INode* node, bool state, ReferenceTarget* mod)
{
      int i = 0, x = 0;
      INode* bone;

      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      IPhysiqueExport *phyExport = (IPhysiqueExport *)mod->GetInterface(I_PHYINTERFACE);
      IPhyContextExport *mcExport = (IPhyContextExport *)phyExport->GetContextInterface(node);
      int numverts = mcExport->GetNumberVertices();
      
      for (i = 0; i<numverts; i++) 
      {
            IPhyVertexExport* vi = mcExport->GetVertexInterface(i);
            if (vi) {
                  int type = vi->GetVertexType();
                  switch (type) 
                  {
                        case RIGID_BLENDED_TYPE:
                              for (x = 0; x<((IPhyBlendedRigidVertex*)vi)->GetNumberNodes(); x++) 
                              {
                                    bone = ((IPhyBlendedRigidVertex*)vi)->GetNode(x);
                                    if (BipedFigureMode(bone, state))
                                    {
                                          mcExport->ReleaseVertexInterface(vi);
                                          phyExport->ReleaseContextInterface(mcExport);
                                          mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
                                          return true;
                                    }

                              }
                              break;
                        case RIGID_TYPE:
                              bone = ((IPhyRigidVertex*)vi)->GetNode();
                              if (BipedFigureMode(bone, state))
                              {
                                    mcExport->ReleaseVertexInterface(vi);
                                    phyExport->ReleaseContextInterface(mcExport);
                                    mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
                                    return true;
                              }
                              break;
                        case DEFORMABLE_TYPE:
                              bone = ((IPhyDeformableOffsetVertex*)vi)->GetNode();
                              if (BipedFigureMode(bone, state))
                              {
                                    mcExport->ReleaseVertexInterface(vi);
                                    phyExport->ReleaseContextInterface(mcExport);
                                    mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
                                    return true;
                              }
                              break;
                        case FLOATING_TYPE:
                              for (x = 0; x<((IPhyFloatingVertex*)vi)->GetNumberNodes(); x++)
                              {
                                    bone = ((IPhyFloatingVertex*)vi)->GetNode(x); 
                                    if (BipedFigureMode(bone, state))
                                    {
                                          mcExport->ReleaseVertexInterface(vi);
                                          phyExport->ReleaseContextInterface(mcExport);
                                          mod->ReleaseInterface(I_PHYINTERFACE, phyExport);
                                          return true;
                                    }
                              }
                              break;
                        default: break;  
                  }
            }
            mcExport->ReleaseVertexInterface(vi);

      }

      phyExport->ReleaseContextInterface(mcExport);
      mod->ReleaseInterface(I_PHYINTERFACE, phyExport);

      return false;
}

bool IPhysiqueInterface::BipedFigureMode(INode* node, bool state)
{
      if (node->IsRootNode()) return false;

      Control* c = node->GetTMController();
    if ((c->ClassID() == BIPDRIVEN_CONTROL_CLASS_ID) ||
         (c->ClassID() == BIPBODY_CONTROL_CLASS_ID) ||
         (c->ClassID() == FOOTPRINT_CLASS_ID))
    {
        IBipedExport *iBiped = (IBipedExport *) c->GetInterface(I_BIPINTERFACE);
        if (iBiped)
            {
                  //iBiped->RemoveNonUniformScale((BOOL)state);
                  if (state) iBiped->BeginFigureMode(0);
                  else iBiped->EndFigureMode(0);

                  Control* iMaster = (Control *) c->GetInterface(I_DRIVER);
                  iMaster->NotifyDependents(FOREVER, PART_TM, REFMSG_CHANGE);
                  
                  c->ReleaseInterface(I_DRIVER,iMaster);
                  c->ReleaseInterface(I_BIPINTERFACE,iBiped);
                  return true;
            }
      }
      return false;
}

bool IPhysiqueInterface::AttachToNode(INode* node, INode* rootNode, ReferenceTarget* mod, TimeValue t)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      IPhysiqueImport *phyImport = (IPhysiqueImport *)mod->GetInterface(I_PHYIMPORT);
      bool val = false;
      if (phyImport->AttachRootNode(rootNode, t))
            val = true;
      mod->ReleaseInterface(I_PHYIMPORT, phyImport);
      return val;
}


bool IPhysiqueInterface::Initialize(INode* node, INode* rootNode, ReferenceTarget* mod, TimeValue t)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      IPhysiqueImport *phyImport = (IPhysiqueImport *)mod->GetInterface(I_PHYIMPORT);
      bool val = false;
      if (phyImport->InitializePhysique(rootNode, t))
            val = true;
      mod->ReleaseInterface(I_PHYIMPORT, phyImport);
      return val;
}

void IPhysiqueInterface::LockVertex(INode* node, int vertexIndex, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      IPhysiqueImport *phyImport = (IPhysiqueImport *)mod->GetInterface(I_PHYIMPORT);
      IPhyContextImport *mcImport = (IPhyContextImport *)phyImport->GetContextInterface(node);
      IPhyVertexImport *vi = mcImport->SetVertexInterface(vertexIndex, RIGID_BLENDED_TYPE);
      if( vi )
	  {
		  vi->LockVertex(TRUE);
		  mcImport->ReleaseVertexInterface(vi);
	  }
	  else
		  ERRORBOX( "vi NULL" );

      phyImport->ReleaseContextInterface(mcImport);
      mod->ReleaseInterface(I_PHYIMPORT, phyImport);
}

void IPhysiqueInterface::UnLockVertex(INode* node, int vertexIndex, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      IPhysiqueImport *phyImport = (IPhysiqueImport *)mod->GetInterface(I_PHYIMPORT);
      IPhyContextImport *mcImport = (IPhyContextImport *)phyImport->GetContextInterface(node);
      IPhyVertexImport *vi = mcImport->SetVertexInterface(vertexIndex, RIGID_BLENDED_TYPE);
      
	  if( vi )
	  {
		  vi->LockVertex(FALSE);
		  mcImport->ReleaseVertexInterface(vi);
	  }
	  else
		  ERRORBOX( "vi NULL" );


      phyImport->ReleaseContextInterface(mcImport);
      mod->ReleaseInterface(I_PHYIMPORT, phyImport);
}

void IPhysiqueInterface::SetVertexBone(INode* node, int vertexIndex, INode* bone, float weight, bool clear, ReferenceTarget* mod)
{
      if (!mod) mod = FindPhysiqueModifier(node, 0);
      if (!mod) throw MAXException(L"No Physique modifier was found on this node");

      IPhysiqueImport *phyImport = (IPhysiqueImport *)mod->GetInterface(I_PHYIMPORT);
      IPhyContextImport *mcImport = (IPhyContextImport *)phyImport->GetContextInterface(node);
      IPhyVertexImport *viImport = mcImport->SetVertexInterface(vertexIndex, RIGID_BLENDED_TYPE);
      
	  if( viImport )
	  {
		  int numverts = mcImport->GetNumberVertices();
      
		  if (!clear)  //if you don't clear it check to see if it already exists
		  {
				Tab<INode*> bones = GetVertexBones(node, vertexIndex, true, true, mod);
				Tab<float> weights;
				weights.ZeroCount();
				bool exists = false;
				int i;

				for(i=0;i<bones.Count();i++)
				{
					  if (bone == bones[i])
					  {
							exists = true;
							break;
					  }
				}
				if (exists)
				{
					  //store the weights
					  for(i=0;i<bones.Count();i++)
					  {
							float curWeight = GetVertexWeight(node, vertexIndex, i, true, true, mod);
							weights.Append(1, &weight);
					  }
					  for(i=0;i<bones.Count();i++)
					  {
							bool tempClear = false;
							if (i==0) 
								  tempClear = true; //clear the first time through
							float curWeight;
							if (bone == bones[i])
								  curWeight = weight;
							else curWeight = weights[i];

							//int type = viImport->GetVertexType();
							((IPhyBlendedRigidVertexImport*)viImport)->SetWeightedNode(bone, curWeight, tempClear);
					  }
				}
				else 
				{
					  ((IPhyBlendedRigidVertexImport*)viImport)->SetWeightedNode(bone, weight, FALSE);
				}
		  }
		  else ((IPhyBlendedRigidVertexImport*)viImport)->SetWeightedNode(bone, weight, TRUE);
      
		  mcImport->ReleaseVertexInterface(viImport);
	  }
	  else
		  ERRORBOX( "viImport NULL" );

      phyImport->ReleaseContextInterface(mcImport);
      mod->ReleaseInterface(I_PHYIMPORT, phyImport);
}
