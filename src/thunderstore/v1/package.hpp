#pragma once

#include "nlohmann/json.hpp"

#include <semver.hpp>

#ifndef NLOHMANN_OPT_HELPER
	#define NLOHMANN_OPT_HELPER

namespace nlohmann
{
	template<typename T>
	struct adl_serializer<std::shared_ptr<T>>
	{
		static void to_json(json& j, const std::shared_ptr<T>& opt)
		{
			if (!opt)
			{
				j = nullptr;
			}
			else
			{
				j = *opt;
			}
		}

		static std::shared_ptr<T> from_json(const json& j)
		{
			if (j.is_null())
			{
				return std::make_shared<T>();
			}
			else
			{
				return std::make_shared<T>(j.get<T>());
			}
		}
	};

	template<typename T>
	struct adl_serializer<std::optional<T>>
	{
		static void to_json(json& j, const std::optional<T>& opt)
		{
			if (!opt)
			{
				j = nullptr;
			}
			else
			{
				j = *opt;
			}
		}

		static std::optional<T> from_json(const json& j)
		{
			if (j.is_null())
			{
				return std::make_optional<T>();
			}
			else
			{
				return std::make_optional<T>(j.get<T>());
			}
		}
	};
} // namespace nlohmann
#endif

namespace ts::v1
{
	struct package_version
	{
		std::string name;
		std::string full_name;
		std::string description;
		std::string icon;
		std::string version_number;
		std::vector<std::string> dependencies;
		std::string download_url;
		int64_t downloads;
		std::string date_created;
		std::string website_url;
		bool is_active;
		std::string uuid4;
		int64_t file_size;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(package_version, name, full_name, description, icon, version_number, dependencies, download_url, downloads, date_created, website_url, is_active, uuid4, file_size)

		// Extra data
		ID3D11ShaderResourceView* icon_texture;
		bool is_installed           = false;
		std::string full_name_lower = "";
	};

	struct package
	{
		std::string name;
		std::string full_name;
		std::string owner;
		std::string package_url;
		std::string date_created{};
		std::string date_updated{};
		std::string uuid4{};
		int64_t rating_score{};
		bool is_pinned{};
		bool is_deprecated{};
		bool has_nsfw_content{};
		std::vector<std::string> categories{};
		std::vector<package_version> versions{};
		std::optional<std::string> donation_link{};

		NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(package, name, full_name, owner, package_url, date_created, date_updated, uuid4, rating_score, is_pinned, is_deprecated, has_nsfw_content, categories, versions, donation_link)

		bool is_installed                    = false;
		std::string installed_version_number = "";
		bool is_local                        = false;
		std::string full_name_lower          = "";
	};
} // namespace ts::v1
