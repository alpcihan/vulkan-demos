#include "gpro/id.hpp"

namespace gpro {

static uint64_t s_count = 0;

ID::ID() : m_id(s_count++) {}

ID::ID(uint64_t id) : m_id(id) {}

}  // namespace gpro