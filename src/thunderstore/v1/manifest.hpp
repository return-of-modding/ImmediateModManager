#pragma once

#include "nlohmann/json.hpp"

#include <semver.hpp>

namespace ts::v1
{
	struct dependency
	{
		std::string team_name{};
		std::string name{};
		semver::version version{};
	};

	struct manifest
	{
		std::string name{};

		std::string version_number{};
		semver::version version{};

		std::string website_url{};

		std::string description{};

		std::vector<std::string> dependencies{};
		std::vector<dependency> dependency_objects{};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(manifest, name, version_number, website_url, description, dependencies)

		std::string author_name{};
		std::string author_and_name_and_version_number{};
	};
} // namespace ts::v1