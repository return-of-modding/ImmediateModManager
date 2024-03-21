#pragma once
#include <locale>
#include <string.h>
#include <vector>

namespace imm::string
{
	inline bool starts_with(const char* pre, const char* str)
	{
		return strncmp(pre, str, strlen(pre)) == 0;
	}

	template<typename T>
	inline std::enable_if_t<std::is_same_v<T, std::string>, T> get_text_value(std::string text)
	{
		return text;
	}

	template<typename T>
	inline std::enable_if_t<!std::is_same_v<T, std::string>, T> get_text_value(std::string text)
	{
		T value = (T)0;
		std::stringstream(text) >> value;
		return value;
	}

	template<typename T = std::string>
	inline std::vector<T> split(const std::string& text, const char delim)
	{
		std::vector<T> result;
		std::string str;
		std::stringstream ss(text);
		while (std::getline(ss, str, delim))
		{
			result.push_back(get_text_value<T>(str));
		}
		return result;
	}

	inline std::string replace(const std::string& original, const std::string& old_value, const std::string& new_value)
	{
		std::string result = original;
		size_t pos         = 0;

		while ((pos = result.find(old_value, pos)) != std::string::npos)
		{
			result.replace(pos, old_value.length(), new_value);
			pos += new_value.length();
		}

		return result;
	}

	template<typename T>
	inline T to_lower(const T& input)
	{
		T result = input;

		// Use the std::locale to ensure proper case conversion for the current locale
		std::locale loc;

		for (size_t i = 0; i < result.length(); ++i)
		{
			result[i] = std::tolower(result[i], loc);
		}

		return result;
	}


} // namespace imm::string

namespace ImGui
{
	struct InputTextCallback_UserData
	{
		std::string* Str;
		ImGuiInputTextCallback ChainCallback;
		void* ChainCallbackUserData;
	};

	inline int InputTextCallback(ImGuiInputTextCallbackData* data)
	{
		InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
		{
			// Resize string callback
			// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
			std::string* str = user_data->Str;
			IM_ASSERT(data->Buf == str->c_str());
			str->resize(data->BufTextLen);
			data->Buf = (char*)str->c_str();
		}
		else if (user_data->ChainCallback)
		{
			// Forward to user callback, if any
			data->UserData = user_data->ChainCallbackUserData;
			return user_data->ChainCallback(data);
		}
		return 0;
	}

	inline bool InputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr)
	{
		IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
		flags |= ImGuiInputTextFlags_CallbackResize;

		InputTextCallback_UserData cb_user_data;
		cb_user_data.Str                   = str;
		cb_user_data.ChainCallback         = callback;
		cb_user_data.ChainCallbackUserData = user_data;
		return InputText(label, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
	}
} // namespace ImGui
