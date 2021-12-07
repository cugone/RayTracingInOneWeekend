#pragma once

#include <chrono>


#define TOKEN_PASTE_SIMPLE(x, y) x##y
#define TOKEN_PASTE(x, y) TOKEN_PASTE_SIMPLE(x, y)
#define TOKEN_STRINGIZE_SIMPLE(x) #x
#define TOKEN_STRINGIZE(x) TOKEN_STRINGIZE_SIMPLE(x)


class ProfileLogScope {
public:
    explicit ProfileLogScope(const char* scopeName) noexcept;
    ~ProfileLogScope() noexcept;

    constexpr ProfileLogScope() = delete;
    constexpr ProfileLogScope(const ProfileLogScope&) = delete;
    constexpr ProfileLogScope(ProfileLogScope&&) = delete;
    constexpr ProfileLogScope& operator=(const ProfileLogScope&) = delete;
    constexpr ProfileLogScope& operator=(ProfileLogScope&&) = delete;

protected:
private:
    using time_point_t = std::chrono::time_point<std::chrono::steady_clock>;

    const char* _scope_name = nullptr;
    time_point_t _time_at_creation{};
};

#if defined PROFILE_LOG_SCOPE || defined PROFILE_LOG_SCOPE_FUNCTION
    #undef PROFILE_LOG_SCOPE
    #undef PROFILE_LOG_SCOPE_FUNCTION
#endif
#define PROFILE_LOG_SCOPE(tag_str) ProfileLogScope TOKEN_PASTE(plscope_, __LINE__)(tag_str)
#define PROFILE_LOG_SCOPE_FUNCTION() PROFILE_LOG_SCOPE(__FUNCSIG__)
