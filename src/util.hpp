#pragma once

#include <string>
#include <vector>
#include <optional>
#include <iostream>


#include "imgui.h"

#include "glad/gl.h"





// References!
template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T>
Ref<T> make_ref() {
	return std::make_shared<T>();
}


template<typename T>
using WeakRef = std::weak_ptr<T>;

template<typename T>
WeakRef<T> make_weak_ref(Ref<T>& a) {
	return std::weak_ptr<T>(a);
}

template<typename T>
using Opt = std::optional<T>;


FILE* open_file(std::string filename, int retries = 5);


// Utility function to load a whole file
std::vector<uint8_t> load_file(std::string filename, size_t offset = 0, size_t len = -1, int retries = 5);

std::string gl_error_name(uint32_t error_code);


#define DEBUG_BREAK() __debugbreak();
#define GL_ERROR_CHECK() {\
	uint32_t e = glGetError(); \
	if(e != GL_NO_ERROR) { \
		std::cerr << "GL ERROR " << gl_error_name(e) << " in FILE " << __FILE__ << " AT LINE " << __LINE__ << "\n";\
		DEBUG_BREAK();\
	}\
}

void skip_whitespace(const char*& it);
void skip_whitespace_not_nl(const char*& it);
void skip_until_whitespace(const char*& it);
std::string get_next_token(const char*& it);
void skip_to_newline(const char*& it);
bool check_token(const char*& it, const std::string & str);

bool consume_token(const char*& it, const std::string & str);
bool consume_glsl_type(const char*& it);
std::string consume_next_token(const char*& it);
std::string consume_string(const char*& it);

float consume_float(const char*& it);
int consume_int(const char*& it);

bool maybe_consume_float(const char*& it, float& f);
bool maybe_consume_int(const char*& it, int& i);


namespace ImGui {
	// This class is an RAII style interface for Dear imgui's pushID / popID
	class Tag {
	public:
		Tag(const char* str_id);
		Tag(const char* str_id_begin, const char* str_id_end);
		Tag(const void* ptr_id);
		Tag(int int_id);

		~Tag();
	};
}

