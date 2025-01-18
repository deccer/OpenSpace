#pragma once

#include "../../Core/Types.hpp"
#include "../Entity.hpp"

#include <vector>

struct TParentComponent
{
    int32_t ParentId;
    TEntity Parent;
    bool HasParent = false;
    std::vector<TEntity> Children = std::vector<TEntity>();

    auto RemoveChildren(const TEntity& entity) -> bool {
        for (int i = 0; i < Children.size(); i++) {
            if (Children[i].GetHandle() == entity.GetHandle()) {
                Children.erase(Children.begin() + i);
                return true;
            }
        }

        return false;
    }
};
