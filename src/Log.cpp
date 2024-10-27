
#include <engine/Log.hpp>

std::shared_ptr<spdlog::logger> Log::s_CoreLogger;

void Log::Init(){
    spdlog::set_pattern("%^[%T] [%L] %v%$");
    s_CoreLogger = spdlog::stdout_color_mt("CORE");
    s_CoreLogger->set_level(spdlog::level::trace);
}
