#ifndef UTILS_H
#define UTILS_H

#include <cmath>
#include <iostream>
#include <chrono>
#include <array>
#include <fstream>
#include "nlohmann/json.hpp"

// ------------------------
//   TEMPLATE VECTOR CLASS
// ------------------------
template <size_t N, typename T = float>
struct Vec {
    std::array<T, N> data{};

    // Constructeurs
    Vec() {
        data.fill(0);
    }

    template<typename... Args>
    Vec(Args... args) : data{static_cast<T>(args)...} {
        static_assert(sizeof...(Args) == N, "Nombre d'arguments incorrect pour Vec<N>");
    }

    // Accès par indice
    T& operator[](size_t i) { return data[i]; }
    const T& operator[](size_t i) const { return data[i]; }

    // --- Opérateurs arithmétiques ---
    Vec operator+(const Vec& other) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) result[i] = data[i] + other[i];
        return result;
    }

    Vec operator-(const Vec& other) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) result[i] = data[i] - other[i];
        return result;
    }

    Vec operator*(T scalar) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) result[i] = data[i] * scalar;
        return result;
    }

    Vec operator/(T scalar) const {
        Vec result;
        for (size_t i = 0; i < N; ++i) result[i] = data[i] / scalar;
        return result;
    }

    Vec& operator+=(const Vec& other) {
        for (size_t i = 0; i < N; ++i) data[i] += other[i];
        return *this;
    }

    Vec& operator-=(const Vec& other) {
        for (size_t i = 0; i < N; ++i) data[i] -= other[i];
        return *this;
    }

    Vec& operator*=(T scalar) {
        for (size_t i = 0; i < N; ++i) data[i] *= scalar;
        return *this;
    }

    Vec& operator/=(T scalar) {
        for (size_t i = 0; i < N; ++i) data[i] /= scalar;
        return *this;
    }

    bool operator==(const Vec& other) const {
        for (size_t i = 0; i < N; ++i)
            if (data[i] != other[i]) return false;
        return true;
    }

    bool operator!=(const Vec& other) const {
        return !(*this == other);
    }

    // --- Fonctions utilitaires ---
    T length() const {
        T sum = 0;
        for (size_t i = 0; i < N; ++i) sum += data[i] * data[i];
        return std::sqrt(sum);
    }

    T lengthSquared() const {
        T sum = 0;
        for (size_t i = 0; i < N; ++i) sum += data[i] * data[i];
        return sum;
    }

    Vec normalized() const {
        T len = length();
        if (len == 0) return Vec();
        return *this / len;
    }

    void normalize() {
        T len = length();
        if (len == 0) return;
        for (size_t i = 0; i < N; ++i) data[i] /= len;
    }

    T dot(const Vec& other) const {
        T sum = 0;
        for (size_t i = 0; i < N; ++i) sum += data[i] * other[i];
        return sum;
    }

    Vec directionTo(const Vec& other) const {
        return (other - *this).normalized();
    }

    T distanceTo(const Vec& other) const {
        return (other - *this).length();
    }

    // --- Debug ---
    void print() const {
        std::cout << "Vec" << N << "(";
        for (size_t i = 0; i < N; ++i) {
            std::cout << data[i];
            if (i < N - 1) std::cout << ", ";
        }
        std::cout << ")" << std::endl;
    }
};

// --- Alias pratiques ---
using Vec2  = Vec<2, float>;
using Vec3  = Vec<3, float>;
using Vec4  = Vec<4, float>;

// ------------------------
//   TIMER CLASS
// ------------------------
class Timer {
    std::chrono::steady_clock::time_point start;
    float duration; // in seconds
    bool active = false;
public:
    void startTimer(float seconds) {
        duration = seconds;
        start = std::chrono::steady_clock::now();
        active = true;
    }

    bool isFinished() {
        if (!active) return true;
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - start).count();
        if (elapsed >= duration) {
            active = false;
            return true;
        }
        return false;
    }

    bool isActive() const {
        return active;
    }
};

// ------------------------
//   FILE CLASS
// ------------------------

class FileManager {
public:
    using json = nlohmann::json;

    // Initialise la racine (automatiquement ou manuellement)
    static void init() {
        namespace fs = std::filesystem;
        fs::path execPath = fs::current_path();
        if (execPath.filename() == "build")
            execPath = execPath.parent_path();
        basePath = execPath.string();
        std::cout << "[FileManager] Base path: " << basePath << std::endl;
    }

    static void SetBasePath(const std::string& path) { basePath = path; }
    static const std::string& GetBasePath() { return basePath; }

    // 🔹 Charge un fichier texte
    static std::string LoadTextFile(const std::string& relativePath) {
        namespace fs = std::filesystem;
        fs::path fullPath = fs::path(basePath) / "assets" / relativePath;
        std::ifstream file(fullPath);
        if (!file.is_open()) {
            throw std::runtime_error("[FileManager] Cannot open text file: " + fullPath.string());
        }
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    // 🔹 Charge un fichier binaire (ex: image, modèle)
    static std::vector<unsigned char> LoadBinaryFile(const std::string& relativePath) {
        namespace fs = std::filesystem;
        fs::path fullPath = fs::path(basePath) / "assets" / relativePath;
        std::ifstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("[FileManager] Cannot open binary file: " + fullPath.string());
        }
        return std::vector<unsigned char>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    // 🔹 Charge et parse un fichier JSON
    static json LoadJSONFile(const std::string& relativePath) {
        return json::parse(LoadTextFile(relativePath));
    }

    // ✅ Convertit un objet JSON déjà chargé en un type C++
    template<typename T>
    static T LoadJSON(const json& j) {
        return j.get<T>(); // nécessite une fonction from_json() pour le type T
    }

private:
    static inline std::string basePath = "";
};

#endif // UTILS_H

