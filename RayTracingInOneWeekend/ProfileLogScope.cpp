#include "ProfileLogScope.hpp"

#include <iostream>

ProfileLogScope::ProfileLogScope(const char* scopeName) noexcept
: _scope_name(scopeName)
, _time_at_creation(std::chrono::steady_clock::now()) {
    /* DO NOTHING */
}

ProfileLogScope::~ProfileLogScope() noexcept {
    const auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsedTime = (now - _time_at_creation);
    std::cerr << "ProfileLogScope " << _scope_name << " took " << elapsedTime.count() << " seconds.\n";
}
