#pragma once

#ifndef ECS_H
#define ECS_H

#include <entt/entt.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <concepts>
#include <span>

#include "../../utils/utils.h"
// =====================================================================
// Forward declarations
// =====================================================================
class Renderer;
class ChunkManager;
struct Mesh;

// =====================================================================
// Item / Recipe system
// =====================================================================

struct ItemStack {
    std::string itemId;
    int         count = 0;

    [[nodiscard]] bool empty() const noexcept { return count <= 0; }
};

// Concept: anything that can be used as an item ID
template<typename T>
concept ItemIdLike = std::convertible_to<T, std::string>;

struct Recipe {
    std::string              id;
    std::vector<ItemStack>   inputs;
    std::vector<ItemStack>   outputs;
    float                    duration = 1.0f; // seconds
};

// Global recipe registry — populated at startup
class RecipeDB {
public:
    static void              registerRecipe(Recipe recipe);
    [[nodiscard]] static const Recipe* get(const std::string& id) noexcept;
    [[nodiscard]] static std::span<const Recipe> all() noexcept;

private:
    static std::vector<Recipe>                      s_recipes;
    static std::unordered_map<std::string, size_t>  s_index;
};

// =====================================================================
// Components
// =====================================================================

// --- Spatial ---
struct CPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f; // height / layer
};

struct CRotation {
    float degrees = 0.0f; // 0 / 90 / 180 / 270 for iso grid
};

struct CScale {
    float x = 1.0f;
    float y = 1.0f;
    float z = 1.0f;
};

// --- Rendering ---
struct CMesh {
    std::string modelPath;   // e.g. "models/smelter.gltf"
    bool        visible = true;
};

// --- Inventory ---
struct CInventory {
    std::vector<ItemStack> items;
    int                    maxSlots  = 10;
    int                    maxStack  = 999;

    // Returns true if at least one item was added
    bool addItem(ItemIdLike auto&& id, int count);
    // Returns true if the full amount was available and removed
    bool removeItem(ItemIdLike auto&& id, int count);
    // Returns nullptr if not found
    [[nodiscard]] const ItemStack* find(const std::string& id) const noexcept;
    [[nodiscard]] int              count(const std::string& id) const noexcept;
    [[nodiscard]] bool             hasItems(const std::string& id, int n) const noexcept;
    [[nodiscard]] bool             full()  const noexcept;
    [[nodiscard]] bool             empty() const noexcept;

    // Iterate slots
    [[nodiscard]] std::span<const ItemStack> slots() const noexcept { return items; }
};

// --- Crafter ---
enum class CrafterState { Idle, Crafting, OutputFull, NoInput };

struct CCrafter {
    std::string  recipeId;
    float        progress  = 0.0f;  // seconds elapsed
    CrafterState state     = CrafterState::Idle;

    [[nodiscard]] float ratio() const noexcept;  // 0..1 progress fraction
};

// --- Conveyor belt ---
enum class Direction { North, East, South, West };

struct CBelt {
    Direction direction = Direction::East;
    float     speed     = 1.0f;          // items/second
    std::optional<ItemStack> carrying;   // one item in transit
    float     progress  = 0.0f;          // 0..1 travel along belt
};

// --- Power ---
struct CPowerConsumer {
    float demandKW  = 0.0f;
    bool  satisfied = false;
};

struct CPowerProducer {
    float outputKW = 0.0f;
};

// --- Tags (zero-size marker components) ---
struct TSelected   {};   // entity is currently selected by player
struct TDirty      {};   // entity needs a state refresh this frame
struct TPlayerOwned{};

// =====================================================================
// ECS World — thin wrapper around entt::registry
// =====================================================================
class ECSWorld {
public:
    ECSWorld() = default;
    ~ECSWorld() = default;

    // Non-copyable, movable
    ECSWorld(const ECSWorld&)            = delete;
    ECSWorld& operator=(const ECSWorld&) = delete;
    ECSWorld(ECSWorld&&)                 = default;
    ECSWorld& operator=(ECSWorld&&)      = default;

    // -----------------------------------------------------------------
    // Entity factory helpers
    // -----------------------------------------------------------------
    [[nodiscard]] entt::entity createBuilding(
        const std::string& modelPath,
        float x, float y,
        const std::string& recipeId = "",
        int inventorySlots = 10
    );

    [[nodiscard]] entt::entity createBelt(
        float x, float y,
        Direction dir,
        float speed = 1.0f
    );

    void destroy(entt::entity e);

    // -----------------------------------------------------------------
    // Component access (forwarded for convenience)
    // -----------------------------------------------------------------
    template<typename T>
    [[nodiscard]] T& get(entt::entity e) { return m_registry.get<T>(e); }

    template<typename T>
    [[nodiscard]] const T& get(entt::entity e) const { return m_registry.get<T>(e); }

    template<typename T>
    [[nodiscard]] T* tryGet(entt::entity e) noexcept { return m_registry.try_get<T>(e); }

    template<typename T>
    [[nodiscard]] bool has(entt::entity e) const noexcept { return m_registry.all_of<T>(e); }

    template<typename T, typename... Args>
    T& emplace(entt::entity e, Args&&... args) {
        return m_registry.emplace<T>(e, std::forward<Args>(args)...);
    }

    // -----------------------------------------------------------------
    // Systems — call each frame
    // -----------------------------------------------------------------
    void updateCrafters(float dt);
    void updateBelts(float dt);
    void renderMeshes(Renderer& renderer);

    // -----------------------------------------------------------------
    // Queries
    // -----------------------------------------------------------------
    [[nodiscard]] std::optional<entt::entity> entityAt(float x, float y) const noexcept;

    // Call cb for every entity with component T
    template<typename T, typename Fn>
    void forEach(Fn&& cb) {
        m_registry.view<T>().each(std::forward<Fn>(cb));
    }

    [[nodiscard]] entt::registry&       raw()       noexcept { return m_registry; }
    [[nodiscard]] const entt::registry& raw() const noexcept { return m_registry; }

private:
    entt::registry m_registry;

    // helpers
    void tryStartCraft(CCrafter& crafter, CInventory& inv);
    void finishCraft  (CCrafter& crafter, CInventory& inv);
};

// =====================================================================
// CInventory template implementations (must be in header)
// =====================================================================

inline bool CInventory::addItem(ItemIdLike auto&& id, int count) {
    const std::string sid{ std::forward<decltype(id)>(id) };
    for (auto& stack : items) {
        if (stack.itemId == sid) {
            stack.count += count;
            return true;
        }
    }
    if (static_cast<int>(items.size()) >= maxSlots) return false;
    items.push_back({ sid, count });
    return true;
}

inline bool CInventory::removeItem(ItemIdLike auto&& id, int count) {
    const std::string sid{ std::forward<decltype(id)>(id) };
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->itemId == sid && it->count >= count) {
            it->count -= count;
            if (it->count == 0) items.erase(it);
            return true;
        }
    }
    return false;
}

#endif