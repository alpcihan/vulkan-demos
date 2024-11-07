#pragma once

#include "gpro/shared.hpp"

namespace gpro {

class ID {
public:
    ID();
    ID(uint64_t id);
    ID(const ID&) = default;

    operator uint64_t() const { return m_id; }

private:
    uint64_t m_id;
};

}  // namespace gpro

namespace std {
template <typename T>
struct hash;

template <>
struct hash<gpro::ID> {
    std::size_t operator()(const gpro::ID& id) const { return (uint64_t)id; }
};
}  // namespace std