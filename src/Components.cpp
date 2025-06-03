#include "Components.hpp"

auto TComponentHierarchy::AddChild(const entt::entity child) -> void {
    assert(std::ranges::count(Children.begin(), Children.end(), child) == 0);
    Children.emplace_back(child);
}

auto TComponentHierarchy::RemoveChild(const entt::entity child) -> void {
    assert(std::ranges::count(Children.begin(), Children.end(), child) == 1);
    std::erase(Children, child);
}
