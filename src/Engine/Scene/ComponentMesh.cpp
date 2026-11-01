#include "Engine/Scene/ComponentMesh.h"

#include "Engine/Resources/Mesh.h"
#include "Engine/Scene/ComponentTransform.h"
#include "Engine/Scene/GameObject.h"

AABB ComponentMesh::GetWorldAABB() const
{
    if (!mesh)
        return {};
    return mesh->GetAABB().Transformed(GetOwner()->GetTransform()->GetWorldMatrix());
}
