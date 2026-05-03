#include "ecs.h"
#include "../../renderer/renderer.h"

#include <algorithm>
#include <iostream>
#include <ranges>

// =====================================================================
// RecipeDB
// =====================================================================

std::vector<Recipe>                     RecipeDB::s_recipes;
std::unordered_map<std::string, size_t> RecipeDB::s_index;

void RecipeDB::registerRecipe(Recipe recipe) {
    const auto id = recipe.id;
    s_index[id]   = s_recipes.size();
    s_recipes.push_back(std::move(recipe));
}

const Recipe* RecipeDB::get(const std::string& id) noexcept {
    if (auto it = s_index.find(id); it != s_index.end())
        return &s_recipes[it->second];
    return nullptr;
}

std::span<const Recipe> RecipeDB::all() noexcept {
    return s_recipes;
}

// =====================================================================
// CInventory
// =====================================================================

const ItemStack* CInventory::find(const std::string& id) const noexcept {
    for (const auto& stack : items)
        if (stack.itemId == id) return &stack;
    return nullptr;
}

int CInventory::count(const std::string& id) const noexcept {
    if (const auto* s = find(id)) return s->count;
    return 0;
}

bool CInventory::hasItems(const std::string& id, int n) const noexcept {
    return count(id) >= n;
}

bool CInventory::full() const noexcept {
    return static_cast<int>(items.size()) >= maxSlots;
}

bool CInventory::empty() const noexcept {
    return items.empty();
}

// =====================================================================
// CCrafter
// =====================================================================

float CCrafter::ratio() const noexcept {
    if (state != CrafterState::Crafting) return 0.0f;
    const auto* recipe = RecipeDB::get(recipeId);
    if (!recipe || recipe->duration <= 0.0f) return 0.0f;
    return std::clamp(progress / recipe->duration, 0.0f, 1.0f);
}

// =====================================================================
// ECSWorld — entity factory
// =====================================================================

entt::entity ECSWorld::createBuilding(
    const std::string& modelPath,
    float x, float y,
    const std::string& recipeId,
    int inventorySlots)
{
    auto e = m_registry.create();

    m_registry.emplace<CPosition>(e, x, y, 0.0f);
    m_registry.emplace<CRotation>(e);
    m_registry.emplace<CScale>(e);
    m_registry.emplace<CMesh>(e, modelPath, true);

    auto& inv      = m_registry.emplace<CInventory>(e);
    inv.maxSlots   = inventorySlots;

    if (!recipeId.empty())
        m_registry.emplace<CCrafter>(e, recipeId);

    return e;
}

entt::entity ECSWorld::createBelt(float x, float y, Direction dir, float speed) {
    auto e = m_registry.create();
    m_registry.emplace<CPosition>(e, x, y, 0.0f);
    m_registry.emplace<CMesh>(e, "models/belt.gltf", true);
    m_registry.emplace<CBelt>(e, dir, speed);
    return e;
}

void ECSWorld::destroy(entt::entity e) {
    m_registry.destroy(e);
}

// =====================================================================
// System — Crafters
// =====================================================================

void ECSWorld::tryStartCraft(CCrafter& crafter, CInventory& inv) {
    const auto* recipe = RecipeDB::get(crafter.recipeId);
    if (!recipe) { crafter.state = CrafterState::Idle; return; }

    // Check all inputs are available
    for (const auto& input : recipe->inputs) {
        if (!inv.hasItems(input.itemId, input.count)) {
            crafter.state = CrafterState::NoInput;
            return;
        }
    }

    // Check output space (rough check: at least one free slot or existing stack)
    for (const auto& output : recipe->outputs) {
        const auto* existing = inv.find(output.itemId);
        if (!existing && inv.full()) {
            crafter.state = CrafterState::OutputFull;
            return;
        }
    }

    // Consume inputs
    for (const auto& input : recipe->inputs)
        inv.removeItem(input.itemId, input.count);

    crafter.progress = 0.0f;
    crafter.state    = CrafterState::Crafting;
}

