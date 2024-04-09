#include "logger.hpp"

#include <filesystem>
#include <iostream>

std::shared_ptr<spdlog::logger> logger = {};

void init_logger()
{
	try
	{
		std::filesystem::path log_file_path = L"./LogOutput.log";

		if (std::filesystem::exists(log_file_path))
		{
			std::filesystem::remove_all(log_file_path);
		}

		logger = spdlog::basic_logger_mt("logger", log_file_path);
		logger->flush_on(spdlog::level::level_enum::info);
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		std::cout << "Log init failed: " << ex.what() << std::endl;
	}
}