void ECSWorld::finishCraft(CCrafter& crafter, CInventory& inv) {
    const auto* recipe = RecipeDB::get(crafter.recipeId);
    if (!recipe) return;

    for (const auto& output : recipe->outputs)
        inv.addItem(output.itemId, output.count);

    crafter.progress = 0.0f;
    crafter.state    = CrafterState::Idle;

    std::cout << "[ECS] Crafted recipe: " << crafter.recipeId << '\n';
}

void ECSWorld::updateCrafters(float dt) {
    auto view = m_registry.view<CCrafter, CInventory>();

    for (auto [entity, crafter, inv] : view.each()) {
        switch (crafter.state) {

        case CrafterState::Idle:
        case CrafterState::NoInput:
        case CrafterState::OutputFull:
            tryStartCraft(crafter, inv);
            break;

        case CrafterState::Crafting: {
            crafter.progress += dt;
            const auto* recipe = RecipeDB::get(crafter.recipeId);
            if (recipe && crafter.progress >= recipe->duration)
                finishCraft(crafter, inv);
            break;
        }
        }
    }
}

// =====================================================================
// System — Conveyor Belts
// =====================================================================

void ECSWorld::updateBelts(float dt) {
    auto view = m_registry.view<CBelt, CPosition>();

    for (auto [entity, belt, pos] : view.each()) {
        if (!belt.carrying) continue;

        belt.progress += dt * belt.speed;

        if (belt.progress >= 1.0f) {
            // Try to push item to next belt or building inventory
            const float dx = (belt.direction == Direction::East)  ?  1.0f
                           : (belt.direction == Direction::West)  ? -1.0f : 0.0f;
            const float dy = (belt.direction == Direction::North) ?  1.0f
                           : (belt.direction == Direction::South) ? -1.0f : 0.0f;

            auto target = entityAt(pos.x + dx, pos.y + dy);
            bool pushed = false;

            if (target) {
                // Try belt-to-belt
                if (auto* nextBelt = m_registry.try_get<CBelt>(*target)) {
                    if (!nextBelt->carrying) {
                        nextBelt->carrying = belt.carrying;
                        nextBelt->progress = 0.0f;
                        pushed = true;
                    }
                }
                // Try belt-to-inventory
                else if (auto* inv = m_registry.try_get<CInventory>(*target)) {
                    if (inv->addItem(belt.carrying->itemId, belt.carrying->count))
                        pushed = true;
                }
            }

            if (pushed) {
                belt.carrying.reset();
                belt.progress = 0.0f;
            } else {
                // Blocked — clamp and wait
                belt.progress = 1.0f;
            }
        }
    }
}

// =====================================================================
// System — Render meshes
// =====================================================================

void ECSWorld::renderMeshes(Renderer& renderer) {
    auto view = m_registry.view<CMesh, CPosition>();

    for (auto [entity, mesh, pos] : view.each()) {
        if (!mesh.visible) continue;

        // Lazy-load mesh into renderer cache
        if (!renderer.meshCache.contains(mesh.modelPath))
            renderer.meshCache[mesh.modelPath] = renderer.loadGLTF(mesh.modelPath);

        const auto& rotation = m_registry.get_or_emplace<CRotation>(entity);
        const auto& scale    = m_registry.get_or_emplace<CScale>(entity);

        // Build transform — adapt Mat4 helpers to yours
        Mat4 transform = Mat4::translate(pos.x, pos.y, pos.z)
                       * Mat4::rotateZ(rotation.degrees)
                       * Mat4::scale(scale.x, scale.y, scale.z);

        renderer.renderMesh(renderer.meshCache[mesh.modelPath], transform);
    }
}

// =====================================================================
// Query — entity at world position
// =====================================================================

std::optional<entt::entity> ECSWorld::entityAt(float x, float y) const noexcept {
    auto view = m_registry.view<const CPosition>();
    for (auto [entity, pos] : view.each()) {
        // Grid snap: match within 0.5 units
        if (std::abs(pos.x - x) < 0.5f && std::abs(pos.y - y) < 0.5f)
            return entity;
    }
    return std::nullopt;
}